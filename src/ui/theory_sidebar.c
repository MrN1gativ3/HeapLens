#include "theory_sidebar.h"

typedef struct {
    GtkWidget *text;
} HeapLensTheorySidebarState;

static void heaplens_theory_sidebar_destroy(gpointer data) {
    g_free(data);
}

GtkWidget *heaplens_theory_sidebar_new(void) {
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *title = gtk_label_new("Theory Sidebar");
    GtkWidget *scroller = gtk_scrolled_window_new();
    GtkWidget *text = gtk_text_view_new();
    HeapLensTheorySidebarState *state = g_new0(HeapLensTheorySidebarState, 1);

    state->text = text;
    g_object_set_data_full(G_OBJECT(root), "heaplens-theory-state", state, heaplens_theory_sidebar_destroy);

    gtk_widget_add_css_class(root, "card");
    gtk_widget_add_css_class(title, "title");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(text), TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), text);
    gtk_widget_set_vexpand(scroller, TRUE);

    gtk_box_append(GTK_BOX(root), title);
    gtk_box_append(GTK_BOX(root), scroller);
    return root;
}

void heaplens_theory_sidebar_set_text(GtkWidget *panel, const char *text) {
    HeapLensTheorySidebarState *state = g_object_get_data(G_OBJECT(panel), "heaplens-theory-state");
    if (!panel || !state) {
        return;
    }
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text)),
                             text ? text : "Select a technique to load theory notes.",
                             -1);
}
