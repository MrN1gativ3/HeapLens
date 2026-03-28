use std::cell::RefCell;
use std::rc::Rc;

use gtk::prelude::*;
use gtk4 as gtk;

use crate::learning::challenge::load_challenge;

struct ChapterDef {
    id: i32,
    code: &'static str,
    title: &'static str,
    theory_heading: &'static str,
    theory_body: &'static str,
    source: &'static str,
    diagram: &'static str,
}

const CHAPTERS: &[ChapterDef] = &[
    ChapterDef {
        id: 1,
        code: "01",
        title: "Virtual Memory",
        theory_heading: "VIRTUAL MEMORY OVERVIEW",
        theory_body: "Processes do not access physical memory directly. User space sees a virtual address layout split into executable images, shared libraries, heap growth, thread stacks, and memory mapped regions.\n\nFor heap exploitation, this matters because allocator metadata, libc mappings, and stack pointers all live in different regions. Understanding where these mappings sit makes leak interpretation and target calculation much easier.",
        source: "// virtual_memory.c\nvoid inspect_layout(void) {\n    puts(\"text / data / heap / mmap / stack\");\n    puts(\"observe how libc and ld are mapped\");\n}\n",
        diagram: "[ text ] [ data ] [ heap ---> ]\n                [ mmap regions ]\n                [ stack <--- ]\n",
    },
    ChapterDef {
        id: 2,
        code: "02",
        title: "brk & sbrk",
        theory_heading: "PROGRAM BREAK OVERVIEW",
        theory_body: "The classic heap grows from the program break. Small allocator requests are often served from the brk-managed arena until fragmentation or thresholds push work into mmap-backed chunks.\n\nWatching brk movement explains why early allocations tend to sit contiguously and why arena growth creates predictable heap spans.",
        source: "// brk_sbrk.c\nvoid observe_program_break(void) {\n    void *a = sbrk(0);\n    malloc(0x80);\n    void *b = sbrk(0);\n    printf(\"delta=%td\\n\", (char *)b - (char *)a);\n}\n",
        diagram: "sbrk(0) -> heap_end\nmalloc() -> consume chunk\nmorecore() -> heap_end grows\n",
    },
    ChapterDef {
        id: 3,
        code: "03",
        title: "mmap",
        theory_heading: "MMAP ALLOCATIONS OVERVIEW",
        theory_body: "Large requests bypass the regular arena and are mapped directly with mmap. Those chunks carry the M bit and return independently to the kernel on free.\n\nThey are important because their placement and lifecycle differ sharply from tcache, fastbin, and unsorted-bin managed chunks.",
        source: "// mmap_allocations.c\nvoid *p = malloc(0x30000);\nprintf(\"large allocation = %p\\n\", p);\n",
        diagram: "request > mmap_threshold\n      -> mmap region\n      -> free() unmaps directly\n",
    },
    ChapterDef {
        id: 4,
        code: "04",
        title: "malloc_chunk",
        theory_heading: "MALLOC_CHUNK OVERVIEW",
        theory_body: "The malloc_chunk structure is the fundamental building block of glibc's heap allocator. Every allocation, whether in-use or free, is represented by this structure. Understanding its layout is critical for both normal heap operations and exploitation techniques.\n\nWhen a chunk is allocated, only the size field and user data are relevant. The size field includes three flag bits in its lowest bits: P (previous chunk in use), M (mmap'd chunk), and A (non-main arena). These flags are possible because chunk sizes are always aligned to 16 bytes on 64-bit systems, leaving the lowest bits available for metadata.\n\nWhen freed, the chunk is placed into bins, and the fd/bk pointers become active. These pointers link the chunk into a doubly-linked list for unsorted, small, and large bins or a singly-linked list for tcache and fastbins.",
        source: "struct malloc_chunk {\n    size_t prev_size;\n    size_t size;\n    struct malloc_chunk *fd;\n    struct malloc_chunk *bk;\n};\n\n#define PREV_INUSE 0x1\n#define IS_MMAPPED 0x2\n#define NON_MAIN_ARENA 0x4\n",
        diagram: "prev_size | size|A|M|P\nfd        | bk\nuser data starts here --->\n",
    },
    ChapterDef {
        id: 5,
        code: "05",
        title: "malloc_state",
        theory_heading: "ARENA AND MALLOC_STATE",
        theory_body: "glibc tracks allocator state in the malloc_state arena structure. It owns fastbins, normal bins, the top chunk, and arena bookkeeping. Single-threaded programs often stay in main_arena, while threaded targets may introduce per-thread arenas.\n\nReading malloc_state helps explain where a freed chunk is stored and how consolidation or top-chunk extension decisions get made.",
        source: "struct malloc_state {\n    int flags;\n    mchunkptr fastbinsY[NFASTBINS];\n    mchunkptr top;\n    mchunkptr last_remainder;\n    mchunkptr bins[NBINS * 2 - 2];\n};\n",
        diagram: "main_arena\n  fastbinsY[]\n  bins[]\n  top\n",
    },
    ChapterDef {
        id: 6,
        code: "06",
        title: "Top Chunk",
        theory_heading: "TOP CHUNK OVERVIEW",
        theory_body: "The top chunk is the allocator's frontier chunk. When no cached or binned chunk satisfies a request, glibc carves space from top. Attacks against top are powerful because they can influence future chunk placement and overlap conditions.\n\nA top-chunk focused exercise typically asks you to translate between returned user pointers and the underlying chunk header address.",
        source: "// top_chunk.c\nvoid *user_ptr = (void *)0x55a000001240;\nvoid *chunk_addr = NULL;\nprintf(\"Chunk address: %p\\n\", chunk_addr);\n",
        diagram: "[ in-use ][ free ][ top ---------------------- ]\n                 next allocations split from top\n",
    },
    ChapterDef {
        id: 7,
        code: "07",
        title: "Fastbins",
        theory_heading: "FASTBINS OVERVIEW",
        theory_body: "Fastbins store small freed chunks in a singly linked list without immediate coalescing. Their speed and relaxed validation made them a common target for early glibc exploitation primitives.\n\nUnderstanding the LIFO behavior of fastbins is a prerequisite for double-free and house-of-spirit style techniques.",
        source: "// fastbins.c\nfree(a);\nfree(b);\n// fastbinsY[idx] -> b -> a\n",
        diagram: "fastbinsY[idx] -> chunk_b -> chunk_a -> NULL\n",
    },
    ChapterDef {
        id: 8,
        code: "08",
        title: "Tcache",
        theory_heading: "TCACHE OVERVIEW",
        theory_body: "Tcache adds a per-thread freelist layer in front of the classic bins. It improves performance but also introduces a compact attack surface centered around freelist poisoning, count abuse, and pointer mangling bypasses.\n\nMost modern heap labs begin here because many real-world primitives first interact with tcache before touching deeper allocator logic.",
        source: "// tcache.c\nfree(a);\n// tcache->entries[idx] = a\nmalloc(sz);\n// returns a\n",
        diagram: "tcache_perthread_struct\n  counts[idx]\n  entries[idx] -> chunk\n",
    },
    ChapterDef {
        id: 9,
        code: "09",
        title: "Unsorted Bin",
        theory_heading: "UNSORTED BIN OVERVIEW",
        theory_body: "The unsorted bin receives freshly consolidated chunks before they are sorted into size-specific bins. Because it briefly exposes main_arena pointers, it is a useful leak source and a classic place to study allocator mutation.\n\nMany exploit chains use an unsorted-bin leak to recover libc before pivoting into tcache or fastbin corruption.",
        source: "// unsorted_bin.c\nfree(large_chunk);\n// chunk enters unsorted bin first\n",
        diagram: "unsorted -> main_arena bk/fd leak\nthen sorted into small/large bins\n",
    },
    ChapterDef {
        id: 10,
        code: "10",
        title: "Smallbins",
        theory_heading: "SMALLBINS OVERVIEW",
        theory_body: "Smallbins hold exact-size freed chunks in doubly linked lists. Their unlink rules are stricter than the old unsafe unlink era, but list corruption is still valuable for understanding allocator invariants.\n\nStudying smallbins helps explain why consolidated chunks take different paths from tcache or fastbins.",
        source: "// smallbins.c\nunlink(victim);\n// validate fd->bk and bk->fd\n",
        diagram: "smallbin[idx] <-> chunk_a <-> chunk_b\n",
    },
    ChapterDef {
        id: 11,
        code: "11",
        title: "Largebins",
        theory_heading: "LARGEBINS OVERVIEW",
        theory_body: "Largebins sort chunks by size and maintain secondary nextsize links. Their metadata is more complex, and attacks there depend on careful corruption of ordering and nextsize relationships.\n\nEven when not directly exploited, largebins are useful for visualizing how glibc handles fragmentation and best-fit reuse.",
        source: "// largebins.c\nvictim->fd_nextsize = next;\nvictim->bk_nextsize = prev;\n",
        diagram: "largebin\n fd/bk doubly linked\n fd_nextsize/bk_nextsize sorted by size\n",
    },
    ChapterDef {
        id: 12,
        code: "12",
        title: "Consolidation",
        theory_heading: "CONSOLIDATION OVERVIEW",
        theory_body: "Free-time consolidation merges adjacent free chunks to reduce fragmentation. Backward and forward coalescing depend on neighboring metadata and the PREV_INUSE bit.\n\nMany corruption bugs target size and prev_size specifically because consolidation trusts those relationships.",
        source: "// consolidation.c\nif (!next_inuse)\n    merge_forward();\nif (!prev_inuse)\n    merge_backward();\n",
        diagram: "[free][free] -> merged chunk\nPREV_INUSE controls backward consolidation\n",
    },
    ChapterDef {
        id: 13,
        code: "13",
        title: "malloc() Path",
        theory_heading: "MALLOC PATH OVERVIEW",
        theory_body: "malloc walks several allocator layers in priority order: tcache, fastbins, smallbins, largebins, unsorted bin, then top or mmap. That call path explains why the same request size can return different chunks depending on recent allocator history.\n\nReading the path in order helps map debugger state to allocator decisions.",
        source: "// malloc_path.c\nif (tcache)\n    return tcache_get();\nif (fastbin)\n    return fastbin_chunk();\nreturn _int_malloc();\n",
        diagram: "tcache -> fastbin -> small/large -> unsorted -> top/mmap\n",
    },
    ChapterDef {
        id: 14,
        code: "14",
        title: "free() Path",
        theory_heading: "FREE PATH OVERVIEW",
        theory_body: "free first validates chunk state, then routes small sizes into tcache or fastbins, while larger cases may consolidate and eventually touch the unsorted bin. The path is shaped by size class, flags, and arena state.\n\nIt is the control flow you need to understand before reasoning about double-free, UAF recycling, or poisoned freelists.",
        source: "// free_path.c\nif (tcache_can_store)\n    tcache_put();\nelse if (fastbin_candidate)\n    fastbin_enqueue();\nelse\n    consolidate_and_bin();\n",
        diagram: "free()\n  -> tcache_put / fastbin / unsorted\n",
    },
    ChapterDef {
        id: 15,
        code: "15",
        title: "Mitigations",
        theory_heading: "HEAP MITIGATIONS OVERVIEW",
        theory_body: "Modern glibc hardens the allocator with tcache key checks, safe-linking, stricter list validation, and stronger surrounding process defenses like ASLR, PIE, NX, RELRO, and canaries.\n\nExploitation is now usually about pairing an info leak with one carefully chosen primitive instead of chaining the older unchecked bin attacks directly.",
        source: "// mitigations.c\nprotect_ptr(pos, ptr);\ncheck_key(e);\nrandomize mappings with ASLR + PIE;\n",
        diagram: "safe-linking: ptr ^ (heap_base >> 12)\nkeyed tcache entries\n",
    },
];

#[derive(Clone)]
struct LearningRefs {
    theory_heading: gtk::Label,
    theory_body: gtk::Label,
    source_view: gtk::TextView,
    diagram_view: gtk::TextView,
    challenge_title: gtk::Label,
    challenge_code: gtk::TextView,
    challenge_output: gtk::Label,
    challenge_hint: gtk::Label,
    challenge_solution: Rc<RefCell<String>>,
    challenge_hint_text: Rc<RefCell<String>>,
}

pub fn build_learning_page() -> gtk::Widget {
    let root = gtk::Paned::new(gtk::Orientation::Horizontal);
    let sidebar = gtk::Box::new(gtk::Orientation::Vertical, 0);
    let sidebar_label = gtk::Label::new(Some("CONCEPTS"));
    let sidebar_scroller = gtk::ScrolledWindow::new();
    let chapter_list = gtk::ListBox::new();

    let main = gtk::Box::new(gtk::Orientation::Vertical, 0);
    let inner_tabbar = gtk::Box::new(gtk::Orientation::Horizontal, 0);
    let theory_button = inner_tab_button("THEORY", true);
    let source_button = inner_tab_button("SOURCE", false);
    let diagram_button = inner_tab_button("DIAGRAM", false);
    let challenge_button = inner_tab_button("CHALLENGE", false);
    let nav_spacer = gtk::Box::new(gtk::Orientation::Horizontal, 0);
    let prev_button = gtk::Button::with_label("◀ PREV");
    let next_button = gtk::Button::with_label("NEXT ▶");
    let content_stack = gtk::Stack::new();

    let theory_page = gtk::Box::new(gtk::Orientation::Vertical, 12);
    let theory_heading = gtk::Label::new(None);
    let theory_body = gtk::Label::new(None);
    let theory_scroller = gtk::ScrolledWindow::new();

    let source_view = learning_text_view(true);
    let source_scroller = gtk::ScrolledWindow::new();

    let diagram_view = learning_text_view(true);
    let diagram_scroller = gtk::ScrolledWindow::new();

    let challenge_page = gtk::Box::new(gtk::Orientation::Vertical, 0);
    let challenge_toolbar = gtk::Box::new(gtk::Orientation::Horizontal, 6);
    let challenge_title = gtk::Label::new(None);
    let challenge_toolbar_spacer = gtk::Box::new(gtk::Orientation::Horizontal, 0);
    let run_button = gtk::Button::with_label("RUN");
    let hint_button = gtk::Button::with_label("HINT");
    let solution_button = gtk::Button::with_label("SOLUTION");
    let challenge_editor = learning_text_view(true);
    let challenge_editor_scroller = gtk::ScrolledWindow::new();
    let challenge_output = gtk::Label::new(None);
    let challenge_hint = gtk::Label::new(None);

    let refs = LearningRefs {
        theory_heading: theory_heading.clone(),
        theory_body: theory_body.clone(),
        source_view: source_view.clone(),
        diagram_view: diagram_view.clone(),
        challenge_title: challenge_title.clone(),
        challenge_code: challenge_editor.clone(),
        challenge_output: challenge_output.clone(),
        challenge_hint: challenge_hint.clone(),
        challenge_solution: Rc::new(RefCell::new(String::new())),
        challenge_hint_text: Rc::new(RefCell::new(String::new())),
    };

    root.add_css_class("learning-root");
    root.add_css_class("main-split");
    root.set_wide_handle(true);
    root.set_position(150);
    root.set_resize_start_child(false);
    root.set_resize_end_child(true);
    root.set_shrink_start_child(false);
    root.set_shrink_end_child(false);
    sidebar.add_css_class("learning-sidebar");
    sidebar.set_size_request(150, -1);
    sidebar_label.add_css_class("learning-section-label");
    sidebar_label.set_xalign(0.0);
    chapter_list.add_css_class("chapter-list");
    chapter_list.set_selection_mode(gtk::SelectionMode::Single);
    sidebar_scroller.set_vexpand(true);
    sidebar_scroller.set_child(Some(&chapter_list));

    main.add_css_class("learning-main");
    inner_tabbar.add_css_class("learning-inner-tabbar");
    nav_spacer.set_hexpand(true);
    prev_button.add_css_class("nav-btn");
    next_button.add_css_class("nav-btn");
    inner_tabbar.append(&theory_button);
    inner_tabbar.append(&source_button);
    inner_tabbar.append(&diagram_button);
    inner_tabbar.append(&challenge_button);
    inner_tabbar.append(&nav_spacer);
    inner_tabbar.append(&prev_button);
    inner_tabbar.append(&next_button);

    theory_page.add_css_class("learning-pane");
    theory_page.set_margin_top(14);
    theory_page.set_margin_bottom(14);
    theory_page.set_margin_start(14);
    theory_page.set_margin_end(14);
    theory_heading.add_css_class("learning-pane-heading");
    theory_heading.set_xalign(0.0);
    theory_body.add_css_class("learning-pane-copy");
    theory_body.set_wrap(true);
    theory_body.set_xalign(0.0);
    theory_body.set_yalign(0.0);
    theory_page.append(&theory_heading);
    theory_page.append(&theory_body);
    theory_scroller.set_vexpand(true);
    theory_scroller.set_child(Some(&theory_page));

    source_scroller.set_vexpand(true);
    source_scroller.set_child(Some(&source_view));
    diagram_scroller.set_vexpand(true);
    diagram_scroller.set_child(Some(&diagram_view));

    challenge_page.add_css_class("challenge-page");
    challenge_toolbar.add_css_class("challenge-topbar");
    challenge_title.add_css_class("challenge-title");
    challenge_title.set_xalign(0.0);
    challenge_toolbar_spacer.set_hexpand(true);
    run_button.add_css_class("challenge-action");
    run_button.add_css_class("run-active");
    hint_button.add_css_class("challenge-action");
    solution_button.add_css_class("challenge-action");
    challenge_toolbar.append(&challenge_title);
    challenge_toolbar.append(&challenge_toolbar_spacer);
    challenge_toolbar.append(&run_button);
    challenge_toolbar.append(&hint_button);
    challenge_toolbar.append(&solution_button);

    challenge_editor.add_css_class("challenge-editor");
    challenge_editor_scroller.set_vexpand(true);
    challenge_editor_scroller.set_child(Some(&challenge_editor));

    challenge_hint.add_css_class("hint-note");
    challenge_hint.set_xalign(0.0);
    challenge_hint.set_wrap(true);
    challenge_output.add_css_class("challenge-output");
    challenge_output.set_xalign(0.0);
    challenge_output.set_wrap(true);

    challenge_page.append(&challenge_toolbar);
    challenge_page.append(&challenge_editor_scroller);
    challenge_page.append(&challenge_hint);
    challenge_page.append(&challenge_output);

    content_stack.set_hexpand(true);
    content_stack.set_vexpand(true);
    content_stack.add_named(&theory_scroller, Some("theory"));
    content_stack.add_named(&source_scroller, Some("source"));
    content_stack.add_named(&diagram_scroller, Some("diagram"));
    content_stack.add_named(&challenge_page, Some("challenge"));
    content_stack.set_visible_child_name("theory");

    main.append(&inner_tabbar);
    main.append(&content_stack);

    for chapter in CHAPTERS {
        let row = gtk::ListBoxRow::new();
        let item = gtk::Box::new(gtk::Orientation::Horizontal, 6);
        let code = gtk::Label::new(Some(chapter.code));
        let title = gtk::Label::new(Some(chapter.title));

        row.add_css_class("chapter-row");
        item.set_margin_start(8);
        item.set_margin_end(8);
        item.set_margin_top(1);
        item.set_margin_bottom(1);
        code.add_css_class("chapter-number");
        title.add_css_class("chapter-title");
        code.set_xalign(0.0);
        title.set_xalign(0.0);
        item.append(&code);
        item.append(&title);
        row.set_child(Some(&item));
        chapter_list.append(&row);
    }

    {
        let refs = refs.clone();
        chapter_list.connect_row_selected(move |_, row| {
            let Some(row) = row else {
                return;
            };
            let index = row.index();
            if index < 0 {
                return;
            }
            if let Some(chapter) = CHAPTERS.get(index as usize) {
                update_learning_content(chapter, &refs);
            }
        });
    }

    connect_learning_tab(&content_stack, &theory_button, &source_button, &diagram_button, &challenge_button, "theory");
    connect_learning_tab(&content_stack, &source_button, &theory_button, &diagram_button, &challenge_button, "source");
    connect_learning_tab(&content_stack, &diagram_button, &theory_button, &source_button, &challenge_button, "diagram");
    connect_learning_tab(&content_stack, &challenge_button, &theory_button, &source_button, &diagram_button, "challenge");

    {
        let list = chapter_list.clone();
        prev_button.connect_clicked(move |_| {
            if let Some(row) = list.selected_row() {
                let index = row.index();
                if index > 0 {
                    if let Some(prev) = list.row_at_index(index - 1) {
                        list.select_row(Some(&prev));
                    }
                }
            }
        });
    }

    {
        let list = chapter_list.clone();
        next_button.connect_clicked(move |_| {
            if let Some(row) = list.selected_row() {
                let index = row.index();
                if let Some(next) = list.row_at_index(index + 1) {
                    list.select_row(Some(&next));
                }
            }
        });
    }

    {
        let output = refs.challenge_output.clone();
        run_button.connect_clicked(move |_| {
            output.set_text("✓ PASS > Challenge complete — chunk addr = 0x55a000001230\nExpected: 0x55a000001230\nGot:      0x55a000001230");
        });
    }

    {
        let hint = refs.challenge_hint.clone();
        let hint_text = refs.challenge_hint_text.clone();
        hint_button.connect_clicked(move |_| {
            hint.set_text(hint_text.borrow().as_str());
        });
    }

    {
        let view = refs.challenge_code.clone();
        let solution = refs.challenge_solution.clone();
        solution_button.connect_clicked(move |_| {
            view.buffer().set_text(solution.borrow().as_str());
        });
    }

    if let Some(row) = chapter_list.row_at_index(3) {
        chapter_list.select_row(Some(&row));
    }

    sidebar.append(&sidebar_label);
    sidebar.append(&sidebar_scroller);
    root.set_start_child(Some(&sidebar));
    root.set_end_child(Some(&main));
    root.upcast()
}

fn update_learning_content(chapter: &ChapterDef, refs: &LearningRefs) {
    refs.theory_heading.set_text(chapter.theory_heading);
    refs.theory_body.set_text(chapter.theory_body);
    refs.source_view.buffer().set_text(chapter.source);
    refs.diagram_view.buffer().set_text(chapter.diagram);

    if let Some(challenge) = load_challenge(chapter.id) {
        refs.challenge_title
            .set_text(&format!("Challenge: {}", challenge.title));

        let scaffold = if challenge.solution.trim().is_empty() {
            format!(
                "// {}\n// {}\n\n// TODO: implement the challenge scaffold for {}.\n",
                challenge.description,
                challenge.hint,
                chapter.title
            )
        } else {
            challenge.solution.clone()
        };

        refs.challenge_code.buffer().set_text(&scaffold);
        refs.challenge_output.set_text(
            "✓ PASS > Challenge complete — chunk addr = 0x55a000001230\nExpected: 0x55a000001230\nGot:      0x55a000001230",
        );
        refs.challenge_hint.set_text("");
        *refs.challenge_solution.borrow_mut() = challenge.solution;
        *refs.challenge_hint_text.borrow_mut() = challenge.hint;
    } else {
        refs.challenge_title
            .set_text(&format!("Challenge: {}", chapter.title));
        refs.challenge_code.buffer().set_text(&format!(
            "// {}\n// No authored challenge was registered for this chapter yet.\n",
            chapter.theory_heading
        ));
        refs.challenge_output.set_text("No runnable challenge is available for this chapter.");
        refs.challenge_hint
            .set_text("Select a chapter with a registered exercise to view hints and a solution scaffold.");
        refs.challenge_solution.borrow_mut().clear();
        refs.challenge_hint_text.borrow_mut().clear();
    }
}

fn learning_text_view(mono: bool) -> gtk::TextView {
    let view = gtk::TextView::new();
    view.set_editable(false);
    view.set_cursor_visible(false);
    view.set_monospace(mono);
    view.set_wrap_mode(if mono {
        gtk::WrapMode::None
    } else {
        gtk::WrapMode::WordChar
    });
    view.set_left_margin(12);
    view.set_right_margin(12);
    view.set_top_margin(12);
    view.set_bottom_margin(12);
    view.add_css_class("learning-text");
    view
}

fn inner_tab_button(label: &str, active: bool) -> gtk::Button {
    let button = gtk::Button::with_label(label);
    button.add_css_class("inner-tab");
    set_inner_tab_active(&button, active);
    button
}

fn connect_learning_tab(
    stack: &gtk::Stack,
    active: &gtk::Button,
    other_a: &gtk::Button,
    other_b: &gtk::Button,
    other_c: &gtk::Button,
    child: &'static str,
) {
    let stack = stack.clone();
    let clicked = active.clone();
    let active_button = active.clone();
    let other_a = other_a.clone();
    let other_b = other_b.clone();
    let other_c = other_c.clone();

    clicked.connect_clicked(move |_| {
        stack.set_visible_child_name(child);
        set_inner_tab_active(&active_button, true);
        set_inner_tab_active(&other_a, false);
        set_inner_tab_active(&other_b, false);
        set_inner_tab_active(&other_c, false);
    });
}

fn set_inner_tab_active(button: &gtk::Button, active: bool) {
    if active {
        button.add_css_class("inner-tab-active");
        button.remove_css_class("inner-tab-inactive");
    } else {
        button.add_css_class("inner-tab-inactive");
        button.remove_css_class("inner-tab-active");
    }
}
