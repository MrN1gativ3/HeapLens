#include "main_window.h"

#include "tab_exploit.h"
#include "tab_learning.h"
#include "theme.h"

typedef struct {
    GtkCssProvider *provider;
    GtkWidget *exploit_tab;
} HeapLensMainWindowState;

static void heaplens_main_window_destroy(gpointer data) {
    HeapLensMainWindowState *state = data;

    if (!state) {
        return;
    }
    g_clear_object(&state->provider);
    g_free(state);
}

static void heaplens_main_window_apply_theme(HeapLensMainWindowState *state) {
    if (!state || !state->provider) {
        return;
    }

    heaplens_theme_apply_css(state->provider, heaplens_theme_default_index());
    heaplens_tab_exploit_apply_theme(state->exploit_tab, heaplens_theme_default_index());
}

GtkWidget *heaplens_main_window_new(GtkApplication *application) {
    GtkWidget *window = gtk_application_window_new(application);
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *header = gtk_header_bar_new();
    GtkWidget *title = gtk_label_new("HeapLens");
    GtkWidget *subtitle = gtk_label_new("glibc heap internals and exploitation lab");
    GtkWidget *title_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *switcher = gtk_stack_switcher_new();
    GtkWidget *stack = gtk_stack_new();
    GtkWidget *learning_tab = heaplens_tab_learning_new();
    GtkWidget *exploit_tab = heaplens_tab_exploit_new(GTK_WINDOW(window));
    GtkWidget *meta = gtk_label_new("Linux x86_64");
    HeapLensMainWindowState *state = g_new0(HeapLensMainWindowState, 1);

    gtk_window_set_title(GTK_WINDOW(window), "HeapLens");
    gtk_window_set_default_size(GTK_WINDOW(window), 1700, 1020);

    gtk_widget_add_css_class(header, "header-bar");
    gtk_widget_add_css_class(switcher, "tab-bar");
    gtk_widget_add_css_class(title, "accent-text");
    gtk_widget_add_css_class(subtitle, "secondary");
    gtk_widget_add_css_class(meta, "secondary");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(subtitle), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(meta), 1.0f);
    gtk_widget_set_margin_start(title_box, 4);
    gtk_widget_set_margin_end(title_box, 4);
    gtk_box_append(GTK_BOX(title_box), title);
    gtk_box_append(GTK_BOX(title_box), subtitle);

    gtk_stack_add_titled(GTK_STACK(stack), learning_tab, "learning", "glibc Heap Internals");
    gtk_stack_add_titled(GTK_STACK(stack), exploit_tab, "exploit", "Exploitation Lab");
    gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcher), GTK_STACK(stack));
    gtk_widget_set_hexpand(stack, TRUE);
    gtk_widget_set_vexpand(stack, TRUE);

    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), title_box);
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), switcher);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), meta);
    gtk_window_set_titlebar(GTK_WINDOW(window), header);

    gtk_box_append(GTK_BOX(root), stack);
    gtk_window_set_child(GTK_WINDOW(window), root);

    state->provider = gtk_css_provider_new();
    state->exploit_tab = exploit_tab;
    g_object_set_data_full(G_OBJECT(window), "heaplens-main-window-state", state, heaplens_main_window_destroy);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                               GTK_STYLE_PROVIDER(state->provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    heaplens_main_window_apply_theme(state);
    return window;
}
