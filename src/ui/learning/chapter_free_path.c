#include "chapter_free_path.h"

#include "chapter_common.h"

GtkWidget *heaplens_chapter_free_path_new(void) {
    static const char *topics[] = {
        "free() converts the user pointer back into a chunk, validates alignment and size, then chooses tcache, fastbin, or consolidation paths.",
        "IS_MMAPPED short-circuits the normal allocator and returns the mapping directly to the kernel.",
        "Most classic integrity checks live on the free() path, which is why exploit setups often revolve around shaping a future free."
    };

    return heaplens_learning_build_chapter_page(
        "Chapter 14 - free() Complete Call Path",
        "The free() path decides whether metadata is cached, linked into a bin, consolidated, or rejected. That makes it the place where mitigations and exploit primitives collide most often.",
        topics,
        G_N_ELEMENTS(topics),
        "Use this chapter side-by-side with the mitigation chapter whenever a demo aborts during a staged free.",
        "__libc_free -> mem2chunk -> _int_free -> tcache / fastbin / consolidate / unsorted",
        NULL);
}
