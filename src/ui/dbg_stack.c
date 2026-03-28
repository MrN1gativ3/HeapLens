#include "dbg_stack.h"

#include <stdio.h>
#include <string.h>

#include "../core/symbol_resolver.h"
#include "dbg_common.h"

#define HEAPLENS_STACK_ROWS 64
#define HEAPLENS_STACK_FRAMES 16

typedef struct {
    uint64_t address;
    uint64_t value;
    char annotation[160];
} HeapLensStackEntry;

typedef struct {
    uint64_t rbp;
    uint64_t ret_addr;
    int start_row;
    int end_row;
    char label[160];
} HeapLensStackFrame;

typedef struct {
    GtkWidget *drawing;
    GtkWidget *frame_list;
    HeapLensTargetProcess *process;
    HeapLensRegisterSnapshot registers;
    HeapLensMemoryMapSnapshot maps;
    HeapLensStackEntry entries[HEAPLENS_STACK_ROWS];
    HeapLensStackFrame frames[HEAPLENS_STACK_FRAMES];
    size_t frame_count;
    int selected_frame;
} HeapLensDbgStackState;

static HeapLensDbgStackState *heaplens_dbg_stack_state(GtkWidget *window) {
    return window ? g_object_get_data(G_OBJECT(window), "heaplens-dbg-stack-state") : NULL;
}

static void heaplens_dbg_stack_state_free(gpointer data) {
    HeapLensDbgStackState *state = data;

    if (!state) {
        return;
    }

    heaplens_memory_map_snapshot_free(&state->maps);
    g_free(state);
}

static const HeapLensMemoryMapEntry *heaplens_dbg_stack_find_map(HeapLensDbgStackState *state, uint64_t value) {
    return heaplens_memory_map_find_containing(&state->maps, value);
}

static void heaplens_dbg_stack_describe_pointer(HeapLensDbgStackState *state,
                                                uint64_t value,
                                                char *buffer,
                                                size_t buffer_size) {
    const HeapLensMemoryMapEntry *map = NULL;

    if (!state || !buffer || buffer_size == 0) {
        return;
    }

    map = heaplens_dbg_stack_find_map(state, value);
    if (!map) {
        g_snprintf(buffer, buffer_size, "-");
        return;
    }

    if (strstr(map->pathname, "[heap]")) {
        g_snprintf(buffer, buffer_size, "-> heap+0x%llx", (unsigned long long) (value - map->start));
        return;
    }
    if (strstr(map->pathname, "libc")) {
        HeapLensResolvedSymbol symbol;
        if (heaplens_symbol_resolve_address(map->pathname, map->start, value, &symbol)) {
            g_snprintf(buffer,
                       buffer_size,
                       "-> libc+0x%llx <%s+0x%llx>",
                       (unsigned long long) (value - map->start),
                       symbol.name,
                       (unsigned long long) symbol.offset);
        } else {
            g_snprintf(buffer, buffer_size, "-> libc+0x%llx", (unsigned long long) (value - map->start));
        }
        return;
    }
    if (strstr(map->pathname, "[stack]")) {
        if (value <= state->registers.gpr.rsp) {
            g_snprintf(buffer, buffer_size, "-> stack-0x%llx", (unsigned long long) (state->registers.gpr.rsp - value));
        } else {
            g_snprintf(buffer, buffer_size, "-> stack+0x%llx", (unsigned long long) (value - state->registers.gpr.rsp));
        }
        return;
    }
    if (strchr(map->perms, 'x')) {
        HeapLensResolvedSymbol symbol;
        const char *module = map->pathname[0] ? g_path_get_basename(map->pathname) : "<exec>";
        if (heaplens_symbol_resolve_address(map->pathname, map->start, value, &symbol)) {
            g_snprintf(buffer,
                       buffer_size,
                       "%s <%s+0x%llx>",
                       module,
                       symbol.name,
                       (unsigned long long) symbol.offset);
        } else {
            g_snprintf(buffer, buffer_size, "%s+0x%llx", module, (unsigned long long) (value - map->start));
        }
        if (map->pathname[0]) {
            g_free((gpointer) module);
        }
        return;
    }

    g_snprintf(buffer, buffer_size, "%s+0x%llx", map->pathname[0] ? map->pathname : "<anon>", (unsigned long long) (value - map->start));
}

static void heaplens_dbg_stack_refresh_rows(HeapLensDbgStackState *state) {
    guint64 values[HEAPLENS_STACK_ROWS];
    size_t index = 0;

    if (!state || !state->process || state->process->reader.mem_fd < 0) {
        memset(state->entries, 0, sizeof(state->entries));
        return;
    }

    memset(values, 0, sizeof(values));
    if (!heaplens_memory_reader_read_fully(&state->process->reader,
                                           state->registers.gpr.rsp,
                                           values,
                                           sizeof(values))) {
        memset(state->entries, 0, sizeof(state->entries));
        return;
    }

    for (index = 0; index < HEAPLENS_STACK_ROWS; ++index) {
        state->entries[index].address = state->registers.gpr.rsp + index * sizeof(guint64);
        state->entries[index].value = values[index];
        heaplens_dbg_stack_describe_pointer(state, values[index], state->entries[index].annotation, sizeof(state->entries[index].annotation));
    }
}

static void heaplens_dbg_stack_clear_frame_rows(GtkWidget *list) {
    GtkWidget *child = gtk_widget_get_first_child(list);

    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(GTK_LIST_BOX(list), child);
        child = next;
    }
}

static void heaplens_dbg_stack_row_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    HeapLensDbgStackState *state = user_data;
    (void) box;

    if (!state || !row) {
        return;
    }

    state->selected_frame = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "heaplens-frame-index"));
    gtk_widget_queue_draw(state->drawing);
}

static void heaplens_dbg_stack_refresh_frames(HeapLensDbgStackState *state) {
    uint64_t rbp = 0;

    if (!state) {
        return;
    }

    state->frame_count = 0;
    heaplens_dbg_stack_clear_frame_rows(state->frame_list);
    rbp = state->registers.gpr.rbp;
    while (rbp && state->frame_count < HEAPLENS_STACK_FRAMES) {
        guint64 pair[2] = {0, 0};
        HeapLensStackFrame *frame = &state->frames[state->frame_count];
        HeapLensResolvedSymbol symbol;
        const HeapLensMemoryMapEntry *map = NULL;
        GtkWidget *row = NULL;
        GtkWidget *label = NULL;

        if (!heaplens_memory_reader_read_fully(&state->process->reader, rbp, pair, sizeof(pair))) {
            break;
        }

        frame->rbp = rbp;
        frame->ret_addr = pair[1];
        frame->start_row = (int) ((rbp - state->registers.gpr.rsp) / 8);
        frame->end_row = frame->start_row + 2;
        map = heaplens_dbg_stack_find_map(state, pair[1]);

        if (map && map->pathname[0] && heaplens_symbol_resolve_address(map->pathname, map->start, pair[1], &symbol)) {
            g_snprintf(frame->label,
                       sizeof(frame->label),
                       "#%zu  %s + 0x%llx",
                       state->frame_count,
                       symbol.name,
                       (unsigned long long) symbol.offset);
        } else {
            g_snprintf(frame->label,
                       sizeof(frame->label),
                       "#%zu  0x%llx",
                       state->frame_count,
                       (unsigned long long) pair[1]);
        }

        row = gtk_list_box_row_new();
        label = gtk_label_new(frame->label);
        gtk_widget_add_css_class(label, "monospace");
        gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        g_object_set_data(G_OBJECT(row), "heaplens-frame-index", GINT_TO_POINTER((int) state->frame_count));
        gtk_list_box_append(GTK_LIST_BOX(state->frame_list), row);

        ++state->frame_count;
        if (pair[0] <= rbp) {
            break;
        }
        rbp = pair[0];
    }
}

static void heaplens_dbg_stack_draw(GtkDrawingArea *area,
                                    cairo_t *cr,
                                    int width,
                                    int height,
                                    gpointer user_data) {
    HeapLensDbgStackState *state = user_data;
    size_t index = 0;
    (void) area;
    (void) height;

    cairo_set_source_rgb(cr, 0.145, 0.145, 0.145);
    cairo_paint(cr);
    cairo_select_font_face(cr, "JetBrains Mono", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10.0);

    for (index = 0; index < HEAPLENS_STACK_ROWS; ++index) {
        const HeapLensStackEntry *entry = &state->entries[index];
        double y = 8.0 + index * 22.0;
        char address[32];
        char value[32];
        gboolean is_rsp = entry->address == state->registers.gpr.rsp;
        gboolean is_rbp = entry->address == state->registers.gpr.rbp;

        if (state->selected_frame >= 0 && (size_t) state->selected_frame < state->frame_count) {
            HeapLensStackFrame *frame = &state->frames[state->selected_frame];
            if ((int) index >= frame->start_row && (int) index <= frame->end_row) {
                cairo_set_source_rgba(cr, 0.208, 0.518, 0.894, 0.10);
                cairo_rectangle(cr, 0.0, y - 12.0, width, 22.0);
                cairo_fill(cr);
            }
        }

        if (is_rsp) {
            cairo_set_source_rgba(cr, 0.208, 0.518, 0.894, 0.25);
            cairo_rectangle(cr, 0.0, y - 12.0, width, 22.0);
            cairo_fill(cr);
        } else if (is_rbp) {
            cairo_set_source_rgba(cr, 0.666, 0.666, 0.666, 0.18);
            cairo_rectangle(cr, 0.0, y - 12.0, width, 22.0);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0.666, 0.666, 0.666);
            cairo_set_line_width(cr, 1.0);
            cairo_stroke(cr);
        }

        g_snprintf(address, sizeof(address), "0x%016llx", (unsigned long long) entry->address);
        g_snprintf(value, sizeof(value), "0x%016llx", (unsigned long long) entry->value);
        cairo_set_source_rgb(cr, 0.666, 0.666, 0.666);
        cairo_move_to(cr, 8.0, y);
        cairo_show_text(cr, address);
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_move_to(cr, 150.0, y);
        cairo_show_text(cr, value);
        cairo_set_source_rgb(cr, 0.666, 0.666, 0.666);
        cairo_move_to(cr, 300.0, y);
        cairo_show_text(cr, entry->annotation[0] ? entry->annotation : "-");
    }

    cairo_set_source_rgb(cr, 0.384, 0.627, 0.918);
    cairo_set_line_width(cr, 1.0);
    for (index = 0; index + 1 < state->frame_count; ++index) {
        double start_y = 8.0 + state->frames[index].start_row * 22.0;
        double end_y = 8.0 + state->frames[index + 1].start_row * 22.0;
        cairo_move_to(cr, width - 12.0, start_y);
        cairo_line_to(cr, width - 12.0, end_y);
        cairo_stroke(cr);
    }
}

GtkWidget *heaplens_dbg_stack_new(GtkWindow *parent) {
    GtkWidget *window = gtk_window_new();
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *frame_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *frame_title = gtk_label_new("Stack Frames");
    GtkWidget *frame_scroller = gtk_scrolled_window_new();
    GtkWidget *frame_list = gtk_list_box_new();
    GtkWidget *stack_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *stack_title = gtk_label_new("Stack Canvas");
    GtkWidget *stack_scroller = gtk_scrolled_window_new();
    GtkWidget *drawing = gtk_drawing_area_new();
    HeapLensDbgStackState *state = g_new0(HeapLensDbgStackState, 1);

    heaplens_dbg_window_init(GTK_WINDOW(window), parent, "Stack", "stack", 360, 500);
    gtk_widget_set_margin_top(root, 8);
    gtk_widget_set_margin_bottom(root, 8);
    gtk_widget_set_margin_start(root, 8);
    gtk_widget_set_margin_end(root, 8);

    gtk_widget_add_css_class(frame_panel, "panel");
    gtk_widget_add_css_class(frame_title, "panel-heading");
    gtk_widget_add_css_class(stack_panel, "panel");
    gtk_widget_add_css_class(stack_title, "panel-heading");
    gtk_label_set_xalign(GTK_LABEL(frame_title), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(stack_title), 0.0f);

    gtk_list_box_set_selection_mode(GTK_LIST_BOX(frame_list), GTK_SELECTION_SINGLE);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(frame_scroller), TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(frame_scroller), frame_list);
    gtk_widget_set_size_request(frame_scroller, -1, 120);
    g_signal_connect(frame_list, "row-selected", G_CALLBACK(heaplens_dbg_stack_row_selected), state);

    gtk_widget_add_css_class(drawing, "stack-canvas");
    gtk_widget_set_size_request(drawing, 340, HEAPLENS_STACK_ROWS * 22 + 16);
    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(drawing), 340);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(drawing), HEAPLENS_STACK_ROWS * 22 + 16);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing), heaplens_dbg_stack_draw, state, NULL);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(stack_scroller), TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(stack_scroller), drawing);
    gtk_widget_set_vexpand(stack_scroller, TRUE);

    gtk_box_append(GTK_BOX(frame_panel), frame_title);
    gtk_box_append(GTK_BOX(frame_panel), frame_scroller);
    gtk_box_append(GTK_BOX(stack_panel), stack_title);
    gtk_box_append(GTK_BOX(stack_panel), stack_scroller);
    gtk_box_append(GTK_BOX(root), frame_panel);
    gtk_box_append(GTK_BOX(root), stack_panel);
    gtk_window_set_child(GTK_WINDOW(window), root);

    state->drawing = drawing;
    state->frame_list = frame_list;
    state->selected_frame = -1;
    g_object_set_data_full(G_OBJECT(window), "heaplens-dbg-stack-state", state, heaplens_dbg_stack_state_free);
    return window;
}

void heaplens_dbg_stack_set_snapshot(GtkWidget *window,
                                     HeapLensTargetProcess *process,
                                     const HeapLensRegisterSnapshot *snapshot) {
    HeapLensDbgStackState *state = heaplens_dbg_stack_state(window);

    if (!state || !process || !snapshot) {
        return;
    }

    state->process = process;
    state->registers = *snapshot;
    heaplens_memory_map_snapshot_free(&state->maps);
    memset(&state->maps, 0, sizeof(state->maps));
    heaplens_memory_reader_read_maps(process->pid, &state->maps);
    heaplens_dbg_stack_refresh_rows(state);
    heaplens_dbg_stack_refresh_frames(state);
    gtk_widget_queue_draw(state->drawing);
}

void heaplens_dbg_stack_clear(GtkWidget *window) {
    HeapLensDbgStackState *state = heaplens_dbg_stack_state(window);

    if (!state) {
        return;
    }

    state->process = NULL;
    memset(&state->registers, 0, sizeof(state->registers));
    heaplens_memory_map_snapshot_free(&state->maps);
    memset(state->entries, 0, sizeof(state->entries));
    state->frame_count = 0;
    state->selected_frame = -1;
    heaplens_dbg_stack_clear_frame_rows(state->frame_list);
    gtk_widget_queue_draw(state->drawing);
}
