#ifndef HEAPLENS_CORE_PROCESS_MANAGER_H
#define HEAPLENS_CORE_PROCESS_MANAGER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "heap_parser.h"
#include "memory_reader.h"
#include "ptrace_backend.h"

typedef struct {
    pid_t pid;
    int stage_fd;
    bool running;
    bool traced;
    char binary_path[PATH_MAX];
    char glibc_version[16];
    char last_stage[128];
    HeapLensMemoryReader reader;
} HeapLensTargetProcess;

bool heaplens_process_spawn(HeapLensTargetProcess *process,
                            const char *binary_path,
                            const char *glibc_version,
                            char *const argv[]);
bool heaplens_process_continue(HeapLensTargetProcess *process);
bool heaplens_process_wait_for_stage(HeapLensTargetProcess *process,
                                     int timeout_ms,
                                     char *stage_line,
                                     size_t stage_line_size);
bool heaplens_process_snapshot(HeapLensTargetProcess *process,
                               HeapLensRegisterSnapshot *registers,
                               HeapLensHeapSnapshot *heap_snapshot);
bool heaplens_process_interrupt(HeapLensTargetProcess *process);
void heaplens_process_destroy(HeapLensTargetProcess *process);

#endif
