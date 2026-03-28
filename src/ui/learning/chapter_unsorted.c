#include "chapter_unsorted.h"

#include "chapter_common.h"

GtkWidget *heaplens_chapter_unsorted_new(void) {
    static const char *topics[] = {
        "The unsorted bin is the allocator's inbox for freshly consolidated chunks.",
        "Chunks arrive here before being redistributed into size-specific bins, which makes the list rich with transient metadata.",
        "A lone unsorted chunk often points back into main_arena, creating a classic libc leak."
    };

    return heaplens_learning_build_chapter_page(
        "Chapter 9 - Unsorted Bin",
        "The unsorted bin is where allocator state feels most alive: freshly freed chunks, remainders after splits, and fastbin flushes all briefly gather here before being sorted.",
        topics,
        G_N_ELEMENTS(topics),
        "This chapter pairs well with the unsorted-bin leak and attack techniques in the lab dropdown because the same fd/bk pointers become immediately visible.",
        "head <-> chunk A <-> chunk B <-> head",
        NULL);
}
