#include "chapter_tcache.h"

#include "chapter_common.h"

GtkWidget *heaplens_chapter_tcache_new(void) {
    static const char *topics[] = {
        "tcache_perthread_struct keeps 64 per-thread bins so small allocations avoid arena locking.",
        "A freed chunk becomes a tcache_entry with next and key embedded directly in its user data.",
        "counts[] tracks occupancy while entries[] points at the head of each singly-linked list.",
        "glibc checks the key field on free() to detect common tcache double-free patterns."
    };

    return heaplens_learning_build_chapter_page(
        "Chapter 8 - Tcache",
        "Modern heap exploitation often starts with tcache because it is fast, simple, and extremely visible. HeapLens parses the per-thread structure directly out of target memory for this chapter and the lab tab.",
        topics,
        G_N_ELEMENTS(topics),
        "In the exploitation tab, the heap visualizer reserves the heap-base tcache structure as its own annotated region when the probe succeeds.",
        "counts[idx] > 0 -> pop entries[idx]\ncounts[idx] < 7 on free -> push chunk immediately",
        NULL);
}
