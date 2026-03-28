#include "chapter_mmap.h"

#include "chapter_common.h"

GtkWidget *heaplens_chapter_mmap_new(void) {
    static const char *topics[] = {
        "Large requests usually cross DEFAULT_MMAP_THRESHOLD and become stand-alone mappings created with MAP_PRIVATE | MAP_ANONYMOUS.",
        "IS_MMAPPED in the chunk size field tells free() to return the mapping with munmap() instead of binning the chunk.",
        "mmapped chunks never consolidate with neighboring heap chunks, which changes both fragmentation behavior and exploit surface."
    };

    return heaplens_learning_build_chapter_page(
        "Chapter 3 - mmap and Large Allocations",
        "Once requests become large enough, glibc stops extending the linear heap and allocates a dedicated mapping instead. This makes the chunk lifecycle easier to reason about because free() can hand memory back to the kernel immediately.",
        topics,
        G_N_ELEMENTS(topics),
        "Use this chapter alongside the syscall trace panel in the lab tab to compare brk growth with mmap/munmap churn.",
        "mmap(NULL, 135168, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)\n... free() ...\nmunmap(addr, 135168)",
        NULL);
}
