#include "terminal_widget.h"

#include "theme.h"

typedef struct {
    VteTerminal *terminal;
} HeapLensTerminalState;

static void heaplens_terminal_widget_destroy(gpointer data) {
    g_free(data);
}

GtkWidget *heaplens_terminal_widget_new(void) {
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *title = gtk_label_new("Terminal");
    VteTerminal *terminal = VTE_TERMINAL(vte_terminal_new());
    char *argv[] = {"/bin/bash", NULL};
    HeapLensTerminalState *state = g_new0(HeapLensTerminalState, 1);

    state->terminal = terminal;
    g_object_set_data_full(G_OBJECT(root), "heaplens-terminal-state", state, heaplens_terminal_widget_destroy);

    gtk_widget_add_css_class(root, "panel");
    gtk_widget_add_css_class(root, "terminal-panel");
    gtk_widget_add_css_class(title, "panel-heading");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_widget_set_hexpand(GTK_WIDGET(terminal), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(terminal), TRUE);
    vte_terminal_set_scrollback_lines(terminal, 5000);
    vte_terminal_set_cursor_blink_mode(terminal, VTE_CURSOR_BLINK_OFF);
    vte_terminal_set_mouse_autohide(terminal, TRUE);
    vte_terminal_set_font_scale(terminal, 0.92);

    vte_terminal_spawn_async(terminal,
                             VTE_PTY_DEFAULT,
                             NULL,
                             argv,
                             NULL,
                             G_SPAWN_SEARCH_PATH,
                             NULL,
                             NULL,
                             NULL,
                             -1,
                             NULL,
                             NULL,
                             NULL);

    heaplens_terminal_widget_apply_theme(root, heaplens_theme_default_index());
    gtk_box_append(GTK_BOX(root), title);
    gtk_box_append(GTK_BOX(root), GTK_WIDGET(terminal));
    return root;
}

void heaplens_terminal_widget_print_banner(GtkWidget *panel, const char *banner) {
    HeapLensTerminalState *state = g_object_get_data(G_OBJECT(panel), "heaplens-terminal-state");
    if (!panel || !state || !banner) {
        return;
    }
    vte_terminal_feed(state->terminal, banner, -1);
    vte_terminal_feed(state->terminal, "\n", 1);
}

void heaplens_terminal_widget_apply_theme(GtkWidget *panel, guint theme_index) {
    HeapLensTerminalState *state = g_object_get_data(G_OBJECT(panel), "heaplens-terminal-state");

    if (!panel || !state) {
        return;
    }

    heaplens_theme_apply_terminal(state->terminal, theme_index);
}

VteTerminal *heaplens_terminal_widget_get_terminal(GtkWidget *panel) {
    HeapLensTerminalState *state = g_object_get_data(G_OBJECT(panel), "heaplens-terminal-state");
    return state ? state->terminal : NULL;
}
