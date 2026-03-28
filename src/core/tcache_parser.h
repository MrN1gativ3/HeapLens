#ifndef HEAPLENS_CORE_TCACHE_PARSER_H
#define HEAPLENS_CORE_TCACHE_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "heap_parser.h"

#define HEAPLENS_TCACHE_ENTRY_DEPTH 16

typedef struct {
    uint64_t address;
    uintptr_t raw_next;
    uint64_t decoded_next;
    uintptr_t key;
} HeapLensTcacheEntryInfo;

typedef struct {
    uint16_t count;
    uint64_t head;
    size_t depth;
    HeapLensTcacheEntryInfo entries[HEAPLENS_TCACHE_ENTRY_DEPTH];
} HeapLensTcacheBinSnapshot;

typedef struct {
    uint64_t address;
    struct tcache_perthread_struct raw;
    HeapLensTcacheBinSnapshot bins[TCACHE_MAX_BINS];
} HeapLensTcacheSnapshot;

bool heaplens_tcache_parser_probe(HeapLensMemoryReader *reader, uint64_t address);
bool heaplens_tcache_parser_read(HeapLensMemoryReader *reader,
                                 uint64_t address,
                                 HeapLensTcacheSnapshot *snapshot);

#endif
