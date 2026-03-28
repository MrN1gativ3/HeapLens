#include "ptrace_backend.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static bool heaplens_ptrace_call(enum __ptrace_request request,
                                 pid_t pid,
                                 void *address,
                                 void *data) {
    errno = 0;
    return ptrace(request, pid, address, data) != -1 || errno == 0;
}

bool heaplens_ptrace_attach(pid_t pid) {
    return heaplens_ptrace_call(PTRACE_ATTACH, pid, NULL, NULL);
}

bool heaplens_ptrace_seize(pid_t pid, unsigned long options) {
    return heaplens_ptrace_call(PTRACE_SEIZE, pid, NULL, (void *) (uintptr_t) options);
}

bool heaplens_ptrace_set_options(pid_t pid, unsigned long options) {
    return heaplens_ptrace_call(PTRACE_SETOPTIONS, pid, NULL, (void *) (uintptr_t) options);
}

bool heaplens_ptrace_interrupt(pid_t pid) {
    return heaplens_ptrace_call(PTRACE_INTERRUPT, pid, NULL, NULL);
}

bool heaplens_ptrace_continue(pid_t pid, int signal_number) {
    return heaplens_ptrace_call(PTRACE_CONT, pid, NULL, (void *) (uintptr_t) signal_number);
}

bool heaplens_ptrace_syscall(pid_t pid, int signal_number) {
    return heaplens_ptrace_call(PTRACE_SYSCALL, pid, NULL, (void *) (uintptr_t) signal_number);
}

bool heaplens_ptrace_single_step(pid_t pid, int signal_number) {
    return heaplens_ptrace_call(PTRACE_SINGLESTEP, pid, NULL, (void *) (uintptr_t) signal_number);
}

bool heaplens_ptrace_detach(pid_t pid, int signal_number) {
    return heaplens_ptrace_call(PTRACE_DETACH, pid, NULL, (void *) (uintptr_t) signal_number);
}

bool heaplens_ptrace_wait_stopped(pid_t pid, int *status, int timeout_ms) {
    int local_status = 0;
    int remaining_ms = timeout_ms;

    while (true) {
        pid_t waited = waitpid(pid, &local_status, timeout_ms >= 0 ? WNOHANG : 0);
        if (waited == pid) {
            if (status) {
                *status = local_status;
            }
            return true;
        }

        if (waited < 0) {
            return false;
        }

        if (timeout_ms >= 0 && remaining_ms <= 0) {
            errno = ETIMEDOUT;
            return false;
        }

        {
            struct timespec delay = {
                .tv_sec = 0,
                .tv_nsec = 10 * 1000 * 1000
            };

            nanosleep(&delay, NULL);
        }

        if (timeout_ms >= 0) {
            remaining_ms -= 10;
        }
    }
}

bool heaplens_ptrace_get_registers(pid_t pid, HeapLensRegisterSnapshot *snapshot) {
    if (!snapshot) {
        errno = EINVAL;
        return false;
    }

    memset(snapshot, 0, sizeof(*snapshot));

    if (!heaplens_ptrace_call(PTRACE_GETREGS, pid, NULL, &snapshot->gpr)) {
        return false;
    }

    /* x86_64 exposes xmm_space via PTRACE_GETFPREGS, which keeps the register panel lightweight. */
    snapshot->have_fpregs = heaplens_ptrace_call(PTRACE_GETFPREGS, pid, NULL, &snapshot->fpregs);
    return true;
}

void heaplens_ptrace_format_rflags(uint64_t rflags, char *buffer, size_t buffer_size) {
    static const struct {
        uint64_t mask;
        const char *name;
    } flags[] = {
        {1ULL << 0, "CF"},
        {1ULL << 2, "PF"},
        {1ULL << 4, "AF"},
        {1ULL << 6, "ZF"},
        {1ULL << 7, "SF"},
        {1ULL << 8, "TF"},
        {1ULL << 9, "IF"},
        {1ULL << 10, "DF"},
        {1ULL << 11, "OF"}
    };
    size_t offset = 0;
    size_t index = 0;

    if (!buffer || buffer_size == 0) {
        return;
    }

    buffer[0] = '\0';
    for (index = 0; index < sizeof(flags) / sizeof(flags[0]); ++index) {
        int written = snprintf(buffer + offset,
                               buffer_size > offset ? buffer_size - offset : 0,
                               "%s%s=%c",
                               index == 0 ? "" : " ",
                               flags[index].name,
                               (rflags & flags[index].mask) ? '1' : '0');
        if (written < 0) {
            break;
        }
        offset += (size_t) written;
        if (offset >= buffer_size) {
            break;
        }
    }
}
