#include "chapter_consolidate.h"

#include "chapter_common.h"

GtkWidget *heaplens_chapter_consolidate_new(void) {
    static const char *topics[] = {
        "Backward consolidation uses prev_size only when PREV_INUSE is clear in the current chunk.",
        "Forward consolidation checks the neighboring chunk state and merges unless the neighbor is still in use.",
        "If the neighbor is the top chunk, free() absorbs the merged range into the wilderness instead of binning it."
    };

    return heaplens_learning_build_chapter_page(
        "Chapter 12 - Memory Consolidation and Coalescing",
        "Allocator performance is not just about allocation fast paths. Coalescing controls fragmentation, rebuilds larger free ranges, and strongly shapes which exploit primitives stay viable after a free().",
        topics,
        G_N_ELEMENTS(topics),
        "Compare this chapter against tcache behavior to see why tcache intentionally delays coalescing in exchange for speed.",
        "if !PREV_INUSE -> merge backward\nif next is free -> merge forward\nif next is top -> absorb into top",
        NULL);
}
