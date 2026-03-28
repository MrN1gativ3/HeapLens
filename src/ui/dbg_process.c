#include "dbg_process.h"

#include <stdio.h>
#include <string.h>

#include "dbg_common.h"

typedef struct {
    GtkWidget *filter_entry;
    GtkWidget *address_bar;
    GtkWidget *info_label;
    GtkStringList *rows;
    GtkFilterListModel *filtered_model;
    GtkCustomFilter *filter;
    HeapLensTargetProcess *process;
    HeapLensMemoryMapSnapshot maps;
} HeapLensDbgProcessState;

static HeapLensDbgProcessState *heaplens_dbg_process_state(GtkWidget *window) {
    return window ? g_object_get_data(G_OBJECT(window), "heaplens-dbg-process-state") : NULL;
}

static void heaplens_dbg_process_state_free(gpointer data) {
    HeapLensDbgProcessState *state = data;

    if (!state) {
        return;
    }

    heaplens_memory_map_snapshot_free(&state->maps);
    g_clear_object(&state->rows);
    g_clear_object(&state->filtered_model);
    g_clear_object(&state->filter);
    g_free(state);
}

static char **heaplens_dbg_process_split(const char *row) {
    return g_strsplit(row ? row : "", "\t", 6);
}

static gboolean heaplens_dbg_process_filter_func(gpointer item, gpointer user_data) {
    HeapLensDbgProcessState *state = user_data;
    const char *text = NULL;
    const char *query = NULL;
    char *lower_text = NULL;
    char *lower_query = NULL;
    gboolean matched = TRUE;

    if (!state || !state->filter_entry || !GTK_IS_STRING_OBJECT(item)) {
        return TRUE;
    }

    query = gtk_editable_get_text(GTK_EDITABLE(state->filter_entry));
    if (!query || !query[0]) {
        return TRUE;
    }

    text = gtk_string_object_get_string(GTK_STRING_OBJECT(item));
    lower_text = g_ascii_strdown(text, -1);
    lower_query = g_ascii_strdown(query, -1);
    matched = g_strrstr(lower_text, lower_query) != NULL;
    g_free(lower_text);
    g_free(lower_query);
    return matched;
}

static void heaplens_dbg_process_entry_changed(GtkEditable *editable, gpointer user_data) {
    HeapLensDbgProcessState *state = user_data;
    (void) editable;

    if (state && state->filter) {
        gtk_filter_changed(GTK_FILTER(state->filter), GTK_FILTER_CHANGE_DIFFERENT);
    }
}

static void heaplens_dbg_process_setup_label(GtkListItemFactory *factory, GtkListItem *item, gpointer user_data) {
    GtkWidget *label = gtk_label_new("");
    (void) factory;
    (void) user_data;

    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_widget_add_css_class(label, "monospace");
    gtk_list_item_set_child(item, label);
}

static void heaplens_dbg_process_bind_column(GtkListItemFactory *factory, GtkListItem *item, gpointer user_data) {
    int column = GPOINTER_TO_INT(user_data);
    GtkWidget *label = gtk_list_item_get_child(item);
    GtkStringObject *string = GTK_STRING_OBJECT(gtk_list_item_get_item(item));
    char **fields = heaplens_dbg_process_split(gtk_string_object_get_string(string));
    const char *perms = fields[3] ? fields[3] : "";

    (void) factory;

    gtk_widget_remove_css_class(label, "green");
    gtk_widget_remove_css_class(label, "yellow");
    gtk_widget_remove_css_class(label, "red");
    gtk_widget_remove_css_class(label, "dim");
    gtk_label_set_text(GTK_LABEL(label), fields[column] ? fields[column] : "");

    if (strcmp(perms, "rwxp") == 0 || strcmp(perms, "rwx-") == 0) {
        gtk_widget_add_css_class(label, "red");
    } else if (g_str_has_prefix(perms, "r-x")) {
        gtk_widget_add_css_class(label, "green");
    } else if (g_str_has_prefix(perms, "rw-")) {
        gtk_widget_add_css_class(label, "yellow");
    } else if (g_str_has_prefix(perms, "r--")) {
        gtk_widget_add_css_class(label, "dim");
    }

    g_strfreev(fields);
}

static GtkColumnViewColumn *heaplens_dbg_process_make_column(const char *title, int column) {
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();

    g_signal_connect(factory, "setup", G_CALLBACK(heaplens_dbg_process_setup_label), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(heaplens_dbg_process_bind_column), GINT_TO_POINTER(column));
    return gtk_column_view_column_new(title, GTK_LIST_ITEM_FACTORY(factory));
}

static void heaplens_dbg_process_draw_bar(GtkDrawingArea *area,
                                          cairo_t *cr,
                                          int width,
                                          int height,
                                          gpointer user_data) {
    HeapLensDbgProcessState *state = user_data;
    size_t index = 0;
    double x = 0.0;
    (void) area;
    (void) height;

    cairo_set_source_rgb(cr, 0.145, 0.145, 0.145);
    cairo_paint(cr);

    if (!state || state->maps.count == 0) {
        return;
    }

    for (index = 0; index < state->maps.count; ++index) {
        const HeapLensMemoryMapEntry *entry = &state->maps.entries[index];
        const char *label = entry->pathname[0] ? entry->pathname : "<anon>";
        double segment_width = MAX(12.0, width / (double) MAX(state->maps.count, (size_t) 1));

        if (strstr(label, "libc")) {
            cairo_set_source_rgb(cr, 0.753, 0.380, 0.796);
        } else if (strstr(label, "[stack]")) {
            cairo_set_source_rgb(cr, 0.384, 0.627, 0.918);
        } else if (strstr(label, "[heap]")) {
            cairo_set_source_rgb(cr, 0.341, 0.890, 0.537);
        } else if (g_str_has_prefix(entry->perms, "r-x")) {
            cairo_set_source_rgb(cr, 0.341, 0.890, 0.537);
        } else if (g_str_has_prefix(entry->perms, "rw-")) {
            cairo_set_source_rgb(cr, 0.976, 0.941, 0.419);
        } else {
            cairo_set_source_rgb(cr, 0.400, 0.400, 0.400);
        }

        cairo_rectangle(cr, x, 6.0, segment_width - 2.0, 28.0);
        cairo_fill(cr);
        x += segment_width;
    }
}

static void heaplens_dbg_process_refresh_info(HeapLensDbgProcessState *state) {
    char status_path[64];
    FILE *status = NULL;
    char line[256];
    char proc_state[16] = "?";
    int threads = 0;
    const HeapLensMemoryMapEntry *heap = NULL;

    if (!state || !state->process) {
        gtk_label_set_text(GTK_LABEL(state->info_label), "PID: -   State: -   Threads: -   Heap base: -   Heap size: -   Break: -");
        return;
    }

    snprintf(status_path, sizeof(status_path), "/proc/%d/status", state->process->pid);
    status = fopen(status_path, "r");
    if (status) {
        while (fgets(line, sizeof(line), status)) {
            if (sscanf(line, "State:\t%15s", proc_state) == 1) {
                continue;
            }
            if (sscanf(line, "Threads:\t%d", &threads) == 1) {
                continue;
            }
        }
        fclose(status);
    }

    heap = heaplens_memory_map_find_named(&state->maps, "[heap]");
    if (heap) {
        char buffer[512];
        g_snprintf(buffer,
                   sizeof(buffer),
                   "PID: %d   State: %s   Threads: %d   Heap base: 0x%llx   Heap size: %zu KB   Break: 0x%llx",
                   state->process->pid,
                   proc_state,
                   threads,
                   (unsigned long long) heap->start,
                   (size_t) ((heap->end - heap->start) / 1024),
                   (unsigned long long) heap->end);
        gtk_label_set_text(GTK_LABEL(state->info_label), buffer);
    } else {
        gtk_label_set_text(GTK_LABEL(state->info_label), "Heap mapping unavailable for current process.");
    }
}

static void heaplens_dbg_process_rebuild_rows(HeapLensDbgProcessState *state) {
    size_t index = 0;

    if (!state || !state->rows) {
        return;
    }

    gtk_string_list_splice(state->rows, 0, g_list_model_get_n_items(G_LIST_MODEL(state->rows)), NULL);
    for (index = 0; index < state->maps.count; ++index) {
        const HeapLensMemoryMapEntry *entry = &state->maps.entries[index];
        char row[PATH_MAX + 128];
        g_snprintf(row,
                   sizeof(row),
                   "0x%llx\t0x%llx\t%zu KB\t%s\t0x%llx\t%s",
                   (unsigned long long) entry->start,
                   (unsigned long long) entry->end,
                   (size_t) ((entry->end - entry->start) / 1024),
                   entry->perms,
                   (unsigned long long) entry->offset,
                   entry->pathname[0] ? entry->pathname : "<anonymous>");
        gtk_string_list_append(state->rows, row);
    }

    heaplens_dbg_process_refresh_info(state);
}

GtkWidget *heaplens_dbg_process_new(GtkWindow *parent) {
    GtkWidget *window = gtk_window_new();
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *filter_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *filter_title = gtk_label_new("Filter");
    GtkWidget *filter_entry = gtk_entry_new();
    GtkStringList *rows = gtk_string_list_new(NULL);
    GtkCustomFilter *filter = gtk_custom_filter_new(heaplens_dbg_process_filter_func, NULL, NULL);
    GtkFilterListModel *filtered = gtk_filter_list_model_new(G_LIST_MODEL(rows), GTK_FILTER(filter));
    GtkSelectionModel *selection = GTK_SELECTION_MODEL(gtk_no_selection_new(G_LIST_MODEL(filtered)));
    GtkWidget *column_view = gtk_column_view_new(selection);
    GtkWidget *table_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *table_title = gtk_label_new("Memory Map");
    GtkWidget *table_scroller = gtk_scrolled_window_new();
    GtkWidget *bar_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *bar_title = gtk_label_new("Address Space");
    GtkWidget *address_bar = gtk_drawing_area_new();
    GtkWidget *info_label = gtk_label_new("PID: -   State: -   Threads: -   Heap base: -   Heap size: -   Break: -");
    HeapLensDbgProcessState *state = g_new0(HeapLensDbgProcessState, 1);

    heaplens_dbg_window_init(GTK_WINDOW(window), parent, "Process", "process", 560, 400);
    gtk_widget_set_margin_top(root, 8);
    gtk_widget_set_margin_bottom(root, 8);
    gtk_widget_set_margin_start(root, 8);
    gtk_widget_set_margin_end(root, 8);

    gtk_label_set_xalign(GTK_LABEL(filter_title), 0.0f);
    gtk_editable_set_text(GTK_EDITABLE(filter_entry), "");
    gtk_widget_set_hexpand(filter_entry, TRUE);
    gtk_box_append(GTK_BOX(filter_bar), filter_title);
    gtk_box_append(GTK_BOX(filter_bar), filter_entry);

    gtk_widget_add_css_class(table_panel, "panel");
    gtk_widget_add_css_class(table_title, "panel-heading");
    gtk_widget_add_css_class(bar_panel, "panel");
    gtk_widget_add_css_class(bar_title, "panel-heading");
    gtk_label_set_xalign(GTK_LABEL(table_title), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(bar_title), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(info_label), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(info_label), TRUE);

    gtk_column_view_append_column(GTK_COLUMN_VIEW(column_view), heaplens_dbg_process_make_column("Start", 0));
    gtk_column_view_append_column(GTK_COLUMN_VIEW(column_view), heaplens_dbg_process_make_column("End", 1));
    gtk_column_view_append_column(GTK_COLUMN_VIEW(column_view), heaplens_dbg_process_make_column("Size", 2));
    gtk_column_view_append_column(GTK_COLUMN_VIEW(column_view), heaplens_dbg_process_make_column("Perms", 3));
    gtk_column_view_append_column(GTK_COLUMN_VIEW(column_view), heaplens_dbg_process_make_column("Offset", 4));
    gtk_column_view_append_column(GTK_COLUMN_VIEW(column_view), heaplens_dbg_process_make_column("Name", 5));
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(table_scroller), TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(table_scroller), column_view);
    gtk_widget_set_vexpand(table_scroller, TRUE);
    gtk_box_append(GTK_BOX(table_panel), table_title);
    gtk_box_append(GTK_BOX(table_panel), filter_bar);
    gtk_box_append(GTK_BOX(table_panel), table_scroller);

    gtk_widget_set_size_request(address_bar, 520, 40);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(address_bar), heaplens_dbg_process_draw_bar, state, NULL);
    gtk_box_append(GTK_BOX(bar_panel), bar_title);
    gtk_box_append(GTK_BOX(bar_panel), address_bar);

    gtk_box_append(GTK_BOX(root), table_panel);
    gtk_box_append(GTK_BOX(root), bar_panel);
    gtk_box_append(GTK_BOX(root), info_label);
    gtk_window_set_child(GTK_WINDOW(window), root);

    state->filter_entry = filter_entry;
    state->address_bar = address_bar;
    state->info_label = info_label;
    state->rows = g_object_ref(rows);
    state->filtered_model = g_object_ref(filtered);
    state->filter = g_object_ref(filter);
    gtk_custom_filter_set_filter_func(filter, heaplens_dbg_process_filter_func, state, NULL);
    g_signal_connect(filter_entry, "changed", G_CALLBACK(heaplens_dbg_process_entry_changed), state);
    g_object_set_data_full(G_OBJECT(window), "heaplens-dbg-process-state", state, heaplens_dbg_process_state_free);

    g_object_unref(selection);
    g_object_unref(filtered);
    g_object_unref(filter);
    g_object_unref(rows);
    return window;
}

void heaplens_dbg_process_set_target(GtkWidget *window, HeapLensTargetProcess *process) {
    HeapLensDbgProcessState *state = heaplens_dbg_process_state(window);

    if (!state || !process) {
        return;
    }

    state->process = process;
    heaplens_memory_map_snapshot_free(&state->maps);
    memset(&state->maps, 0, sizeof(state->maps));
    heaplens_memory_reader_read_maps(process->pid, &state->maps);
    heaplens_dbg_process_rebuild_rows(state);
    gtk_widget_queue_draw(state->address_bar);
}

void heaplens_dbg_process_clear(GtkWidget *window) {
    HeapLensDbgProcessState *state = heaplens_dbg_process_state(window);

    if (!state) {
        return;
    }

    state->process = NULL;
    heaplens_memory_map_snapshot_free(&state->maps);
    memset(&state->maps, 0, sizeof(state->maps));
    heaplens_dbg_process_rebuild_rows(state);
    gtk_widget_queue_draw(state->address_bar);
}
