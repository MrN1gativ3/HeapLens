#include "chapter_fastbin.h"

#include "chapter_common.h"

GtkWidget *heaplens_chapter_fastbin_new(void) {
    static const char *topics[] = {
        "Fastbins are singly-linked LIFO lists for recently freed small chunks.",
        "Fastbin chunks deliberately keep PREV_INUSE set in the following chunk to postpone consolidation.",
        "glibc safe-linking mangles the stored fd pointer as (slot >> 12) XOR next.",
        "malloc_consolidate() flushes fastbins into the unsorted bin before larger allocations."
    };

    return heaplens_learning_build_chapter_page(
        "Chapter 7 - Fastbins",
        "Fastbins trade fragmentation for speed. They are one of the most important stepping stones between basic allocator internals and real exploitation primitives.",
        topics,
        G_N_ELEMENTS(topics),
        "HeapLens renders free-list arrows and shows both mangled and revealed forward pointers for fastbin-sized chunks.",
        "fastbin_index(size) = (size >> 4) - 2\nhead -> newest freed chunk -> older chunk -> ...",
        NULL);
}
