#include "tab_learning.h"

#include "learning/chapter_arena.h"
#include "learning/chapter_brk.h"
#include "learning/chapter_chunk.h"
#include "learning/chapter_consolidate.h"
#include "learning/chapter_fastbin.h"
#include "learning/chapter_free_path.h"
#include "learning/chapter_largebin.h"
#include "learning/chapter_malloc_path.h"
#include "learning/chapter_mitigations.h"
#include "learning/chapter_mmap.h"
#include "learning/chapter_smallbin.h"
#include "learning/chapter_tcache.h"
#include "learning/chapter_topchunk.h"
#include "learning/chapter_unsorted.h"
#include "learning/chapter_vmem.h"

typedef struct {
    const char *name;
    const char *title;
    GtkWidget *(*factory)(void);
} HeapLensLearningChapter;

static void heaplens_tab_learning_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    GtkStack *stack = user_data;
    const char *page_name = NULL;
    (void) box;

    if (!row || !stack) {
        return;
    }

    page_name = g_object_get_data(G_OBJECT(row), "heaplens-page-name");
    if (page_name) {
        gtk_stack_set_visible_child_name(stack, page_name);
    }
}

GtkWidget *heaplens_tab_learning_new(void) {
    static const HeapLensLearningChapter chapters[] = {
        {"vmem", "Virtual Memory", heaplens_chapter_vmem_new},
        {"brk", "Program Break", heaplens_chapter_brk_new},
        {"mmap", "mmap Allocations", heaplens_chapter_mmap_new},
        {"chunk", "malloc_chunk", heaplens_chapter_chunk_new},
        {"arena", "Arena and malloc_state", heaplens_chapter_arena_new},
        {"topchunk", "Top Chunk", heaplens_chapter_topchunk_new},
        {"fastbin", "Fastbins", heaplens_chapter_fastbin_new},
        {"tcache", "Tcache", heaplens_chapter_tcache_new},
        {"unsorted", "Unsorted Bin", heaplens_chapter_unsorted_new},
        {"smallbin", "Smallbins", heaplens_chapter_smallbin_new},
        {"largebin", "Largebins", heaplens_chapter_largebin_new},
        {"consolidate", "Coalescing", heaplens_chapter_consolidate_new},
        {"malloc-path", "malloc Path", heaplens_chapter_malloc_path_new},
        {"free-path", "free Path", heaplens_chapter_free_path_new},
        {"mitigations", "Mitigations", heaplens_chapter_mitigations_new}
    };
    GtkWidget *root = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    GtkWidget *sidebar_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *sidebar_title = gtk_label_new("Chapters");
    GtkWidget *sidebar_scroller = gtk_scrolled_window_new();
    GtkWidget *sidebar = gtk_list_box_new();
    GtkWidget *stack = gtk_stack_new();
    size_t index = 0;

    gtk_widget_set_margin_top(root, 8);
    gtk_widget_set_margin_bottom(root, 8);
    gtk_widget_set_margin_start(root, 8);
    gtk_widget_set_margin_end(root, 8);
    gtk_paned_set_wide_handle(GTK_PANED(root), TRUE);
    gtk_paned_set_position(GTK_PANED(root), 320);
    gtk_paned_set_resize_start_child(GTK_PANED(root), FALSE);
    gtk_paned_set_shrink_start_child(GTK_PANED(root), FALSE);
    gtk_paned_set_resize_end_child(GTK_PANED(root), TRUE);
    gtk_paned_set_shrink_end_child(GTK_PANED(root), FALSE);
    gtk_widget_add_css_class(sidebar_panel, "panel");
    gtk_widget_add_css_class(sidebar_title, "panel-heading");
    gtk_widget_add_css_class(sidebar, "chapter-list");
    gtk_widget_set_hexpand(sidebar_panel, FALSE);
    gtk_widget_set_vexpand(sidebar_panel, TRUE);
    gtk_widget_set_hexpand(stack, TRUE);
    gtk_widget_set_vexpand(stack, TRUE);
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(sidebar), GTK_SELECTION_SINGLE);
    gtk_widget_set_size_request(sidebar_panel, 280, -1);
    gtk_label_set_xalign(GTK_LABEL(sidebar_title), 0.0f);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(sidebar_scroller), FALSE);
    gtk_widget_set_vexpand(sidebar_scroller, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sidebar_scroller), sidebar);
    g_signal_connect(sidebar, "row-selected", G_CALLBACK(heaplens_tab_learning_selected), stack);

    for (index = 0; index < G_N_ELEMENTS(chapters); ++index) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new(chapters[index].title);
        GtkWidget *page = chapters[index].factory();
        gtk_widget_add_css_class(label, "technique-row");
        gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        g_object_set_data_full(G_OBJECT(row), "heaplens-page-name", g_strdup(chapters[index].name), g_free);
        gtk_list_box_append(GTK_LIST_BOX(sidebar), row);
        gtk_stack_add_named(GTK_STACK(stack), page, chapters[index].name);
    }

    gtk_box_append(GTK_BOX(sidebar_panel), sidebar_title);
    gtk_box_append(GTK_BOX(sidebar_panel), sidebar_scroller);
    gtk_paned_set_start_child(GTK_PANED(root), sidebar_panel);
    gtk_paned_set_end_child(GTK_PANED(root), stack);

    {
        GtkListBoxRow *first = gtk_list_box_get_row_at_index(GTK_LIST_BOX(sidebar), 0);
        if (first) {
            gtk_list_box_select_row(GTK_LIST_BOX(sidebar), first);
        }
    }

    return root;
}
