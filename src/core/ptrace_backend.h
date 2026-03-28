#ifndef HEAPLENS_CORE_PTRACE_BACKEND_H
#define HEAPLENS_CORE_PTRACE_BACKEND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/user.h>

typedef struct {
    struct user_regs_struct gpr;
    struct user_fpregs_struct fpregs;
    bool have_fpregs;
} HeapLensRegisterSnapshot;

bool heaplens_ptrace_attach(pid_t pid);
bool heaplens_ptrace_seize(pid_t pid, unsigned long options);
bool heaplens_ptrace_set_options(pid_t pid, unsigned long options);
bool heaplens_ptrace_interrupt(pid_t pid);
bool heaplens_ptrace_continue(pid_t pid, int signal_number);
bool heaplens_ptrace_syscall(pid_t pid, int signal_number);
bool heaplens_ptrace_single_step(pid_t pid, int signal_number);
bool heaplens_ptrace_detach(pid_t pid, int signal_number);
bool heaplens_ptrace_wait_stopped(pid_t pid, int *status, int timeout_ms);
bool heaplens_ptrace_get_registers(pid_t pid, HeapLensRegisterSnapshot *snapshot);
void heaplens_ptrace_format_rflags(uint64_t rflags, char *buffer, size_t buffer_size);

#endif
