#include "chapter_vmem.h"

#include <unistd.h>

#include "../memory_map_panel.h"
#include "chapter_common.h"

GtkWidget *heaplens_chapter_vmem_new(void) {
    static const char *topics[] = {
        "Each /proc/<pid>/maps row exposes start-end addresses, permissions, file offset, device, inode, and optional backing path.",
        "Anonymous regions back the heap, stacks, and glibc allocator state, while file-backed VMAs map ELF segments and shared objects.",
        "[heap] grows upward via the program break, [stack] grows downward, and [vdso]/[vvar]/[vsyscall] are kernel-provided helper regions.",
        "Demand paging means a mapping can exist long before physical pages are faulted in."
    };
    GtkWidget *map_panel = heaplens_memory_map_panel_new();

    heaplens_memory_map_panel_load_pid(map_panel, getpid());
    return heaplens_learning_build_chapter_page(
        "Chapter 1 - Virtual Memory and Process Address Space",
        "This chapter grounds the rest of HeapLens in Linux virtual memory. Click the live rows below to inspect how the current process is laid out from low addresses to high addresses.",
        topics,
        G_N_ELEMENTS(topics),
        "Interactive demo: the table below is populated from a live /proc/self/maps read so learners can inspect real mappings immediately.",
        "0x400000 [.text]\n0x600000 [.data/.bss]\n0x...    [heap] grows upward\n0x7fff.. [stack] grows downward",
        map_panel);
}
