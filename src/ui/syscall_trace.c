#include "syscall_trace.h"

typedef struct {
    GtkWidget *text;
} HeapLensSyscallTraceState;

static void heaplens_syscall_trace_destroy(gpointer data) {
    g_free(data);
}

GtkWidget *heaplens_syscall_trace_new(void) {
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *title = gtk_label_new("Syscall Trace");
    GtkWidget *scroller = gtk_scrolled_window_new();
    GtkWidget *text = gtk_text_view_new();
    HeapLensSyscallTraceState *state = g_new0(HeapLensSyscallTraceState, 1);

    state->text = text;
    g_object_set_data_full(G_OBJECT(root), "heaplens-syscall-trace-state", state, heaplens_syscall_trace_destroy);

    gtk_widget_add_css_class(root, "card");
    gtk_widget_add_css_class(title, "title");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(text), TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), text);
    gtk_widget_set_vexpand(scroller, TRUE);

    gtk_box_append(GTK_BOX(root), title);
    gtk_box_append(GTK_BOX(root), scroller);
    return root;
}

void heaplens_syscall_trace_append(GtkWidget *panel, const char *line) {
    HeapLensSyscallTraceState *state = g_object_get_data(G_OBJECT(panel), "heaplens-syscall-trace-state");
    GtkTextBuffer *buffer;
    GtkTextIter end;

    if (!panel || !state || !line) {
        return;
    }

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text));
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, line, -1);
    gtk_text_buffer_insert(buffer, &end, "\n", 1);
}

void heaplens_syscall_trace_clear(GtkWidget *panel) {
    HeapLensSyscallTraceState *state = g_object_get_data(G_OBJECT(panel), "heaplens-syscall-trace-state");
    if (!panel || !state) {
        return;
    }
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text)), "", -1);
}
