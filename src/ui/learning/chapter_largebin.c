#include "chapter_largebin.h"

#include "chapter_common.h"

GtkWidget *heaplens_chapter_largebin_new(void) {
    static const char *topics[] = {
        "Largebins cover wider size ranges and maintain additional nextsize links to speed best-fit search.",
        "Chunks are ordered by size within each bin, which exposes fd_nextsize/bk_nextsize as attack targets.",
        "Largebin attacks usually hinge on corrupting sorted-list metadata after an unsorted-bin transition."
    };

    return heaplens_learning_build_chapter_page(
        "Chapter 11 - Largebins",
        "Largebins are where allocator bookkeeping becomes more sophisticated. The extra ordering metadata is powerful for performance and equally powerful when corruption is possible.",
        topics,
        G_N_ELEMENTS(topics),
        "The lab tab groups largebin attacks separately so learners can focus on size-ordered unlink behavior without mixing it up with fastbin or tcache rules.",
        "fd/bk maintain the bin list\nfd_nextsize/bk_nextsize maintain sorted-by-size links",
        NULL);
}
