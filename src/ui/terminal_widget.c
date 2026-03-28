#include "terminal_widget.h"

typedef struct {
    VteTerminal *terminal;
} HeapLensTerminalState;

static void heaplens_terminal_widget_destroy(gpointer data) {
    g_free(data);
}

GtkWidget *heaplens_terminal_widget_new(void) {
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *title = gtk_label_new("Embedded Terminal");
    VteTerminal *terminal = VTE_TERMINAL(vte_terminal_new());
    char *argv[] = {"/bin/bash", NULL};
    HeapLensTerminalState *state = g_new0(HeapLensTerminalState, 1);

    state->terminal = terminal;
    g_object_set_data_full(G_OBJECT(root), "heaplens-terminal-state", state, heaplens_terminal_widget_destroy);

    gtk_widget_add_css_class(root, "card");
    gtk_widget_add_css_class(title, "title");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_widget_set_hexpand(GTK_WIDGET(terminal), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(terminal), TRUE);

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

VteTerminal *heaplens_terminal_widget_get_terminal(GtkWidget *panel) {
    HeapLensTerminalState *state = g_object_get_data(G_OBJECT(panel), "heaplens-terminal-state");
    return state ? state->terminal : NULL;
}
