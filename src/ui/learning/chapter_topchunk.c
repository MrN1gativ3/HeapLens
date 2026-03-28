#include "chapter_topchunk.h"

#include "chapter_common.h"

GtkWidget *heaplens_chapter_topchunk_new(void) {
    static const char *topics[] = {
        "The top chunk is the wilderness chunk that borders the current program break.",
        "malloc() splits from the top chunk when bins cannot satisfy a request.",
        "If the top chunk is too small, sysmalloc() extends the heap so the wilderness remains available.",
        "Corrupting the top chunk size is the core primitive behind House of Force."
    };

    return heaplens_learning_build_chapter_page(
        "Chapter 6 - The Top Chunk",
        "The allocator always keeps one chunk at the end of the heap that can be grown. Understanding the wilderness explains both normal heap expansion and some of the oldest top-chunk exploitation ideas.",
        topics,
        G_N_ELEMENTS(topics),
        "Watch the top chunk shrink and regrow in the lab tab when a staged demo repeatedly allocates and frees from an empty arena.",
        "request fits top -> split -> advance top\nrequest too large -> sysmalloc() -> brk/mmap",
        NULL);
}
