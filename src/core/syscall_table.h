#ifndef HEAPLENS_CORE_SYSCALL_TABLE_H
#define HEAPLENS_CORE_SYSCALL_TABLE_H

#include <stddef.h>
#include <stdint.h>

const char *heaplens_syscall_name(long syscall_number);
void heaplens_format_syscall(long syscall_number,
                             const unsigned long arguments[6],
                             long return_value,
                             char *buffer,
                             size_t buffer_size);

#endif
