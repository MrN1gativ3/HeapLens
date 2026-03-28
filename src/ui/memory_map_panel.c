#include "memory_map_panel.h"

#include <stdio.h>
#include <string.h>

#include "../core/memory_reader.h"

typedef struct {
    GtkWidget *list;
    GtkWidget *details;
} HeapLensMemoryMapPanelState;

static void heaplens_memory_map_panel_set_details(GtkWidget *details,
                                                  const HeapLensMemoryMapEntry *entry) {
    char buffer[1024];

    if (!entry) {
        gtk_label_set_text(GTK_LABEL(details), "Select a region to inspect its permissions, size, and backing file.");
        return;
    }

    snprintf(buffer,
             sizeof(buffer),
             "Range: %#llx - %#llx\nPermissions: %s\nOffset: %#llx\nDevice: %s\nInode: %lu\nPath: %s",
             (unsigned long long) entry->start,
             (unsigned long long) entry->end,
             entry->perms,
             (unsigned long long) entry->offset,
             entry->dev,
             entry->inode,
             entry->pathname[0] ? entry->pathname : "<anonymous>");
    gtk_label_set_text(GTK_LABEL(details), buffer);
}

static void heaplens_memory_map_panel_row_selected(GtkListBox *box,
                                                   GtkListBoxRow *row,
                                                   gpointer user_data) {
    HeapLensMemoryMapPanelState *state = user_data;
    (void) box;

    if (!state || !row) {
        heaplens_memory_map_panel_set_details(state ? state->details : NULL, NULL);
        return;
    }

    heaplens_memory_map_panel_set_details(state->details,
                                          g_object_get_data(G_OBJECT(row), "heaplens-map-entry"));
}

static void heaplens_memory_map_panel_destroy(gpointer data) {
    g_free(data);
}

static void heaplens_memory_map_panel_clear(GtkWidget *list) {
    GtkWidget *child = gtk_widget_get_first_child(list);

    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(GTK_LIST_BOX(list), child);
        child = next;
    }
}

GtkWidget *heaplens_memory_map_panel_new(void) {
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *title = gtk_label_new("Live /proc/<pid>/maps");
    GtkWidget *list_scroller = gtk_scrolled_window_new();
    GtkWidget *list = gtk_list_box_new();
    GtkWidget *details = gtk_label_new("Select a region to inspect its permissions, size, and backing file.");
    HeapLensMemoryMapPanelState *state = g_new0(HeapLensMemoryMapPanelState, 1);

    state->list = list;
    state->details = details;
    g_object_set_data_full(G_OBJECT(root), "heaplens-memory-map-state", state, heaplens_memory_map_panel_destroy);

    gtk_widget_add_css_class(root, "card");
    gtk_widget_add_css_class(title, "title");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(details), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(details), TRUE);
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list), GTK_SELECTION_SINGLE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(list_scroller), list);
    gtk_widget_set_vexpand(list_scroller, TRUE);

    g_signal_connect(list, "row-selected", G_CALLBACK(heaplens_memory_map_panel_row_selected), state);

    gtk_box_append(GTK_BOX(root), title);
    gtk_box_append(GTK_BOX(root), list_scroller);
    gtk_box_append(GTK_BOX(root), details);
    return root;
}

void heaplens_memory_map_panel_load_pid(GtkWidget *panel, pid_t pid) {
    HeapLensMemoryMapPanelState *state = g_object_get_data(G_OBJECT(panel), "heaplens-memory-map-state");
    HeapLensMemoryMapSnapshot snapshot;
    size_t index = 0;

    if (!panel || !state) {
        return;
    }

    memset(&snapshot, 0, sizeof(snapshot));
    heaplens_memory_map_panel_clear(state->list);
    if (!heaplens_memory_reader_read_maps(pid, &snapshot)) {
        gtk_label_set_text(GTK_LABEL(state->details), "Unable to read /proc/<pid>/maps for this process.");
        return;
    }

    for (index = 0; index < snapshot.count; ++index) {
        const HeapLensMemoryMapEntry *entry = &snapshot.entries[index];
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new(NULL);
        HeapLensMemoryMapEntry *copy = g_new0(HeapLensMemoryMapEntry, 1);
        char text[512];

        *copy = *entry;
        snprintf(text,
                 sizeof(text),
                 "%#llx-%#llx  %-4s  %s",
                 (unsigned long long) entry->start,
                 (unsigned long long) entry->end,
                 entry->perms,
                 entry->pathname[0] ? entry->pathname : "<anonymous>");

        gtk_label_set_text(GTK_LABEL(label), text);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        g_object_set_data_full(G_OBJECT(row), "heaplens-map-entry", copy, g_free);
        gtk_list_box_append(GTK_LIST_BOX(state->list), row);
    }

    heaplens_memory_map_panel_set_details(state->details,
                                          snapshot.count > 0 ? &snapshot.entries[0] : NULL);
    heaplens_memory_map_snapshot_free(&snapshot);
}
