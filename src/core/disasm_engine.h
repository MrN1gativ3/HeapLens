#ifndef HEAPLENS_CORE_DISASM_ENGINE_H
#define HEAPLENS_CORE_DISASM_ENGINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <capstone/capstone.h>

#include "memory_reader.h"

typedef struct {
    csh handle;
    bool ready;
} HeapLensDisasmEngine;

bool heaplens_disasm_engine_init(HeapLensDisasmEngine *engine);
void heaplens_disasm_engine_shutdown(HeapLensDisasmEngine *engine);
char *heaplens_disasm_buffer(HeapLensDisasmEngine *engine,
                             uint64_t address,
                             const uint8_t *bytes,
                             size_t size);
char *heaplens_disasm_process_region(HeapLensDisasmEngine *engine,
                                     HeapLensMemoryReader *reader,
                                     uint64_t address,
                                     size_t size);

#endif
