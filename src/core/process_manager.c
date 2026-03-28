#include "process_manager.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

static bool heaplens_process_read_stage_line(HeapLensTargetProcess *process,
                                             char *stage_line,
                                             size_t stage_line_size) {
    char buffer[127];
    ssize_t received = 0;
    bool have_stage = false;

    if (!process || process->stage_fd < 0) {
        return false;
    }

    while ((received = read(process->stage_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[received] = '\0';
        snprintf(process->last_stage, sizeof(process->last_stage), "%s", buffer);
        if (stage_line && stage_line_size > 0) {
            snprintf(stage_line, stage_line_size, "%s", buffer);
        }
        have_stage = true;
    }

    return have_stage;
}

static bool heaplens_wait_initial_stop(pid_t pid) {
    int status = 0;

    if (!heaplens_ptrace_wait_stopped(pid, &status, 3000)) {
        return false;
    }

    return WIFSTOPPED(status);
}

static void heaplens_process_apply_library_env(const char *preload_library) {
    const char *slash = NULL;

    if (!preload_library || !preload_library[0]) {
        return;
    }

    setenv("LD_PRELOAD", preload_library, 1);
    slash = strrchr(preload_library, '/');
    if (slash && slash != preload_library) {
        char directory[PATH_MAX];
        size_t length = (size_t) (slash - preload_library);

        if (length < sizeof(directory)) {
            memcpy(directory, preload_library, length);
            directory[length] = '\0';
            setenv("LD_LIBRARY_PATH", directory, 1);
        }
    }
}

bool heaplens_process_spawn(HeapLensTargetProcess *process,
                            const char *binary_path,
                            const char *glibc_version,
                            const char *preload_library,
                            char *const argv[]) {
    int stage_pipe[2] = {-1, -1};
    pid_t child = -1;

    if (!process || !binary_path) {
        errno = EINVAL;
        return false;
    }

    memset(process, 0, sizeof(*process));
    process->reader.mem_fd = -1;
    process->stage_fd = -1;

    if (pipe(stage_pipe) != 0) {
        return false;
    }

    child = fork();
    if (child < 0) {
        close(stage_pipe[0]);
        close(stage_pipe[1]);
        return false;
    }

    if (child == 0) {
        if (stage_pipe[1] != 3 && dup2(stage_pipe[1], 3) < 0) {
            _exit(127);
        }
        if (stage_pipe[0] != 3) {
            close(stage_pipe[0]);
        }
        if (stage_pipe[1] != 3) {
            close(stage_pipe[1]);
        }

        /* TRACEME lets the parent own the process before the demo begins emitting stages. */
        if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) {
            _exit(127);
        }

        raise(SIGSTOP);
        heaplens_process_apply_library_env(preload_library);
        execv(binary_path, argv);
        _exit(127);
    }

    close(stage_pipe[1]);
    fcntl(stage_pipe[0], F_SETFL, fcntl(stage_pipe[0], F_GETFL, 0) | O_NONBLOCK);

    if (!heaplens_wait_initial_stop(child)) {
        close(stage_pipe[0]);
        kill(child, SIGKILL);
        return false;
    }

    if (!heaplens_ptrace_set_options(child, PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEEXEC | PTRACE_O_EXITKILL)) {
        close(stage_pipe[0]);
        kill(child, SIGKILL);
        return false;
    }

    if (!heaplens_memory_reader_open(&process->reader, child)) {
        close(stage_pipe[0]);
        kill(child, SIGKILL);
        return false;
    }

    process->pid = child;
    process->stage_fd = stage_pipe[0];
    process->running = true;
    process->traced = true;
    snprintf(process->binary_path, sizeof(process->binary_path), "%s", binary_path);
    snprintf(process->preload_library,
             sizeof(process->preload_library),
             "%s",
             preload_library ? preload_library : "");
    snprintf(process->glibc_version, sizeof(process->glibc_version), "%s", glibc_version ? glibc_version : "native");
    return heaplens_process_continue(process);
}

bool heaplens_process_continue(HeapLensTargetProcess *process) {
    if (!process || !process->running) {
        errno = EINVAL;
        return false;
    }

    return heaplens_ptrace_continue(process->pid, 0);
}

bool heaplens_process_wait_for_stage(HeapLensTargetProcess *process,
                                     int timeout_ms,
                                     char *stage_line,
                                     size_t stage_line_size) {
    int status = 0;
    int remaining_ms = timeout_ms;

    if (!process || !process->running) {
        errno = EINVAL;
        return false;
    }

    while (timeout_ms < 0 || remaining_ms > 0) {
        int wait_ms = timeout_ms < 0 ? -1 : (remaining_ms > 200 ? 200 : remaining_ms);
        bool have_stage = false;

        if (!heaplens_ptrace_wait_stopped(process->pid, &status, wait_ms)) {
            if (timeout_ms >= 0 && errno == ETIMEDOUT) {
                remaining_ms -= wait_ms;
                continue;
            }
            return false;
        }

        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            process->running = false;
            return false;
        }

        if (!WIFSTOPPED(status)) {
            if (timeout_ms >= 0) {
                remaining_ms -= wait_ms;
            }
            continue;
        }

        have_stage = heaplens_process_read_stage_line(process, stage_line, stage_line_size);
        if (have_stage || WSTOPSIG(status) == SIGSTOP) {
            return true;
        }

        if (!heaplens_ptrace_continue(process->pid, 0)) {
            return false;
        }

        if (timeout_ms >= 0) {
            remaining_ms -= wait_ms;
        }
    }

    errno = ETIMEDOUT;
    return false;
}

bool heaplens_process_snapshot(HeapLensTargetProcess *process,
                               HeapLensRegisterSnapshot *registers,
                               HeapLensHeapSnapshot *heap_snapshot) {
    if (!process || !process->running || !registers || !heap_snapshot) {
        errno = EINVAL;
        return false;
    }

    if (!heaplens_ptrace_get_registers(process->pid, registers)) {
        return false;
    }

    /*
     * Heap decoding is best-effort: keep any partial mapping/tcache data so the UI can still
     * render process state even when chunk walking fails on the current glibc layout.
     */
    heaplens_heap_parser_scan_heap(&process->reader, process->pid, 256, heap_snapshot);

    return true;
}

bool heaplens_process_interrupt(HeapLensTargetProcess *process) {
    if (!process || !process->running) {
        errno = EINVAL;
        return false;
    }

    if (heaplens_ptrace_interrupt(process->pid)) {
        return true;
    }

    /* TRACEME children do not always accept PTRACE_INTERRUPT, so SIGSTOP is the fallback. */
    return kill(process->pid, SIGSTOP) == 0;
}

void heaplens_process_destroy(HeapLensTargetProcess *process) {
    if (!process) {
        return;
    }

    if (process->running) {
        kill(process->pid, SIGKILL);
        waitpid(process->pid, NULL, 0);
    }

    if (process->stage_fd >= 0) {
        close(process->stage_fd);
    }

    heaplens_memory_reader_close(&process->reader);
    memset(process, 0, sizeof(*process));
    process->stage_fd = -1;
    process->reader.mem_fd = -1;
}
