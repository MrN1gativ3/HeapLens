#include "chapter_common.h"

#include <math.h>
#include <string.h>
#include <unistd.h>

#include "../../challenges/challenge_engine.h"
#include "../../challenges/challenges.h"
#include "../../core/heap_parser.h"
#include "../memory_map_panel.h"

#if __has_include(<gtksourceview/gtksource.h>)
#include <gtksourceview/gtksource.h>
#define HEAPLENS_HAVE_SOURCE_VIEW 1
#else
#define HEAPLENS_HAVE_SOURCE_VIEW 0
#endif

typedef struct {
    int id;
    const char *title;
    const char *theory;
    const char *source;
    const char *demo;
} HeapLensChapterContent;

typedef struct {
    int chapter_id;
    GtkWidget *drawing;
    GtkWidget *info_primary;
    GtkWidget *info_secondary;
    HeapLensMemoryMapSnapshot maps;
    struct {
        double y0;
        double y1;
        char tooltip[256];
    } regions[64];
    size_t region_count;
    int step_index;
} HeapLensChapterDiagramState;

static void heaplens_chapter_diagram_state_free(gpointer data) {
    HeapLensChapterDiagramState *state = data;

    if (!state) {
        return;
    }
    heaplens_memory_map_snapshot_free(&state->maps);
    g_free(state);
}

static const HeapLensChapterContent heaplens_chapters[] = {
    {
        1,
        "Chapter 1 - Virtual Memory",
        "Linux exposes user space as a stack of virtual memory areas rather than one flat array of bytes. The process map tells you which ranges are file-backed, which are anonymous, which are executable, and where the heap and stack actually sit.\n\nThis chapter ties the allocator back to process layout: the heap grows upward from the data segment, the stack grows downward, libc and ld.so live in shared-object mappings, and the kernel reserves the top of the address space. Hover the diagram to inspect the live VMAs that back HeapLens itself.",
        "/* How the kernel tracks VMAs - simplified */\nstruct vm_area_struct {\n    unsigned long vm_start;\n    unsigned long vm_end;\n    unsigned long vm_flags;\n    struct file *vm_file;\n};\n\n/* Reading the process map from userspace */\nint fd = open(\"/proc/self/maps\", O_RDONLY);\n/* Each line: start-end perms offset dev inode pathname */\n",
        "The live table below is populated from /proc/self/maps so you can compare the cairo diagram against the exact rows the kernel exports."
    },
    {
        2,
        "Chapter 2 - brk / sbrk",
        "glibc does not call brk() for every allocation. It grows the heap in page-aligned chunks, turns the fresh memory into a new or expanded top chunk, and then serves subsequent small requests from that wilderness chunk until it needs more pages.\n\nUse the step control in the top diagram to watch the break pointer move, then compare it against the malloc control-flow diagram in the Demo tab.",
        "static void *sysmalloc(size_t nb, mstate av) {\n    size_t size = nb + mp_.top_pad;\n    size = ALIGN_UP(size, pagesize);\n\n    char *new_brk = (char*)MORECORE(size);\n    if (new_brk == (char*)MORECORE_FAILURE)\n        goto try_mmap;\n\n    av->top = (mchunkptr)old_brk;\n    set_head(av->top, (new_brk - old_brk + old_size) | PREV_INUSE);\n}\n\n/* sbrk(increment) internally drives brk(current_break + increment). */\n",
        "The demo tab adds the malloc path flowchart so the break animation stays anchored to the allocator branches that can actually reach sysmalloc()."
    },
    {
        3,
        "Chapter 3 - mmap Allocations",
        "Once a request crosses the mmap threshold, glibc stops extending the linear heap and creates a dedicated mapping. That mapping gets the IS_MMAPPED flag in the chunk header and free() returns it directly to the kernel with munmap() instead of feeding it through bins or coalescing.\n\nThe diagram highlights how mmapped chunks stand apart from the main heap and why their lifecycle is much shorter.",
        "if ((unsigned long)(nb) >= (unsigned long)(mp_.mmap_threshold) &&\n    (mp_.n_mmaps < mp_.n_mmaps_max)) {\n    char *mm = (char *)MMAP(NULL, size, PROT_READ | PROT_WRITE,\n                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);\n    set_head(p, size | IS_MMAPPED);\n}\n",
        "Use the challenge to force a large allocation and inspect the size field directly."
    },
    {
        4,
        "Chapter 4 - malloc_chunk",
        "The chunk header is the allocator's core data structure. Every later exploit primitive reduces to understanding where prev_size, size, fd, bk, and the user pointer actually land in memory.\n\nThe top diagram lays out a 0x30-byte chunk byte-for-byte and then shows the arithmetic glibc uses to walk to adjacent chunks.",
        "#define chunk2mem(p)    ((void*)((char*)(p) + 2*SIZE_SZ))\n#define mem2chunk(mem) ((mchunkptr)((char*)(mem) - 2*SIZE_SZ))\n#define MINSIZE  (unsigned long)(((MIN_CHUNK_SIZE+MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK))\n#define request2size(req) (((req) + SIZE_SZ + MALLOC_ALIGN_MASK < MINSIZE) ? MINSIZE : ((req) + SIZE_SZ + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)\n#define PREV_INUSE      0x1\n#define IS_MMAPPED      0x2\n#define NON_MAIN_ARENA  0x4\n#define chunksize(p) (chunksize_nomask(p) & ~(SIZE_BITS))\n#define next_chunk(p) ((mchunkptr)(((char*)(p)) + chunksize(p)))\n#define prev_chunk(p) ((mchunkptr)(((char*)(p)) - prev_size(p)))\n",
        "Pair this page with the Heap debugger window to compare the static layout against a live target chunk."
    },
    {
        5,
        "Chapter 5 - Arena and malloc_state",
        "Arenas hold the allocator's global bookkeeping: fastbin heads, bin array, top chunk, last remainder, and total heap accounting. The main arena backs the single-threaded case, while additional arenas reduce contention for multi-threaded workloads.\n\nThis diagram reduces malloc_state to the fields that matter most when you leak or corrupt allocator metadata.",
        "struct malloc_state {\n    int flags;\n    int have_fastchunks;\n    mfastbinptr fastbinsY[NFASTBINS];\n    mchunkptr top;\n    mchunkptr last_remainder;\n    mchunkptr bins[NBINS * 2 - 2];\n    unsigned int binmap[BINMAPSIZE];\n    struct malloc_state *next;\n    struct malloc_state *next_free;\n    INTERNAL_SIZE_T system_mem;\n    INTERNAL_SIZE_T max_system_mem;\n};\n",
        "The challenge uses the classic unsorted-bin leak pattern to motivate why main_arena matters."
    },
    {
        6,
        "Chapter 6 - Top Chunk",
        "The top chunk is the wilderness at the end of the heap. When bins fail to satisfy a request, malloc() splits space out of the top chunk; when it becomes too small, sysmalloc() grows the heap and updates top again.\n\nThe diagram emphasizes that the top chunk is just another chunk header with privileged position at the heap frontier.",
        "victim = av->top;\nsize = chunksize(victim);\nif ((unsigned long)(size) >= (unsigned long)(nb + MINSIZE)) {\n    remainder = chunk_at_offset(victim, nb);\n    av->top = remainder;\n    set_head(victim, nb | PREV_INUSE);\n    set_head(remainder, (size - nb) | PREV_INUSE);\n}\n",
        "Use the challenge to reason about the top frontier from process memory rather than from allocator source alone."
    },
    {
        7,
        "Chapter 7 - Fastbins",
        "Fastbins are singly linked LIFO lists for recently freed small chunks. They keep PREV_INUSE set in the following chunk to avoid immediate consolidation, which makes them extremely fast and historically very attractive for exploitation.\n\nThe diagram shows the fastbinsY array on the left, chunk nodes on the right, and the safe-linking transform that newer glibc versions apply to fd pointers.",
        "if ((unsigned long)(size) <= (unsigned long)(get_max_fast())) {\n    fb = &fastbin(av, fastbin_index(size));\n    mchunkptr old = *fb;\n    if (__builtin_expect(old == p, 0))\n        malloc_printerr(\"double free or corruption (fasttop)\");\n    p->fd = PROTECT_PTR(&p->fd, old);\n    *fb = p;\n}\n\n#define PROTECT_PTR(pos, ptr) ((__typeof(ptr)) ((((size_t)pos) >> 12) ^ ((size_t)ptr)))\n",
        "The challenge makes the LIFO property explicit by printing the first and second wave of allocations."
    },
    {
        8,
        "Chapter 8 - Tcache",
        "tcache is the allocator's modern per-thread cache. The first chunk on the heap often stores a tcache_perthread_struct, and freed user data becomes a tcache_entry with next and key fields embedded directly in place.\n\nThe top diagram combines the heap-base structure layout with the size-class-to-bin-index arithmetic that explains why request sizes land in specific tcache bins.",
        "static __always_inline void tcache_put(mchunkptr chunk, size_t tc_idx) {\n    tcache_entry *e = (tcache_entry *) chunk2mem(chunk);\n    e->key = tcache_key;\n    e->next = PROTECT_PTR(&e->next, tcache->entries[tc_idx]);\n    tcache->entries[tc_idx] = e;\n    ++(tcache->counts[tc_idx]);\n}\n\nstatic __always_inline void *tcache_get(size_t tc_idx) {\n    tcache_entry *e = tcache->entries[tc_idx];\n    tcache->entries[tc_idx] = REVEAL_PTR(e->next);\n    --(tcache->counts[tc_idx]);\n    e->key = 0;\n    return (void *)e;\n}\n",
        "The Demo tab adds the tcache_put flowchart so you can trace the key assignment and safe-linking steps directly."
    },
    {
        9,
        "Chapter 9 - Unsorted Bin",
        "Freshly consolidated chunks enter the unsorted bin before glibc sorts them into size-specific bins. That makes the unsorted bin both a performance staging area and a rich source of transient pointers back into main_arena.\n\nThe diagram shows the doubly linked list shape and the classic fd/bk leak intuition.",
        "bck = unsorted_chunks(av);\nfwd = bck->fd;\np->bk = bck;\np->fd = fwd;\nbck->fd = p;\nfwd->bk = p;\n",
        "The challenge focuses on the leak side of unsorted-bin placement: getting a libc-range pointer out of fd."
    },
    {
        10,
        "Chapter 10 - Smallbins",
        "Smallbins are fixed-size doubly linked FIFO lists. Unlike fastbins and tcache, chunks are consolidated before they reach a smallbin, so the metadata is tidier and the unlink semantics are easier to reason about.\n\nThe diagram emphasizes the stable bin heads, FIFO order, and the round-trip from unsorted into a smallbin and back out to a request.",
        "if (in_smallbin_range(nb)) {\n    idx = smallbin_index(nb);\n    bin = bin_at(av, idx);\n    if ((victim = last(bin)) != bin) {\n        bck = victim->bk;\n        bin->bk = bck;\n        bck->fd = bin;\n    }\n}\n",
        "Use the challenge to make the round-trip explicit with a single success marker."
    },
    {
        11,
        "Chapter 11 - Largebins",
        "Largebins add size-ordered skip-list style links on top of the ordinary fd/bk ring. That extra ordering metadata speeds best-fit allocation and exposes fd_nextsize / bk_nextsize as high-value corruption targets.\n\nThe diagram focuses on the dual linked-list structure: one list for bin membership, one for size ordering.",
        "victim->fd_nextsize = fwd->fd;\nvictim->bk_nextsize = fwd->fd->bk_nextsize;\nfwd->fd->bk_nextsize = victim;\nvictim->bk_nextsize->fd_nextsize = victim;\n",
        "The challenge keeps the output simple: show the three observed largebin nodes in descending size order."
    },
    {
        12,
        "Chapter 12 - Coalescing",
        "Backward and forward consolidation rebuild larger free chunks out of adjacent free neighbors. That logic is the allocator's answer to fragmentation, and it is also the mechanism that later exploitation techniques try to derail or steer.\n\nThe diagram walks through A, B, and C with arrows showing how free(B) followed by free(A) merges the pair into a larger chunk.",
        "if (!prev_inuse(p)) {\n    prevsize = prev_size(p);\n    size += prevsize;\n    p = chunk_at_offset(p, -((long) prevsize));\n    unlink_chunk(av, p);\n}\nif (!inuse(nextchunk)) {\n    size += chunksize(nextchunk);\n    unlink_chunk(av, nextchunk);\n}\n",
        "The challenge asks you to prove the merged size numerically after the second free."
    },
    {
        13,
        "Chapter 13 - malloc() Path",
        "malloc() starts with request alignment and arena selection, then walks tcache, fastbins, smallbins, unsorted, largebins, and finally sysmalloc(). Understanding that control flow is the bridge between reading glibc and predicting what a live request will do.\n\nThis chapter's top diagram is a node graph that compresses the full allocation path into a readable decision tree.",
        "__libc_malloc(bytes)\n  -> arena_get(av)\n  -> checked_request2size(bytes)\n  -> _int_malloc(av, nb)\n  -> tcache / fastbin / smallbin / unsorted / largebin / sysmalloc\n",
        "The Demo tab redraws the flowchart in a larger format and the challenge asks you to capture the syscall side of a fresh allocation."
    },
    {
        14,
        "Chapter 14 - free() Path",
        "free() converts a user pointer back into a chunk, validates it, and then chooses between tcache insertion, fastbin insertion, consolidation, or munmap for mmapped chunks. Most modern integrity checks live on this path.\n\nThe top node graph tracks the major branches: mem2chunk, sanity checks, tcache, fastbins, coalescing, unsorted insertion, and munmap.",
        "__libc_free(mem)\n  -> mem2chunk(mem)\n  -> if (chunk_is_mmapped) munmap_chunk(p)\n  -> _int_free(av, p, have_lock)\n  -> tcache_put / fastbin / consolidate / unsorted\n",
        "The challenge compares the lightweight tcache path against a heavier non-tcache path by printing two labeled counts."
    },
    {
        15,
        "Chapter 15 - Mitigations",
        "Modern heap exploitation has to respect both allocator-specific checks and system-wide mitigations. Safe-linking, tcache key validation, fastbin top checks, vtable validation, ASLR, PIE, RELRO, NX, and canaries all shape which primitive is still viable.\n\nThe top canvas renders a compact mitigation table with version, location, and bypass condition so the exploit lab badges map back to concrete logic.",
        "/* Representative checks */\nif (__glibc_unlikely(old == p))\n    malloc_printerr(\"double free or corruption (fasttop)\");\nif (__glibc_unlikely(e->key == tcache_key))\n    malloc_printerr(\"double free detected in tcache 2\");\n#define PROTECT_PTR(pos, ptr) ((__typeof(ptr)) ((((size_t)pos) >> 12) ^ ((size_t)ptr)))\nif (vtable != &_IO_file_jumps)\n    _IO_vtable_check();\n",
        "The challenge collects the three mitigation-oriented failure strings most often discussed when teaching modern glibc heap bugs."
    }
};

static const HeapLensChapterContent *heaplens_learning_chapter_lookup(int chapter_id) {
    return chapter_id >= 1 && chapter_id <= (int) G_N_ELEMENTS(heaplens_chapters)
        ? &heaplens_chapters[chapter_id - 1]
        : NULL;
}

static GtkWidget *heaplens_learning_build_tab_label(const char *icon_name, const char *text) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *icon = gtk_image_new_from_icon_name(icon_name);
    GtkWidget *label = gtk_label_new(text);

    gtk_image_set_pixel_size(GTK_IMAGE(icon), 16);
    gtk_box_append(GTK_BOX(box), icon);
    gtk_box_append(GTK_BOX(box), label);
    return box;
}

static GtkWidget *heaplens_learning_build_text_tab(const char *text, gboolean source_view) {
    GtkWidget *scroller = gtk_scrolled_window_new();
    GtkWidget *widget = NULL;

    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(scroller), FALSE);

#if HEAPLENS_HAVE_SOURCE_VIEW
    if (source_view) {
        GtkSourceBuffer *buffer = gtk_source_buffer_new(NULL);
        GtkWidget *view = gtk_source_view_new_with_buffer(buffer);
        gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), TRUE);
        gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
        gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);
        gtk_widget_add_css_class(view, "monospace");
        gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 12);
        gtk_text_view_set_right_margin(GTK_TEXT_VIEW(view), 12);
        gtk_text_view_set_top_margin(GTK_TEXT_VIEW(view), 10);
        gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(view), 10);
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), text ? text : "", -1);
        widget = view;
        g_object_unref(buffer);
    }
#endif

    if (!widget) {
        GtkWidget *view = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
        gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), source_view ? GTK_WRAP_NONE : GTK_WRAP_WORD_CHAR);
        gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), source_view);
        gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 12);
        gtk_text_view_set_right_margin(GTK_TEXT_VIEW(view), 12);
        gtk_text_view_set_top_margin(GTK_TEXT_VIEW(view), 10);
        gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(view), 10);
        if (source_view) {
            gtk_widget_add_css_class(view, "monospace");
        }
        gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)), text ? text : "", -1);
        widget = view;
    }

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), widget);
    gtk_widget_set_vexpand(scroller, TRUE);
    gtk_widget_set_hexpand(scroller, TRUE);
    return scroller;
}

static void heaplens_chapter_flowchart_draw(cairo_t *cr, int width, int chapter_id) {
    static const char *malloc_nodes[] = {
        "malloc(n)", "tcache?", "fastbin?", "unsorted", "sysmalloc", "brk / mmap"
    };
    static const char *free_nodes[] = {
        "free(ptr)", "mem2chunk", "tcache?", "fastbin?", "coalesce", "unsorted / munmap"
    };
    const char *const *nodes = chapter_id == 14 ? free_nodes : malloc_nodes;
    int count = 6;
    int index = 0;
    double x = 16.0;
    double y = 18.0;

    cairo_select_font_face(cr, "JetBrains Mono", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 11.0);

    for (index = 0; index < count; ++index) {
        cairo_set_source_rgb(cr, 0.176, 0.176, 0.176);
        cairo_rectangle(cr, x, y, width - 32.0, 28.0);
        cairo_fill_preserve(cr);
        cairo_set_source_rgb(cr, 0.220, 0.518, 0.894);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);
        cairo_move_to(cr, x + 10.0, y + 18.0);
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_show_text(cr, nodes[index]);
        if (index + 1 < count) {
            cairo_set_source_rgb(cr, 0.666, 0.666, 0.666);
            cairo_move_to(cr, width / 2.0, y + 28.0);
            cairo_line_to(cr, width / 2.0, y + 42.0);
            cairo_stroke(cr);
            cairo_move_to(cr, width / 2.0, y + 42.0);
            cairo_line_to(cr, width / 2.0 - 4.0, y + 36.0);
            cairo_line_to(cr, width / 2.0 + 4.0, y + 36.0);
            cairo_close_path(cr);
            cairo_fill(cr);
        }
        y += 44.0;
    }
}

static void heaplens_chapter_demo_draw(GtkDrawingArea *area,
                                       cairo_t *cr,
                                       int width,
                                       int height,
                                       gpointer user_data) {
    HeapLensChapterDiagramState *state = user_data;
    (void) area;

    cairo_set_source_rgb(cr, 0.145, 0.145, 0.145);
    cairo_paint(cr);

    if (state->chapter_id == 2 || state->chapter_id == 8 || state->chapter_id == 13 || state->chapter_id == 14) {
        heaplens_chapter_flowchart_draw(cr, width, state->chapter_id == 8 ? 13 : state->chapter_id);
        return;
    }

    cairo_select_font_face(cr, "JetBrains Mono", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 11.0);
    cairo_set_source_rgb(cr, 0.666, 0.666, 0.666);
    cairo_move_to(cr, 16.0, height / 2.0);
    cairo_show_text(cr, "The interactive challenge below is the primary hands-on demo for this chapter.");
}

static GtkWidget *heaplens_learning_build_demo_tab(int chapter_id) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    const HeapLensChapterContent *content = heaplens_learning_chapter_lookup(chapter_id);

    gtk_widget_set_hexpand(box, TRUE);
    gtk_widget_set_vexpand(box, TRUE);

    if (chapter_id == 1) {
        GtkWidget *map_panel = heaplens_memory_map_panel_new();
        heaplens_memory_map_panel_load_pid(map_panel, getpid());
        gtk_box_append(GTK_BOX(box), gtk_label_new(content->demo));
        gtk_box_append(GTK_BOX(box), map_panel);
        return box;
    }

    if (chapter_id == 2 || chapter_id == 8 || chapter_id == 13 || chapter_id == 14) {
        GtkWidget *label = gtk_label_new(content->demo);
        GtkWidget *drawing = gtk_drawing_area_new();
        HeapLensChapterDiagramState *state = g_new0(HeapLensChapterDiagramState, 1);

        state->chapter_id = chapter_id;
        gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
        gtk_label_set_wrap(GTK_LABEL(label), TRUE);
        gtk_widget_set_vexpand(drawing, TRUE);
        gtk_widget_set_size_request(drawing, 480, chapter_id == 2 ? 280 : 300);
        gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing), heaplens_chapter_demo_draw, state, g_free);
        gtk_box_append(GTK_BOX(box), label);
        gtk_box_append(GTK_BOX(box), drawing);
        return box;
    }

    gtk_box_append(GTK_BOX(box), gtk_label_new(content->demo));
    return box;
}

static void heaplens_chapter_brk_step(GtkButton *button, gpointer user_data) {
    HeapLensChapterDiagramState *state = user_data;
    static const char *brk_values[] = {
        "brk: 0x555555760000", "brk: 0x555555781000", "brk: 0x555555781000"
    };
    static const char *syscalls[] = {
        "brk called: 0 times", "brk called: 1 time", "brk called: 1 time"
    };
    (void) button;

    state->step_index = (state->step_index + 1) % 3;
    gtk_label_set_text(GTK_LABEL(state->info_primary), brk_values[state->step_index]);
    gtk_label_set_text(GTK_LABEL(state->info_secondary), syscalls[state->step_index]);
    gtk_widget_queue_draw(state->drawing);
}

static void heaplens_chapter_primary_draw(GtkDrawingArea *area,
                                          cairo_t *cr,
                                          int width,
                                          int height,
                                          gpointer user_data) {
    HeapLensChapterDiagramState *state = user_data;
    const HeapLensChapterContent *content = heaplens_learning_chapter_lookup(state->chapter_id);
    (void) area;
    (void) content;

    cairo_set_source_rgb(cr, 0.145, 0.145, 0.145);
    cairo_paint(cr);
    cairo_select_font_face(cr, "JetBrains Mono", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10.0);

    if (state->chapter_id == 1) {
        size_t index = 0;
        guint64 total = 0;
        double y = 12.0;
        state->region_count = 0;
        for (index = 0; index < state->maps.count; ++index) {
            total += state->maps.entries[index].end - state->maps.entries[index].start;
        }
        if (total == 0) {
            return;
        }
        for (index = 0; index < state->maps.count && state->region_count < G_N_ELEMENTS(state->regions); ++index) {
            const HeapLensMemoryMapEntry *entry = &state->maps.entries[index];
            double bar_height = MAX(18.0, (double) (entry->end - entry->start) / (double) total * (height - 24.0));
            char line[256];

            if (strstr(entry->pathname, "[stack]")) {
                cairo_set_source_rgb(cr, 0.384, 0.627, 0.918);
            } else if (strstr(entry->pathname, "libc")) {
                cairo_set_source_rgb(cr, 0.753, 0.380, 0.796);
            } else if (strstr(entry->pathname, "[heap]")) {
                cairo_set_source_rgb(cr, 0.341, 0.890, 0.537);
            } else if (entry->start > 0x00007fffffffffffULL) {
                cairo_set_source_rgb(cr, 0.900, 0.478, 0.388);
            } else {
                cairo_set_source_rgb(cr, 0.400, 0.400, 0.400);
            }

            cairo_rectangle(cr, 70.0, y, 90.0, bar_height);
            cairo_fill(cr);
            cairo_set_source_rgb(cr, 0.666, 0.666, 0.666);
            cairo_move_to(cr, 12.0, y + 12.0);
            g_snprintf(line, sizeof(line), "0x%llx", (unsigned long long) entry->start);
            cairo_show_text(cr, line);
            cairo_move_to(cr, 170.0, y + 12.0);
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_show_text(cr, entry->pathname[0] ? entry->pathname : entry->perms);

            state->regions[state->region_count].y0 = y;
            state->regions[state->region_count].y1 = y + bar_height;
            g_snprintf(state->regions[state->region_count].tooltip,
                       sizeof(state->regions[state->region_count].tooltip),
                       "0x%llx-0x%llx | %zu KB | %s | %s",
                       (unsigned long long) entry->start,
                       (unsigned long long) entry->end,
                       (size_t) ((entry->end - entry->start) / 1024),
                       entry->perms,
                       entry->pathname[0] ? entry->pathname : "<anonymous>");
            ++state->region_count;
            y += bar_height + 2.0;
            if (y > height - 18.0) {
                break;
            }
        }
        return;
    }

    if (state->chapter_id == 2) {
        static const char *labels[] = {
            "[.bss end] ====================| heap start = brk = heap end",
            "[.bss end] == [chunk] =========| brk moves right by 0x21000",
            "[.bss end] == [A] [B] ========| brk unchanged"
        };
        cairo_set_source_rgb(cr, 0.341, 0.890, 0.537);
        cairo_set_line_width(cr, 6.0);
        cairo_move_to(cr, 24.0, height / 2.0);
        cairo_line_to(cr, width - 64.0, height / 2.0);
        cairo_stroke(cr);
        cairo_set_source_rgb(cr, 0.220, 0.518, 0.894);
        cairo_rectangle(cr, 120.0 + state->step_index * 90.0, height / 2.0 - 18.0, 12.0, 36.0);
        cairo_fill(cr);
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_move_to(cr, 24.0, height / 2.0 + 42.0);
        cairo_show_text(cr, labels[state->step_index]);
        return;
    }

    if (state->chapter_id == 4) {
        const char *rows[] = {
            "0x55a000001230  +0   [00 00 00 00 00 00 00 00]  prev_size",
            "0x55a000001238  +8   [31 00 00 00 00 00 00 00]  size",
            "0x55a000001240 +16   [.. .. .. .. .. .. .. ..]  user data",
            "0x55a000001248 +24   [.. .. .. .. .. .. .. ..]  fd",
            "0x55a000001250 +32   [.. .. .. .. .. .. .. ..]  bk",
            "0x55a000001260 +48   [01 00 00 00 00 00 00 00]  next prev_size"
        };
        int index = 0;
        double y = 18.0;
        for (index = 0; index < 6; ++index) {
            cairo_set_source_rgb(cr, 0.176, 0.176, 0.176);
            cairo_rectangle(cr, 12.0, y, width - 24.0, 26.0);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, index == 1 ? 0.976 : 0.341, index == 1 ? 0.941 : 0.890, index == 1 ? 0.419 : 0.537);
            cairo_set_line_width(cr, 1.0);
            cairo_stroke(cr);
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_move_to(cr, 18.0, y + 17.0);
            cairo_show_text(cr, rows[index]);
            y += 30.0;
        }
        cairo_set_source_rgb(cr, 0.976, 0.941, 0.419);
        cairo_move_to(cr, 18.0, y + 16.0);
        cairo_show_text(cr, "0x31 = 0b00110001  -> PREV_INUSE=1 IS_MMAPPED=0 NON_MAIN_ARENA=0 size=0x30");
        cairo_move_to(cr, 18.0, y + 40.0);
        cairo_show_text(cr, "next_chunk = chunk + size      prev_chunk = chunk - prev_size");
        return;
    }

    if (state->chapter_id == 7) {
        int index = 0;
        double y = 20.0;
        const char *slots[] = {"fastbinsY[0]", "fastbinsY[1]", "fastbinsY[2]"};
        for (index = 0; index < 3; ++index) {
            cairo_set_source_rgb(cr, 0.220, 0.220, 0.220);
            cairo_rectangle(cr, 14.0, y, 110.0, 28.0);
            cairo_fill(cr);
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_move_to(cr, 20.0, y + 18.0);
            cairo_show_text(cr, slots[index]);
            cairo_set_source_rgb(cr, 1.000, 0.639, 0.282);
            cairo_rectangle(cr, 180.0, y, 120.0, 28.0);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_stroke(cr);
            cairo_move_to(cr, 190.0, y + 18.0);
            cairo_show_text(cr, index == 0 ? "0x5555001280 -> 0x5555001250" : index == 1 ? "NULL" : "0x5555001300");
            y += 42.0;
        }
        cairo_move_to(cr, 18.0, y + 20.0);
        cairo_set_source_rgb(cr, 0.753, 0.380, 0.796);
        cairo_show_text(cr, "fd = raw_ptr ^ (storage_slot >> 12) after glibc 2.32");
        return;
    }

    if (state->chapter_id == 8) {
        cairo_set_source_rgb(cr, 0.176, 0.176, 0.176);
        cairo_rectangle(cr, 18.0, 18.0, width - 36.0, 150.0);
        cairo_fill_preserve(cr);
        cairo_set_source_rgb(cr, 0.341, 0.890, 0.537);
        cairo_stroke(cr);
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_move_to(cr, 28.0, 42.0);
        cairo_show_text(cr, "tcache_perthread_struct");
        cairo_move_to(cr, 28.0, 70.0);
        cairo_show_text(cr, "counts[0..63]  [0]=0 [1]=2 [2]=0 ...");
        cairo_move_to(cr, 28.0, 96.0);
        cairo_show_text(cr, "entries[0..63] [1]=0x555500001290 -> [chunk] -> [chunk]");
        cairo_move_to(cr, 28.0, 136.0);
        cairo_show_text(cr, "tcache_index(size) = (size - 0x20) / 0x10");
        return;
    }

    if (state->chapter_id == 13 || state->chapter_id == 14) {
        heaplens_chapter_flowchart_draw(cr, width, state->chapter_id);
        return;
    }

    if (state->chapter_id == 15) {
        const char *rows[][4] = {
            {"ASLR", "kernel", "mmap base randomization", "Need leak or brute-force window"},
            {"PIE", "ld.so", "ELF load relocation", "Need binary base leak"},
            {"RELRO", "ld.so", ".got / reloc protection", "Need non-GOT target or partial overwrite"},
            {"NX", "kernel", "page permissions", "Need code reuse or data-only path"},
            {"Safe-linking", "2.32", "_int_free / tcache_put", "Need heap leak or pointer reveal"},
            {"tcache key", "2.29", "tcache_put", "Bypass key reuse / partial overwrite"},
            {"fasttop", "2.26-", "_int_free", "Need head mismatch before second free"},
            {"libio vtable", "2.24", "_IO_vtable_check", "Need valid vtable or reuse path"}
        };
        int index = 0;
        double y = 24.0;
        for (index = 0; index < 8; ++index) {
            cairo_set_source_rgb(cr, 0.176, 0.176, 0.176);
            cairo_rectangle(cr, 12.0, y, width - 24.0, 34.0);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0.220, 0.518, 0.894);
            cairo_stroke(cr);
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_move_to(cr, 18.0, y + 14.0);
            cairo_show_text(cr, rows[index][0]);
            cairo_move_to(cr, 150.0, y + 14.0);
            cairo_show_text(cr, rows[index][1]);
            cairo_move_to(cr, 220.0, y + 14.0);
            cairo_show_text(cr, rows[index][2]);
            cairo_move_to(cr, 18.0, y + 28.0);
            cairo_show_text(cr, rows[index][3]);
            y += 40.0;
        }
        return;
    }

    cairo_set_source_rgb(cr, 0.176, 0.176, 0.176);
    cairo_rectangle(cr, 18.0, 18.0, width - 36.0, height - 36.0);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0.220, 0.518, 0.894);
    cairo_stroke(cr);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_move_to(cr, 26.0, 44.0);
    cairo_show_text(cr, content ? content->title : "Allocator diagram");
    cairo_move_to(cr, 26.0, 74.0);
    cairo_show_text(cr, state->chapter_id == 5 ? "main_arena -> fastbinsY / bins / top / last_remainder" :
                    state->chapter_id == 6 ? "top chunk shrinks on split and regrows after sysmalloc()" :
                    state->chapter_id == 9 ? "unsorted bin: head <-> chunk A <-> chunk B <-> head" :
                    state->chapter_id == 10 ? "smallbin: fixed-size FIFO doubly linked list" :
                    state->chapter_id == 11 ? "largebin: fd/bk plus fd_nextsize/bk_nextsize ordering" :
                    state->chapter_id == 12 ? "A + B merge when PREV_INUSE is cleared and free(A) unlinks B" :
                    "glibc allocator structure overview");
}

static void heaplens_chapter_motion(GtkEventControllerMotion *motion, double x, double y, gpointer user_data) {
    HeapLensChapterDiagramState *state = user_data;
    size_t index = 0;
    (void) motion;
    (void) x;

    if (state->chapter_id != 1) {
        return;
    }
    for (index = 0; index < state->region_count; ++index) {
        if (y >= state->regions[index].y0 && y <= state->regions[index].y1) {
            gtk_widget_set_tooltip_text(state->drawing, state->regions[index].tooltip);
            return;
        }
    }
    gtk_widget_set_tooltip_text(state->drawing, NULL);
}

static GtkWidget *heaplens_learning_build_primary_widget(int chapter_id) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget *drawing = gtk_drawing_area_new();
    GtkWidget *button = NULL;
    GtkEventController *motion = NULL;
    HeapLensChapterDiagramState *state = g_new0(HeapLensChapterDiagramState, 1);

    state->chapter_id = chapter_id;
    state->drawing = drawing;
    if (chapter_id == 1) {
        heaplens_memory_reader_read_maps(getpid(), &state->maps);
        motion = gtk_event_controller_motion_new();
        g_signal_connect(motion, "motion", G_CALLBACK(heaplens_chapter_motion), state);
        gtk_widget_add_controller(drawing, motion);
    }

    gtk_widget_add_css_class(drawing, "chapter-diagram");
    gtk_widget_set_hexpand(drawing, TRUE);
    gtk_widget_set_vexpand(drawing, TRUE);
    gtk_widget_set_size_request(drawing, 640, 360);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing), heaplens_chapter_primary_draw, state, heaplens_chapter_diagram_state_free);

    if (chapter_id == 2) {
        state->info_primary = gtk_label_new("brk: 0x555555760000");
        state->info_secondary = gtk_label_new("brk called: 0 times");
        gtk_label_set_xalign(GTK_LABEL(state->info_primary), 0.0f);
        gtk_label_set_xalign(GTK_LABEL(state->info_secondary), 0.0f);
        gtk_widget_add_css_class(state->info_primary, "accent-text");
        gtk_widget_add_css_class(state->info_secondary, "secondary");
        button = gtk_button_new_with_label("Next Step");
        g_signal_connect(button, "clicked", G_CALLBACK(heaplens_chapter_brk_step), state);
        gtk_box_append(GTK_BOX(box), state->info_primary);
        gtk_box_append(GTK_BOX(box), state->info_secondary);
        gtk_box_append(GTK_BOX(box), button);
    }

    gtk_box_append(GTK_BOX(box), drawing);
    return box;
}

GtkWidget *heaplens_learning_build_chapter_page(int chapter_id) {
    const HeapLensChapterContent *content = heaplens_learning_chapter_lookup(chapter_id);
    const Challenge *challenge = heaplens_challenge_get(chapter_id);
    GtkWidget *root = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    GtkWidget *top = heaplens_learning_build_primary_widget(chapter_id);
    GtkWidget *notebook = gtk_notebook_new();
    GtkWidget *theory = heaplens_learning_build_text_tab(content ? content->theory : "", FALSE);
    GtkWidget *source = heaplens_learning_build_text_tab(content ? content->source : "", TRUE);
    GtkWidget *demo = heaplens_learning_build_demo_tab(chapter_id);
    GtkWidget *challenge_widget = heaplens_challenge_engine_new(chapter_id,
                                                                challenge,
                                                                heaplens_challenge_language(chapter_id),
                                                                heaplens_challenge_solution(chapter_id));

    gtk_widget_set_hexpand(root, TRUE);
    gtk_widget_set_vexpand(root, TRUE);
    gtk_paned_set_wide_handle(GTK_PANED(root), TRUE);
    gtk_paned_set_position(GTK_PANED(root), 400);
    gtk_paned_set_resize_start_child(GTK_PANED(root), TRUE);
    gtk_paned_set_shrink_start_child(GTK_PANED(root), FALSE);
    gtk_paned_set_resize_end_child(GTK_PANED(root), TRUE);
    gtk_paned_set_shrink_end_child(GTK_PANED(root), FALSE);
    gtk_widget_set_hexpand(notebook, TRUE);
    gtk_widget_set_vexpand(notebook, TRUE);
    gtk_paned_set_start_child(GTK_PANED(root), top);
    gtk_paned_set_end_child(GTK_PANED(root), notebook);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), theory, heaplens_learning_build_tab_label("accessories-text-editor-symbolic", "Theory"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), source, heaplens_learning_build_tab_label("text-x-source-symbolic", "Source"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), demo, heaplens_learning_build_tab_label("media-playback-start-symbolic", "Demo"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), challenge_widget, heaplens_learning_build_tab_label("flag-symbolic", "Challenge"));
    return root;
}
