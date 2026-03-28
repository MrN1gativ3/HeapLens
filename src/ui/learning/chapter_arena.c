#include "chapter_arena.h"

#include "chapter_common.h"

GtkWidget *heaplens_chapter_arena_new(void) {
    static const char *topics[] = {
        "main_arena is the allocator's central malloc_state and owns fastbins, bins, the top chunk, and bookkeeping fields such as system_mem.",
        "Additional arenas reduce lock contention for multi-threaded workloads by letting threads allocate from separate heaps.",
        "Arenas are linked through next/next_free, which means an address leak from one arena can often be pivoted into a broader allocator map."
    };

    return heaplens_learning_build_chapter_page(
        "Chapter 5 - Arena and malloc_state",
        "This chapter tours malloc_state field by field so learners can connect abstract allocator paths to the exact doubly-linked lists and counters that drive them.",
        topics,
        G_N_ELEMENTS(topics),
        "HeapLens keeps the malloc_state mirror in C so arena snapshots always come from target memory instead of host-side reinterpretation.",
        "fastbinsY[10]\ntop\nlast_remainder\nbins[254]\nbinmap[4]\nsystem_mem / max_system_mem",
        NULL);
}
