#include "dbg_heap.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../core/tcache_parser.h"
#include "dbg_common.h"

typedef struct {
    double y;
    double height;
} HeapLensChunkLayout;

typedef struct {
    GtkWidget *arena_dropdown;
    GtkWidget *drawing;
    GtkWidget *dump_view;
    GtkWidget *dump_popover;
    GtkWidget *tcache_box;
    GtkWidget *fastbin_box;
    GtkWidget *unsorted_box;
    GtkWidget *small_large_box;
    GtkWidget *arena_fields;
    HeapLensTargetProcess *process;
    HeapLensHeapSnapshot snapshot;
    HeapLensTcacheSnapshot tcache;
    gboolean have_tcache;
    HeapLensChunkLayout *layouts;
    size_t layout_count;
    int highlighted_index;
} HeapLensDbgHeapState;

static HeapLensDbgHeapState *heaplens_dbg_heap_state(GtkWidget *window) {
    return window ? g_object_get_data(G_OBJECT(window), "heaplens-dbg-heap-state") : NULL;
}

static void heaplens_dbg_heap_snapshot_copy(HeapLensHeapSnapshot *dst, const HeapLensHeapSnapshot *src) {
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

static void heaplens_dbg_heap_state_free(gpointer data) {
    HeapLensDbgHeapState *state = data;

    if (!state) {
        return;
    }

    heaplens_heap_snapshot_free(&state->snapshot);
    g_free(state->layouts);
    g_free(state);
}

static void heaplens_dbg_heap_clear_box(GtkWidget *box) {
    GtkWidget *child = gtk_widget_get_first_child(box);

    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(box), child);
        child = next;
    }
}

static int heaplens_dbg_heap_find_index(const HeapLensDbgHeapState *state, uint64_t address) {
    size_t index = 0;

    if (!state) {
        return -1;
    }

    for (index = 0; index < state->snapshot.chunk_count; ++index) {
        if (state->snapshot.chunks[index].address == address) {
            return (int) index;
        }
    }

    return -1;
}

static gboolean heaplens_dbg_heap_is_tcache_chunk(const HeapLensDbgHeapState *state, uint64_t address) {
    size_t bin_index = 0;
    size_t entry_index = 0;

    if (!state || !state->have_tcache) {
        return FALSE;
    }

    for (bin_index = 0; bin_index < TCACHE_MAX_BINS; ++bin_index) {
        const HeapLensTcacheBinSnapshot *bin = &state->tcache.bins[bin_index];
        for (entry_index = 0; entry_index < bin->depth; ++entry_index) {
            if (bin->entries[entry_index].address - 0x10 == address) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

static gboolean heaplens_dbg_heap_is_fastbin_chunk(const HeapLensDbgHeapState *state, uint64_t address) {
    int idx = heaplens_dbg_heap_find_index(state, address);

    if (!state || idx < 0) {
        return FALSE;
    }

    return strstr(state->snapshot.chunks[idx].bin_name, "fast") != NULL;
}

static const GdkRGBA *heaplens_dbg_heap_color(const HeapLensDbgHeapState *state,
                                              const HeapLensChunkInfo *chunk,
                                              int index,
                                              GdkRGBA *fill,
                                              GdkRGBA *stroke) {
    static const GdkRGBA green = {0.341, 0.890, 0.537, 1.0};
    static const GdkRGBA red = {1.000, 0.482, 0.388, 1.0};
    static const GdkRGBA yellow = {0.976, 0.941, 0.419, 1.0};
    static const GdkRGBA purple = {0.753, 0.380, 0.796, 1.0};
    static const GdkRGBA orange = {1.000, 0.639, 0.282, 1.0};
    static const GdkRGBA blue = {0.384, 0.627, 0.918, 1.0};
    static const GdkRGBA accent = {0.208, 0.518, 0.894, 1.0};
    const GdkRGBA *base = &green;

    if (!chunk) {
        return &green;
    }

    if (index == state->highlighted_index) {
        base = &yellow;
    } else if (!chunk->is_valid) {
        base = &red;
    } else if (state->snapshot.have_arena && chunk->address == (uint64_t) (uintptr_t) state->snapshot.arena.top) {
        base = &accent;
    } else if (heaplens_dbg_heap_is_tcache_chunk(state, chunk->address)) {
        base = &purple;
    } else if (heaplens_dbg_heap_is_fastbin_chunk(state, chunk->address)) {
        base = &orange;
    } else if (!chunk->prev_inuse) {
        base = &blue;
    }

    *stroke = *base;
    *fill = *base;
    if (base == &green) {
        fill->alpha = 0.40;
    } else {
        fill->alpha = 0.30;
    }
    return base;
}

static void heaplens_dbg_heap_draw_arrows(HeapLensDbgHeapState *state,
                                          cairo_t *cr,
                                          double left,
                                          double width) {
    size_t index = 0;

    if (!state || !state->layouts || state->layout_count != state->snapshot.chunk_count) {
        return;
    }

    for (index = 0; index < state->snapshot.chunk_count; ++index) {
        const HeapLensChunkInfo *chunk = &state->snapshot.chunks[index];
        int fd_index = heaplens_dbg_heap_find_index(state, chunk->decoded_fd);
        int bk_index = heaplens_dbg_heap_find_index(state, chunk->decoded_bk);
        GdkRGBA fill;
        GdkRGBA stroke;
        double start_x = left + width;
        double start_y = state->layouts[index].y + state->layouts[index].height / 2.0;

        heaplens_dbg_heap_color(state, chunk, (int) index, &fill, &stroke);
        cairo_set_source_rgba(cr, stroke.red, stroke.green, stroke.blue, 0.85);
        cairo_set_line_width(cr, 1.0);

        if (fd_index >= 0) {
            double end_y = state->layouts[fd_index].y + state->layouts[fd_index].height / 2.0;
            cairo_move_to(cr, start_x, start_y);
            cairo_curve_to(cr, start_x + 18.0, start_y, start_x + 18.0, end_y, start_x, end_y);
            cairo_stroke(cr);
        }

        if (bk_index >= 0) {
            double end_y = state->layouts[bk_index].y + state->layouts[bk_index].height / 2.0;
            cairo_move_to(cr, left, start_y);
            cairo_curve_to(cr, left - 14.0, start_y, left - 14.0, end_y, left, end_y);
            cairo_stroke(cr);
        }
    }
}

static void heaplens_dbg_heap_draw(GtkDrawingArea *area,
                                   cairo_t *cr,
                                   int width,
                                   int height,
                                   gpointer user_data) {
    HeapLensDbgHeapState *state = user_data;
    const double address_width = 120.0;
    const double bar_left = address_width;
    const double bar_width = MAX(60.0, width - address_width - 16.0);
    double y = 12.0;
    size_t index = 0;

    cairo_set_source_rgb(cr, 0.145, 0.145, 0.145);
    cairo_paint(cr);

    if (!state || state->snapshot.chunk_count == 0) {
        cairo_set_source_rgb(cr, 0.666, 0.666, 0.666);
        cairo_move_to(cr, 14.0, 24.0);
        cairo_show_text(cr, "No heap snapshot available.");
        return;
    }

    g_free(state->layouts);
    state->layouts = g_new0(HeapLensChunkLayout, state->snapshot.chunk_count);
    state->layout_count = state->snapshot.chunk_count;

    cairo_select_font_face(cr, "JetBrains Mono", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10.0);

    for (index = 0; index < state->snapshot.chunk_count; ++index) {
        const HeapLensChunkInfo *chunk = &state->snapshot.chunks[index];
        GdkRGBA fill;
        GdkRGBA stroke;
        char label[256];
        char address[48];
        double bar_height = MAX(18.0, 18.0 + log2((double) MAX(chunk->decoded_size, (size_t) 0x20)) * 3.0);

        if (y + bar_height > height - 12.0) {
            break;
        }

        state->layouts[index].y = y;
        state->layouts[index].height = bar_height;
        heaplens_dbg_heap_color(state, chunk, (int) index, &fill, &stroke);

        cairo_set_source_rgb(cr, 0.400, 0.400, 0.400);
        g_snprintf(address, sizeof(address), "0x%llx", (unsigned long long) chunk->address);
        cairo_move_to(cr, 10.0, y + bar_height * 0.65);
        cairo_show_text(cr, address);

        cairo_rectangle(cr, bar_left, y, bar_width, bar_height);
        cairo_set_source_rgba(cr, fill.red, fill.green, fill.blue, fill.alpha);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, stroke.red, stroke.green, stroke.blue, 1.0);
        cairo_set_line_width(cr, 1.0);
        if (!chunk->is_valid) {
            static const double dashes[] = {4.0, 3.0};
            cairo_set_dash(cr, dashes, 2, 0.0);
        }
        cairo_stroke(cr);
        cairo_set_dash(cr, NULL, 0, 0.0);

        g_snprintf(label,
                   sizeof(label),
                   "sz:0x%zx | P:%d M:%d A:%d | fd:0x%llx bk:0x%llx",
                   chunk->decoded_size,
                   chunk->prev_inuse ? 1 : 0,
                   chunk->is_mmapped ? 1 : 0,
                   chunk->non_main_arena ? 1 : 0,
                   (unsigned long long) chunk->decoded_fd,
                   (unsigned long long) chunk->decoded_bk);
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_move_to(cr, bar_left + 8.0, y + MIN(bar_height - 6.0, 14.0));
        cairo_show_text(cr, label);

        y += bar_height + 8.0;
    }

    heaplens_dbg_heap_draw_arrows(state, cr, bar_left, bar_width);
}

static void heaplens_dbg_heap_show_dump(HeapLensDbgHeapState *state, const HeapLensChunkInfo *chunk, double x, double y) {
    guint8 bytes[64];
    char *text = NULL;
    GdkRectangle rect;

    if (!state || !chunk || !state->process || state->process->reader.mem_fd < 0) {
        return;
    }

    memset(bytes, 0, sizeof(bytes));
    if (!heaplens_memory_reader_read_fully(&state->process->reader, chunk->address, bytes, sizeof(bytes))) {
        gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->dump_view)),
                                 "Unable to read chunk bytes.",
                                 -1);
    } else {
        text = heaplens_dbg_format_hex_dump(bytes, sizeof(bytes), chunk->address);
        gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->dump_view)), text, -1);
        g_free(text);
    }

    rect.x = (int) x;
    rect.y = (int) y;
    rect.width = 2;
    rect.height = 2;
    gtk_popover_set_pointing_to(GTK_POPOVER(state->dump_popover), &rect);
    gtk_popover_popup(GTK_POPOVER(state->dump_popover));
}

static void heaplens_dbg_heap_refresh_fields(HeapLensDbgHeapState *state) {
    char buffer[512];

    if (!state) {
        return;
    }

    if (state->snapshot.have_arena) {
        g_snprintf(buffer,
                   sizeof(buffer),
                   "top: 0x%llx    last_remainder: 0x%llx\nsystem_mem: %zu bytes    max_system_mem: %zu bytes",
                   (unsigned long long) (uintptr_t) state->snapshot.arena.top,
                   (unsigned long long) (uintptr_t) state->snapshot.arena.last_remainder,
                   state->snapshot.arena.system_mem,
                   state->snapshot.arena.max_system_mem);
    } else {
        g_snprintf(buffer, sizeof(buffer), "Arena metadata unavailable for this libc build.");
    }

    gtk_label_set_text(GTK_LABEL(state->arena_fields), buffer);
}

static void heaplens_dbg_heap_select_address(GtkButton *button, gpointer user_data) {
    HeapLensDbgHeapState *state = user_data;
    guint64 *address_ptr = g_object_get_data(G_OBJECT(button), "heaplens-address");
    uint64_t address = address_ptr ? *address_ptr : 0;
    int index = heaplens_dbg_heap_find_index(state, address);

    if (!state || index < 0) {
        return;
    }

    state->highlighted_index = index;
    gtk_widget_queue_draw(state->drawing);
}

static void heaplens_dbg_heap_add_link_button(GtkWidget *box, const char *label, uint64_t address, HeapLensDbgHeapState *state) {
    GtkWidget *button = gtk_button_new_with_label(label);
    guint64 *address_ptr = g_new(guint64, 1);

    gtk_widget_add_css_class(button, "monospace");
    *address_ptr = address;
    g_object_set_data_full(G_OBJECT(button), "heaplens-address", address_ptr, g_free);
    g_signal_connect(button, "clicked", G_CALLBACK(heaplens_dbg_heap_select_address), state);
    gtk_box_append(GTK_BOX(box), button);
}

static void heaplens_dbg_heap_refresh_bins(HeapLensDbgHeapState *state) {
    size_t index = 0;

    if (!state) {
        return;
    }

    heaplens_dbg_heap_clear_box(state->tcache_box);
    heaplens_dbg_heap_clear_box(state->fastbin_box);
    heaplens_dbg_heap_clear_box(state->unsorted_box);
    heaplens_dbg_heap_clear_box(state->small_large_box);

    if (state->have_tcache) {
        for (index = 0; index < TCACHE_MAX_BINS; ++index) {
            const HeapLensTcacheBinSnapshot *bin = &state->tcache.bins[index];
            char row[512];
            size_t entry_index = 0;
            GString *line = NULL;

            if (bin->count == 0) {
                continue;
            }

            line = g_string_new("");
            g_string_append_printf(line, "bin[0x%zx] count=%u ", 0x20 + index * 0x10, bin->count);
            for (entry_index = 0; entry_index < bin->depth; ++entry_index) {
                g_string_append_printf(line,
                                       "%s0x%llx",
                                       entry_index == 0 ? "" : " -> ",
                                       (unsigned long long) (bin->entries[entry_index].address - 0x10));
            }
            g_snprintf(row, sizeof(row), "%s", line->str);
            heaplens_dbg_heap_add_link_button(state->tcache_box,
                                              row,
                                              bin->entries[0].address - 0x10,
                                              state);
            g_string_free(line, TRUE);
        }
    } else {
        gtk_box_append(GTK_BOX(state->tcache_box), gtk_label_new("No tcache structure detected."));
    }

    for (index = 0; index < state->snapshot.chunk_count; ++index) {
        const HeapLensChunkInfo *chunk = &state->snapshot.chunks[index];
        char row[256];

        if (!chunk->prev_inuse) {
            g_snprintf(row,
                       sizeof(row),
                       "free 0x%llx -> fd 0x%llx bk 0x%llx",
                       (unsigned long long) chunk->address,
                       (unsigned long long) chunk->decoded_fd,
                       (unsigned long long) chunk->decoded_bk);
            heaplens_dbg_heap_add_link_button(state->unsorted_box, row, chunk->address, state);
        }
        if (heaplens_dbg_heap_is_fastbin_chunk(state, chunk->address)) {
            g_snprintf(row,
                       sizeof(row),
                       "fastbin 0x%llx -> fd 0x%llx",
                       (unsigned long long) chunk->address,
                       (unsigned long long) chunk->decoded_fd);
            heaplens_dbg_heap_add_link_button(state->fastbin_box, row, chunk->address, state);
        }
        if (chunk->decoded_size >= 0x90 && chunk->decoded_size <= 0x410 && !chunk->prev_inuse) {
            g_snprintf(row, sizeof(row), "bin chunk 0x%llx size=0x%zx", (unsigned long long) chunk->address, chunk->decoded_size);
            heaplens_dbg_heap_add_link_button(state->small_large_box, row, chunk->address, state);
        }
    }
}

static void heaplens_dbg_heap_click(GtkGestureClick *gesture,
                                    int n_press,
                                    double x,
                                    double y,
                                    gpointer user_data) {
    HeapLensDbgHeapState *state = user_data;
    size_t index = 0;
    (void) gesture;
    (void) n_press;

    if (!state || !state->layouts) {
        return;
    }

    for (index = 0; index < state->snapshot.chunk_count; ++index) {
        if (y >= state->layouts[index].y && y <= state->layouts[index].y + state->layouts[index].height) {
            state->highlighted_index = (int) index;
            gtk_widget_queue_draw(state->drawing);
            heaplens_dbg_heap_show_dump(state, &state->snapshot.chunks[index], x, y);
            return;
        }
    }
}

GtkWidget *heaplens_dbg_heap_new(GtkWindow *parent) {
    GtkWidget *window = gtk_window_new();
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *arena_dropdown = gtk_drop_down_new(G_LIST_MODEL(gtk_string_list_new((const char *[]) {"main_arena", NULL})), NULL);
    GtkWidget *drawing = gtk_drawing_area_new();
    GtkWidget *bins_expander = gtk_expander_new("Bin State");
    GtkWidget *bins_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget *tcache_expander = gtk_expander_new("tcachebins");
    GtkWidget *fast_expander = gtk_expander_new("fastbins");
    GtkWidget *unsorted_expander = gtk_expander_new("unsorted bin");
    GtkWidget *small_large_expander = gtk_expander_new("small/large bins");
    GtkWidget *tcache_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *fast_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *unsorted_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *small_large_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *arena_expander = gtk_expander_new("Arena Fields");
    GtkWidget *arena_fields = gtk_label_new("Arena metadata unavailable.");
    GtkWidget *dump_popover = gtk_popover_new();
    GtkWidget *dump_scroller = gtk_scrolled_window_new();
    GtkWidget *dump_view = gtk_text_view_new();
    GtkGesture *click = gtk_gesture_click_new();
    HeapLensDbgHeapState *state = g_new0(HeapLensDbgHeapState, 1);

    heaplens_dbg_window_init(GTK_WINDOW(window), parent, "Heap", "heap", 480, 700);
    gtk_widget_add_css_class(root, "panel-flat");
    gtk_widget_set_margin_top(root, 8);
    gtk_widget_set_margin_bottom(root, 8);
    gtk_widget_set_margin_start(root, 8);
    gtk_widget_set_margin_end(root, 8);
    gtk_widget_add_css_class(arena_dropdown, "monospace");
    gtk_widget_set_size_request(arena_dropdown, 180, -1);
    gtk_widget_set_hexpand(drawing, TRUE);
    gtk_widget_set_vexpand(drawing, TRUE);
    gtk_widget_set_size_request(drawing, 440, 480);
    gtk_widget_add_css_class(drawing, "heap-canvas");
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing), heaplens_dbg_heap_draw, state, NULL);
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), GDK_BUTTON_PRIMARY);
    g_signal_connect(click, "pressed", G_CALLBACK(heaplens_dbg_heap_click), state);
    gtk_widget_add_controller(drawing, GTK_EVENT_CONTROLLER(click));

    gtk_text_view_set_editable(GTK_TEXT_VIEW(dump_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(dump_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(dump_view), TRUE);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(dump_scroller), TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(dump_scroller), dump_view);
    gtk_widget_set_size_request(dump_scroller, 420, 180);
    gtk_popover_set_child(GTK_POPOVER(dump_popover), dump_scroller);
    gtk_popover_set_has_arrow(GTK_POPOVER(dump_popover), TRUE);
    gtk_widget_set_parent(dump_popover, drawing);

    gtk_expander_set_expanded(GTK_EXPANDER(bins_expander), FALSE);
    gtk_expander_set_expanded(GTK_EXPANDER(arena_expander), FALSE);
    gtk_expander_set_expanded(GTK_EXPANDER(tcache_expander), FALSE);
    gtk_expander_set_expanded(GTK_EXPANDER(fast_expander), FALSE);
    gtk_expander_set_expanded(GTK_EXPANDER(unsorted_expander), FALSE);
    gtk_expander_set_expanded(GTK_EXPANDER(small_large_expander), FALSE);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(dump_scroller), TRUE);

    gtk_expander_set_child(GTK_EXPANDER(tcache_expander), tcache_box);
    gtk_expander_set_child(GTK_EXPANDER(fast_expander), fast_box);
    gtk_expander_set_child(GTK_EXPANDER(unsorted_expander), unsorted_box);
    gtk_expander_set_child(GTK_EXPANDER(small_large_expander), small_large_box);
    gtk_box_append(GTK_BOX(bins_box), tcache_expander);
    gtk_box_append(GTK_BOX(bins_box), fast_expander);
    gtk_box_append(GTK_BOX(bins_box), unsorted_expander);
    gtk_box_append(GTK_BOX(bins_box), small_large_expander);
    gtk_expander_set_child(GTK_EXPANDER(bins_expander), bins_box);

    gtk_label_set_xalign(GTK_LABEL(arena_fields), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(arena_fields), TRUE);
    gtk_expander_set_child(GTK_EXPANDER(arena_expander), arena_fields);

    gtk_box_append(GTK_BOX(root), arena_dropdown);
    gtk_box_append(GTK_BOX(root), drawing);
    gtk_box_append(GTK_BOX(root), bins_expander);
    gtk_box_append(GTK_BOX(root), arena_expander);
    gtk_window_set_child(GTK_WINDOW(window), root);

    state->arena_dropdown = arena_dropdown;
    state->drawing = drawing;
    state->dump_view = dump_view;
    state->dump_popover = dump_popover;
    state->tcache_box = tcache_box;
    state->fastbin_box = fast_box;
    state->unsorted_box = unsorted_box;
    state->small_large_box = small_large_box;
    state->arena_fields = arena_fields;
    state->highlighted_index = -1;
    g_object_set_data_full(G_OBJECT(window), "heaplens-dbg-heap-state", state, heaplens_dbg_heap_state_free);
    return window;
}

void heaplens_dbg_heap_set_snapshot(GtkWidget *window,
                                    HeapLensTargetProcess *process,
                                    const HeapLensHeapSnapshot *snapshot) {
    HeapLensDbgHeapState *state = heaplens_dbg_heap_state(window);

    if (!state) {
        return;
    }

    heaplens_heap_snapshot_free(&state->snapshot);
    memset(&state->tcache, 0, sizeof(state->tcache));
    state->process = process;
    state->have_tcache = FALSE;
    heaplens_dbg_heap_snapshot_copy(&state->snapshot, snapshot);
    if (process && process->reader.mem_fd >= 0 && snapshot && snapshot->tcache_address &&
        heaplens_tcache_parser_read(&process->reader, snapshot->tcache_address, &state->tcache)) {
        state->have_tcache = TRUE;
    }

    heaplens_dbg_heap_refresh_bins(state);
    heaplens_dbg_heap_refresh_fields(state);
    gtk_widget_queue_draw(state->drawing);
}

void heaplens_dbg_heap_clear(GtkWidget *window) {
    HeapLensDbgHeapState *state = heaplens_dbg_heap_state(window);

    if (!state) {
        return;
    }

    state->process = NULL;
    state->highlighted_index = -1;
    memset(&state->tcache, 0, sizeof(state->tcache));
    state->have_tcache = FALSE;
    heaplens_heap_snapshot_free(&state->snapshot);
    heaplens_dbg_heap_refresh_bins(state);
    heaplens_dbg_heap_refresh_fields(state);
    gtk_widget_queue_draw(state->drawing);
}

gboolean heaplens_dbg_heap_search(GtkWidget *window, const char *query) {
    HeapLensDbgHeapState *state = heaplens_dbg_heap_state(window);
    size_t index = 0;

    if (!state || !query || !query[0]) {
        return FALSE;
    }

    for (index = 0; index < state->snapshot.chunk_count; ++index) {
        char address[32];
        const HeapLensChunkInfo *chunk = &state->snapshot.chunks[index];

        g_snprintf(address, sizeof(address), "0x%llx", (unsigned long long) chunk->address);
        if (g_strrstr(address, query) || g_strrstr(chunk->bin_name, query)) {
            state->highlighted_index = (int) index;
            gtk_widget_queue_draw(state->drawing);
            return TRUE;
        }
    }

    return FALSE;
}
