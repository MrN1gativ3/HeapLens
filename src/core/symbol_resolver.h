#ifndef HEAPLENS_CORE_SYMBOL_RESOLVER_H
#define HEAPLENS_CORE_SYMBOL_RESOLVER_H

#include <stdbool.h>
#include <stdint.h>

#include "memory_reader.h"

typedef struct {
    char name[128];
    char module_path[PATH_MAX];
    uint64_t module_base;
    uint64_t offset;
    uint64_t address;
} HeapLensResolvedSymbol;

bool heaplens_symbol_find_module(pid_t pid,
                                 const char *needle,
                                 HeapLensResolvedSymbol *module_info);
bool heaplens_symbol_resolve(const char *module_path,
                             uint64_t module_base,
                             const char *symbol_name,
                             HeapLensResolvedSymbol *out_symbol);

#endif
