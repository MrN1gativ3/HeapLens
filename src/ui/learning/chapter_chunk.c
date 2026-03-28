#include "chapter_chunk.h"

#include "chapter_common.h"

GtkWidget *heaplens_chapter_chunk_new(void) {
    static const char *topics[] = {
        "On x86_64 the chunk header begins with prev_size and size, then the user region starts 16 bytes after the chunk pointer.",
        "The low three bits of size encode PREV_INUSE, IS_MMAPPED, and NON_MAIN_ARENA.",
        "When the previous chunk is in use, prev_size is dead space from the current chunk's point of view and can be repurposed as user data.",
        "Chunk navigation is pure pointer arithmetic: next = p + (size & ~0x7), prev = p - prev_size."
    };

    return heaplens_learning_build_chapter_page(
        "Chapter 4 - The malloc_chunk Structure",
        "Every heap attack eventually becomes a story about chunk headers. This page centers the exact binary layout so the later visualizer views feel concrete instead of magical.",
        topics,
        G_N_ELEMENTS(topics),
        "Cross-reference this chapter with the heap visualizer in the lab tab to see the same metadata parsed live out of /proc/<pid>/mem.",
        "offset 0x00: prev_size\noffset 0x08: size\noffset 0x10: user data or fd/bk overlay when free",
        NULL);
}
