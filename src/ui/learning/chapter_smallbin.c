#include "chapter_smallbin.h"

#include "chapter_common.h"

GtkWidget *heaplens_chapter_smallbin_new(void) {
    static const char *topics[] = {
        "Smallbins hold fixed-size doubly-linked FIFO lists for chunk sizes up to roughly one kilobyte.",
        "binmap[] lets malloc skip empty bins without walking each list head.",
        "Unlike fastbins and tcache, smallbin chunks are coalesced before insertion."
    };

    return heaplens_learning_build_chapter_page(
        "Chapter 10 - Smallbins",
        "Smallbins are the allocator's orderly middle ground: size-segregated, doubly linked, and ready for straightforward unlink-style reasoning.",
        topics,
        G_N_ELEMENTS(topics),
        "The FIFO behavior matters during exploitation because the oldest chunk, not the newest, is usually returned first.",
        "smallbin_index(size) = (size >> 4) - 2\ninsert at head, remove from tail",
        NULL);
}
