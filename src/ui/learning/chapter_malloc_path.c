#include "chapter_malloc_path.h"

#include "chapter_common.h"

GtkWidget *heaplens_chapter_malloc_path_new(void) {
    static const char *topics[] = {
        "__libc_malloc checks hooks, chooses an arena, aligns the request, then walks tcache, fastbins, smallbins, unsorted, largebins, and finally sysmalloc().",
        "Each branch corresponds to a different allocator data structure and therefore a different visual pattern in HeapLens.",
        "Following the full path is the bridge between reading allocator code and reasoning about exploit reliability."
    };

    return heaplens_learning_build_chapter_page(
        "Chapter 13 - malloc() Complete Call Path",
        "This chapter is the allocator's road map. The lab tab reuses the same sequence concept so a learner can step a demo and correlate it with the control-flow nodes described here.",
        topics,
        G_N_ELEMENTS(topics),
        "When a lab snapshot lands in an unexpected bin, come back to this page and trace which earlier branch would have sent the request there.",
        "__libc_malloc -> arena_get -> _int_malloc -> tcache / bins / sysmalloc",
        NULL);
}
