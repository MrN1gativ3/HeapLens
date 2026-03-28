#include "disasm_panel.h"

typedef struct {
    GtkWidget *text;
} HeapLensDisasmPanelState;

static void heaplens_disasm_panel_destroy(gpointer data) {
    g_free(data);
}

GtkWidget *heaplens_disasm_panel_new(void) {
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *title = gtk_label_new("Disassembly");
    GtkWidget *scroller = gtk_scrolled_window_new();
    GtkWidget *text = gtk_text_view_new();
    HeapLensDisasmPanelState *state = g_new0(HeapLensDisasmPanelState, 1);

    state->text = text;
    g_object_set_data_full(G_OBJECT(root), "heaplens-disasm-state", state, heaplens_disasm_panel_destroy);

    gtk_widget_add_css_class(root, "panel");
    gtk_widget_add_css_class(title, "panel-heading");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(text), TRUE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text), 12);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text), 12);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(text), 10);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(text), 10);
    gtk_widget_add_css_class(text, "monospace");
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(scroller), FALSE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), text);
    gtk_widget_set_vexpand(scroller, TRUE);

    gtk_box_append(GTK_BOX(root), title);
    gtk_box_append(GTK_BOX(root), scroller);
    return root;
}

void heaplens_disasm_panel_set_text(GtkWidget *panel, const char *text) {
    HeapLensDisasmPanelState *state = g_object_get_data(G_OBJECT(panel), "heaplens-disasm-state");
    if (!panel || !state) {
        return;
    }
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text)),
                             text ? text : "No disassembly available.",
                             -1);
}
