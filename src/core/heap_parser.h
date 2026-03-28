#ifndef HEAPLENS_CORE_HEAP_PARSER_H
#define HEAPLENS_CORE_HEAP_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "memory_reader.h"

/* Mirror of glibc malloc_chunk */
struct malloc_chunk {
    size_t prev_size;   /* size of previous chunk (if free) */
    size_t size;        /* size of this chunk + flag bits */
    struct malloc_chunk *fd;          /* forward pointer (free only) */
    struct malloc_chunk *bk;          /* backward pointer (free only) */
    struct malloc_chunk *fd_nextsize; /* large bin fwd (large free only) */
    struct malloc_chunk *bk_nextsize; /* large bin bk  (large free only) */
};

#define PREV_INUSE      0x1
#define IS_MMAPPED      0x2
#define NON_MAIN_ARENA  0x4
#define SIZE_BITS       (PREV_INUSE | IS_MMAPPED | NON_MAIN_ARENA)
#define chunk_size(p)   ((p)->size & ~SIZE_BITS)
#define chunk_fd(p)     ((p)->fd)
#define chunk_bk(p)     ((p)->bk)
#define next_chunk(p)   ((struct malloc_chunk*)((char*)(p) + chunk_size(p)))
#define prev_chunk(p)   ((struct malloc_chunk*)((char*)(p) - (p)->prev_size))

/* Mirror of glibc malloc_state (main_arena) */
struct malloc_state {
    int flags;
    int have_fastchunks;
    struct malloc_chunk *fastbinsY[10];   /* NFASTBINS = 10 */
    struct malloc_chunk *top;
    struct malloc_chunk *last_remainder;
    struct malloc_chunk *bins[254];       /* smallbins + largebins */
    unsigned int binmap[4];
    struct malloc_state *next;
    struct malloc_state *next_free;
    size_t attached_threads;
    size_t system_mem;
    size_t max_system_mem;
};

/* Mirror of glibc tcache_perthread_struct */
#define TCACHE_MAX_BINS 64
struct tcache_entry { struct tcache_entry *next; uintptr_t key; };
struct tcache_perthread_struct {
    uint16_t counts[TCACHE_MAX_BINS];
    struct tcache_entry *entries[TCACHE_MAX_BINS];
};

#define HEAPLENS_CHUNK_LABEL_MAX 32
#define HEAPLENS_CHUNK_VALIDATION_MAX 128

typedef struct {
    uint64_t address;
    struct malloc_chunk raw;
    size_t decoded_size;
    size_t usable_size;
    bool prev_inuse;
    bool is_mmapped;
    bool non_main_arena;
    bool is_valid;
    uintptr_t decoded_fd;
    uintptr_t decoded_bk;
    char bin_name[HEAPLENS_CHUNK_LABEL_MAX];
    char validation_error[HEAPLENS_CHUNK_VALIDATION_MAX];
} HeapLensChunkInfo;

typedef struct {
    uint64_t heap_start;
    uint64_t heap_end;
    uint64_t tcache_address;
    struct malloc_state arena;
    bool have_arena;
    HeapLensChunkInfo *chunks;
    size_t chunk_count;
} HeapLensHeapSnapshot;

bool heaplens_heap_parser_read_chunk(HeapLensMemoryReader *reader,
                                     uint64_t address,
                                     struct malloc_chunk *out_chunk);

bool heaplens_heap_parser_read_arena(HeapLensMemoryReader *reader,
                                     uint64_t address,
                                     struct malloc_state *out_arena);

bool heaplens_heap_parser_scan_heap(HeapLensMemoryReader *reader,
                                    pid_t pid,
                                    size_t max_chunks,
                                    HeapLensHeapSnapshot *out_snapshot);

void heaplens_heap_snapshot_free(HeapLensHeapSnapshot *snapshot);

size_t heaplens_request_to_chunk_size(size_t request_size);
uintptr_t heaplens_safe_link_reveal(uintptr_t storage_position, uintptr_t stored_pointer);

#endif
