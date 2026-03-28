#include "heap_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symbol_resolver.h"
#include "tcache_parser.h"

static bool heaplens_heap_chunk_validate(uint64_t address,
                                         uint64_t heap_end,
                                         HeapLensChunkInfo *chunk) {
    if (chunk->decoded_size < 0x20) {
        snprintf(chunk->validation_error,
                 sizeof(chunk->validation_error),
                 "chunk size %#zx is smaller than MINSIZE (0x20 on x86_64)",
                 chunk->decoded_size);
        return false;
    }

    if ((chunk->decoded_size & 0xf) != 0) {
        snprintf(chunk->validation_error,
                 sizeof(chunk->validation_error),
                 "chunk size %#zx is not 16-byte aligned",
                 chunk->decoded_size);
        return false;
    }

    if (address + chunk->decoded_size > heap_end) {
        snprintf(chunk->validation_error,
                 sizeof(chunk->validation_error),
                 "chunk extends beyond [heap] mapping");
        return false;
    }

    return true;
}

bool heaplens_heap_parser_read_chunk(HeapLensMemoryReader *reader,
                                     uint64_t address,
                                     struct malloc_chunk *out_chunk) {
    return heaplens_memory_reader_read_fully(reader, address, out_chunk, sizeof(*out_chunk));
}

bool heaplens_heap_parser_read_arena(HeapLensMemoryReader *reader,
                                     uint64_t address,
                                     struct malloc_state *out_arena) {
    return heaplens_memory_reader_read_fully(reader, address, out_arena, sizeof(*out_arena));
}

size_t heaplens_request_to_chunk_size(size_t request_size) {
    size_t with_header = request_size + sizeof(size_t);
    size_t aligned = (with_header + 0x0fU) & ~(size_t) 0x0fU;

    /* glibc keeps the minimum chunk large enough to hold fd/bk when a chunk becomes free. */
    return aligned < 0x20 ? 0x20 : aligned;
}

uintptr_t heaplens_safe_link_reveal(uintptr_t storage_position, uintptr_t stored_pointer) {
    /* glibc safe-linking stores next as (storage_position >> 12) XOR real_pointer. */
    return (storage_position >> 12) ^ stored_pointer;
}

bool heaplens_heap_parser_scan_heap(HeapLensMemoryReader *reader,
                                    pid_t pid,
                                    size_t max_chunks,
                                    HeapLensHeapSnapshot *out_snapshot) {
    HeapLensMemoryMapSnapshot maps;
    const HeapLensMemoryMapEntry *heap_map = NULL;
    HeapLensResolvedSymbol libc_module;
    HeapLensResolvedSymbol main_arena;
    uint64_t cursor = 0;
    size_t chunk_count = 0;
    bool have_tcache = false;

    if (!reader || !out_snapshot || max_chunks == 0) {
        return false;
    }

    memset(out_snapshot, 0, sizeof(*out_snapshot));
    memset(&maps, 0, sizeof(maps));

    if (!heaplens_memory_reader_read_maps(pid, &maps)) {
        return false;
    }

    heap_map = heaplens_memory_map_find_named(&maps, "[heap]");
    if (!heap_map) {
        heaplens_memory_map_snapshot_free(&maps);
        return false;
    }

    out_snapshot->heap_start = heap_map->start;
    out_snapshot->heap_end = heap_map->end;
    memset(&libc_module, 0, sizeof(libc_module));
    memset(&main_arena, 0, sizeof(main_arena));
    if (heaplens_symbol_find_module(pid, "libc", &libc_module) &&
        heaplens_symbol_resolve(libc_module.module_path, libc_module.module_base, "main_arena", &main_arena) &&
        heaplens_heap_parser_read_arena(reader, main_arena.address, &out_snapshot->arena)) {
        out_snapshot->have_arena = true;
    }

    out_snapshot->chunks = calloc(max_chunks, sizeof(*out_snapshot->chunks));
    if (!out_snapshot->chunks) {
        heaplens_memory_map_snapshot_free(&maps);
        return false;
    }

    cursor = heap_map->start;
    have_tcache = heaplens_tcache_parser_probe(reader, cursor);
    if (have_tcache) {
        out_snapshot->tcache_address = cursor;
        /*
         * The tcache_perthread_struct lives in the user-data portion of the heap's first allocation,
         * so we skip its rounded-up footprint before attempting to interpret following bytes as chunks.
         */
        cursor += (uint64_t) ((sizeof(struct tcache_perthread_struct) + 0x0fU) & ~(size_t) 0x0fU);
    }

    while (cursor + sizeof(struct malloc_chunk) <= heap_map->end && chunk_count < max_chunks) {
        HeapLensChunkInfo *chunk = &out_snapshot->chunks[chunk_count];

        memset(chunk, 0, sizeof(*chunk));
        chunk->address = cursor;
        if (!heaplens_heap_parser_read_chunk(reader, cursor, &chunk->raw)) {
            break;
        }

        chunk->decoded_size = chunk_size(&chunk->raw);
        chunk->usable_size = chunk->decoded_size >= 0x10 ? chunk->decoded_size - 0x10 : 0;
        chunk->prev_inuse = (chunk->raw.size & PREV_INUSE) != 0;
        chunk->is_mmapped = (chunk->raw.size & IS_MMAPPED) != 0;
        chunk->non_main_arena = (chunk->raw.size & NON_MAIN_ARENA) != 0;
        snprintf(chunk->bin_name, sizeof(chunk->bin_name), "%s", chunk->is_mmapped ? "mmapped" : "heap");

        /*
         * Safe-linked forward pointers are revealed by XORing with the storage slot address shifted
         * down by 12 bits. The slot for fd begins at chunk_addr + offsetof(struct malloc_chunk, fd).
         */
        chunk->decoded_fd = heaplens_safe_link_reveal(cursor + offsetof(struct malloc_chunk, fd),
                                                      (uintptr_t) chunk->raw.fd);
        chunk->decoded_bk = (uintptr_t) chunk->raw.bk;
        chunk->is_valid = heaplens_heap_chunk_validate(cursor, heap_map->end, chunk);
        if (!chunk->is_valid) {
            break;
        }

        cursor += chunk->decoded_size;
        ++chunk_count;
    }

    out_snapshot->chunk_count = chunk_count;
    heaplens_memory_map_snapshot_free(&maps);
    return chunk_count > 0;
}

void heaplens_heap_snapshot_free(HeapLensHeapSnapshot *snapshot) {
    if (!snapshot) {
        return;
    }

    free(snapshot->chunks);
    snapshot->chunks = NULL;
    snapshot->chunk_count = 0;
    snapshot->heap_start = 0;
    snapshot->heap_end = 0;
    snapshot->tcache_address = 0;
    snapshot->have_arena = false;
}
