#include "heap_visualizer.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    GtkWidget *drawing;
    GtkWidget *details;
    HeapLensHeapSnapshot snapshot;
    int selected_index;
} HeapLensHeapVisualizerState;

static void heaplens_heap_visualizer_state_free(gpointer data) {
    HeapLensHeapVisualizerState *state = data;
    if (!state) {
        return;
    }
    heaplens_heap_snapshot_free(&state->snapshot);
    g_free(state);
}

static void heaplens_heap_visualizer_copy_snapshot(HeapLensHeapSnapshot *dst,
                                                   const HeapLensHeapSnapshot *src) {
    size_t index = 0;

    memset(dst, 0, sizeof(*dst));
    if (!src) {
        return;
    }

    *dst = *src;
    dst->chunks = NULL;
    if (src->chunk_count == 0) {
        return;
    }

    dst->chunks = g_new0(HeapLensChunkInfo, src->chunk_count);
    for (index = 0; index < src->chunk_count; ++index) {
        dst->chunks[index] = src->chunks[index];
    }
}

static void heaplens_heap_visualizer_update_details(HeapLensHeapVisualizerState *state) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->details));
    GString *text = g_string_new("");

    if (!state || state->snapshot.chunk_count == 0) {
        gtk_text_buffer_set_text(buffer, "No heap snapshot loaded.", -1);
        g_string_free(text, TRUE);
        return;
    }

    if (state->selected_index >= 0 && (size_t) state->selected_index < state->snapshot.chunk_count) {
        const HeapLensChunkInfo *chunk = &state->snapshot.chunks[state->selected_index];
        g_string_append_printf(text,
                               "Chunk @ %#llx\n"
                               "Decoded size: %#zx\n"
                               "Usable size: %#zx\n"
                               "Flags: P=%d M=%d A=%d\n"
                               "fd raw: %#llx\n"
                               "fd decoded: %#llx\n"
                               "bk: %#llx\n"
                               "Bin: %s\n"
                               "Validation: %s\n",
                               (unsigned long long) chunk->address,
                               chunk->decoded_size,
                               chunk->usable_size,
                               chunk->prev_inuse,
                               chunk->is_mmapped,
                               chunk->non_main_arena,
                               (unsigned long long) (uintptr_t) chunk->raw.fd,
                               (unsigned long long) chunk->decoded_fd,
                               (unsigned long long) (uintptr_t) chunk->raw.bk,
                               chunk->bin_name,
                               chunk->is_valid ? "OK" : chunk->validation_error);
    } else {
        size_t index = 0;
        for (index = 0; index < state->snapshot.chunk_count; ++index) {
            const HeapLensChunkInfo *chunk = &state->snapshot.chunks[index];
            g_string_append_printf(text,
                                   "[%02zu] %#llx size=%#zx usable=%#zx flags=%c%c%c %s\n",
                                   index,
                                   (unsigned long long) chunk->address,
                                   chunk->decoded_size,
                                   chunk->usable_size,
                                   chunk->prev_inuse ? 'P' : '-',
                                   chunk->is_mmapped ? 'M' : '-',
                                   chunk->non_main_arena ? 'A' : '-',
                                   chunk->is_valid ? "" : chunk->validation_error);
        }
    }

    gtk_text_buffer_set_text(buffer, text->str, -1);
    g_string_free(text, TRUE);
}

static void heaplens_heap_visualizer_draw(GtkDrawingArea *area,
                                          cairo_t *cr,
                                          int width,
                                          int height,
                                          gpointer user_data) {
    HeapLensHeapVisualizerState *state = user_data;
    double x = 8.0;
    double y = 24.0;
    double usable_width = width - 16.0;
    size_t index = 0;
    size_t total_size = 0;
    (void) area;

    cairo_set_source_rgb(cr, 0.10, 0.14, 0.18);
    cairo_paint(cr);

    if (!state || state->snapshot.chunk_count == 0) {
        cairo_set_source_rgb(cr, 0.85, 0.88, 0.92);
        cairo_move_to(cr, 12.0, 24.0);
        cairo_show_text(cr, "Heap snapshot will appear here after stepping a demo.");
        return;
    }

    if (state->snapshot.tcache_address != 0) {
        cairo_set_source_rgb(cr, 0.40, 0.67, 0.95);
        cairo_rectangle(cr, x, y, usable_width, 18.0);
        cairo_fill(cr);
        cairo_set_source_rgb(cr, 0.08, 0.11, 0.15);
        cairo_move_to(cr, x + 6.0, y + 13.0);
        cairo_show_text(cr, "tcache_perthread_struct");
        y += 28.0;
    }

    for (index = 0; index < state->snapshot.chunk_count; ++index) {
        total_size += state->snapshot.chunks[index].decoded_size;
    }
    if (total_size == 0) {
        total_size = 1;
    }

    for (index = 0; index < state->snapshot.chunk_count; ++index) {
        const HeapLensChunkInfo *chunk = &state->snapshot.chunks[index];
        double bar_width = usable_width * ((double) chunk->decoded_size / (double) total_size);
        if (bar_width < 14.0) {
            bar_width = 14.0;
        }

        if (!chunk->is_valid) {
            cairo_set_source_rgb(cr, 0.90, 0.35, 0.35);
        } else if (chunk->is_mmapped) {
            cairo_set_source_rgb(cr, 0.95, 0.76, 0.38);
        } else if (!chunk->prev_inuse) {
            cairo_set_source_rgb(cr, 0.40, 0.80, 0.66);
        } else {
            cairo_set_source_rgb(cr, 0.33, 0.60, 0.82);
        }

        cairo_rectangle(cr, x, y, bar_width, 26.0);
        cairo_fill_preserve(cr);
        cairo_set_source_rgb(cr, 0.06, 0.07, 0.10);
        cairo_set_line_width(cr, index == (size_t) state->selected_index ? 3.0 : 1.0);
        cairo_stroke(cr);

        cairo_move_to(cr, x + 6.0, y + 17.0);
        cairo_show_text(cr, chunk->bin_name);

        x += bar_width + 4.0;
        if (x + 40.0 > width) {
            x = 8.0;
            y += 34.0;
        }
        if (y + 32.0 > height) {
            break;
        }
    }
}

static void heaplens_heap_visualizer_clicked(GtkGestureClick *gesture,
                                             int n_press,
                                             double x,
                                             double y,
                                             gpointer user_data) {
    HeapLensHeapVisualizerState *state = user_data;
    int width = gtk_widget_get_width(state->drawing);
    double cursor_x = 8.0;
    double cursor_y = 24.0;
    double usable_width = width - 16.0;
    size_t total_size = 0;
    size_t index = 0;
    (void) gesture;
    (void) n_press;

    if (!state || state->snapshot.chunk_count == 0) {
        return;
    }

    for (index = 0; index < state->snapshot.chunk_count; ++index) {
        total_size += state->snapshot.chunks[index].decoded_size;
    }
    if (total_size == 0) {
        total_size = 1;
    }

    if (state->snapshot.tcache_address != 0) {
        cursor_y += 28.0;
    }

    for (index = 0; index < state->snapshot.chunk_count; ++index) {
        const HeapLensChunkInfo *chunk = &state->snapshot.chunks[index];
        double bar_width = usable_width * ((double) chunk->decoded_size / (double) total_size);
        if (bar_width < 14.0) {
            bar_width = 14.0;
        }

        if (x >= cursor_x && x <= cursor_x + bar_width && y >= cursor_y && y <= cursor_y + 26.0) {
            state->selected_index = (int) index;
            heaplens_heap_visualizer_update_details(state);
            gtk_widget_queue_draw(state->drawing);
            return;
        }

        cursor_x += bar_width + 4.0;
        if (cursor_x + 40.0 > width) {
            cursor_x = 8.0;
            cursor_y += 34.0;
        }
    }
}

GtkWidget *heaplens_heap_visualizer_new(void) {
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *title = gtk_label_new("Real-time Heap Visualizer");
    GtkWidget *drawing = gtk_drawing_area_new();
    GtkWidget *details_scroller = gtk_scrolled_window_new();
    GtkWidget *details = gtk_text_view_new();
    GtkGesture *click = gtk_gesture_click_new();
    HeapLensHeapVisualizerState *state = g_new0(HeapLensHeapVisualizerState, 1);

    state->drawing = drawing;
    state->details = details;
    state->selected_index = -1;
    g_object_set_data_full(G_OBJECT(root), "heaplens-heap-viz-state", state, heaplens_heap_visualizer_state_free);

    gtk_widget_add_css_class(root, "card");
    gtk_widget_add_css_class(title, "title");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_widget_set_hexpand(drawing, TRUE);
    gtk_widget_set_vexpand(drawing, TRUE);
    gtk_widget_set_size_request(drawing, 320, 320);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(details), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(details), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(details), TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(details_scroller), details);
    gtk_widget_set_vexpand(details_scroller, TRUE);

    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing), heaplens_heap_visualizer_draw, state, NULL);
    g_signal_connect(click, "pressed", G_CALLBACK(heaplens_heap_visualizer_clicked), state);
    gtk_widget_add_controller(drawing, GTK_EVENT_CONTROLLER(click));

    gtk_box_append(GTK_BOX(root), title);
    gtk_box_append(GTK_BOX(root), drawing);
    gtk_box_append(GTK_BOX(root), details_scroller);
    return root;
}

void heaplens_heap_visualizer_set_snapshot(GtkWidget *panel, const HeapLensHeapSnapshot *snapshot) {
    HeapLensHeapVisualizerState *state = g_object_get_data(G_OBJECT(panel), "heaplens-heap-viz-state");

    if (!panel || !state) {
        return;
    }

    heaplens_heap_snapshot_free(&state->snapshot);
    heaplens_heap_visualizer_copy_snapshot(&state->snapshot, snapshot);
    state->selected_index = state->snapshot.chunk_count > 0 ? 0 : -1;
    heaplens_heap_visualizer_update_details(state);
    gtk_widget_queue_draw(state->drawing);
}

void heaplens_heap_visualizer_clear(GtkWidget *panel) {
    HeapLensHeapVisualizerState *state = g_object_get_data(G_OBJECT(panel), "heaplens-heap-viz-state");
    if (!panel || !state) {
        return;
    }
    heaplens_heap_snapshot_free(&state->snapshot);
    state->selected_index = -1;
    heaplens_heap_visualizer_update_details(state);
    gtk_widget_queue_draw(state->drawing);
}
