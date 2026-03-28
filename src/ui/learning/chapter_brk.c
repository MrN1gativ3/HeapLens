#include "chapter_brk.h"

#include <stdio.h>

#include "../../core/heap_parser.h"
#include "chapter_common.h"

typedef struct {
    GtkWidget *result;
    GtkAdjustment *adjustment;
} HeapLensBrkDemoState;

static void heaplens_brk_demo_refresh(GtkAdjustment *adjustment, gpointer user_data) {
    HeapLensBrkDemoState *state = user_data;
    size_t request = (size_t) gtk_adjustment_get_value(adjustment);
    size_t chunk = heaplens_request_to_chunk_size(request);
    char buffer[256];

    snprintf(buffer,
             sizeof(buffer),
             "malloc(%zu) -> chunk size %#zx -> likely %s path",
             request,
             chunk,
             request >= 128 * 1024 ? "mmap" : "brk/top-chunk");
    gtk_label_set_text(GTK_LABEL(state->result), buffer);
}

static GtkWidget *heaplens_brk_demo_widget(void) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkAdjustment *adjustment = gtk_adjustment_new(32.0, 1.0, 262144.0, 16.0, 64.0, 0.0);
    GtkWidget *scale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, adjustment);
    GtkWidget *result = gtk_label_new(NULL);
    HeapLensBrkDemoState *state = g_new0(HeapLensBrkDemoState, 1);

    state->result = result;
    state->adjustment = adjustment;
    g_signal_connect(adjustment, "value-changed", G_CALLBACK(heaplens_brk_demo_refresh), state);
    g_object_set_data_full(G_OBJECT(box), "heaplens-brk-demo-state", state, g_free);
    gtk_label_set_xalign(GTK_LABEL(result), 0.0f);
    gtk_box_append(GTK_BOX(box), gtk_label_new("Request size simulator"));
    gtk_box_append(GTK_BOX(box), scale);
    gtk_box_append(GTK_BOX(box), result);
    heaplens_brk_demo_refresh(adjustment, state);
    return box;
}

GtkWidget *heaplens_chapter_brk_new(void) {
    static const char *topics[] = {
        "The program break marks the boundary between heap-backed virtual memory and unmapped space.",
        "glibc does not call brk() for every malloc(). It carves smaller allocations out of a top chunk until more pages are needed.",
        "When the requested space does not fit the current top chunk, sysmalloc() grows the heap or falls back to mmap().",
        "HeapLens ties brk activity back to allocator stages so learners can see exactly which allocation caused growth."
    };

    return heaplens_learning_build_chapter_page(
        "Chapter 2 - Program Break and brk/sbrk",
        "This chapter focuses on the heap's moving frontier. The small simulator below shows how requested user sizes become internal chunk sizes and when glibc typically switches away from the brk path.",
        topics,
        G_N_ELEMENTS(topics),
        "Interactive demo: move the slider to approximate whether a request would be served from the current top chunk or promoted into the mmap path.",
        "brk(0)              -> current break\nbrk(new_break)      -> expanded heap end",
        heaplens_brk_demo_widget());
}
