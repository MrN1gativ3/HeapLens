#include "main_window.h"

#include "tab_exploit.h"
#include "tab_learning.h"

GtkWidget *heaplens_main_window_new(GtkApplication *application) {
    GtkWidget *window = gtk_application_window_new(application);
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    GtkWidget *topbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *titles = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *title = gtk_label_new("HeapLens");
    GtkWidget *subtitle = gtk_label_new("Interactive glibc heap internals and exploitation lab for Linux x86_64");
    GtkWidget *stack = gtk_stack_new();
    GtkWidget *switcher = gtk_stack_switcher_new();
    GtkCssProvider *provider = gtk_css_provider_new();

    gtk_window_set_title(GTK_WINDOW(window), "HeapLens");
    gtk_window_set_default_size(GTK_WINDOW(window), 1600, 980);
    gtk_widget_add_css_class(root, "panel");
    gtk_widget_add_css_class(title, "title");
    gtk_widget_add_css_class(subtitle, "subtitle");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(subtitle), 0.0f);
    gtk_box_append(GTK_BOX(titles), title);
    gtk_box_append(GTK_BOX(titles), subtitle);

    gtk_stack_add_titled(GTK_STACK(stack), heaplens_tab_learning_new(), "learning", "glibc Heap Internals");
    gtk_stack_add_titled(GTK_STACK(stack), heaplens_tab_exploit_new(), "exploit", "Exploitation Lab");
    gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcher), GTK_STACK(stack));
    gtk_widget_set_hexpand(stack, TRUE);
    gtk_widget_set_vexpand(stack, TRUE);

    gtk_box_append(GTK_BOX(topbar), titles);
    gtk_box_append(GTK_BOX(topbar), switcher);
    gtk_box_append(GTK_BOX(root), topbar);
    gtk_box_append(GTK_BOX(root), stack);
    gtk_window_set_child(GTK_WINDOW(window), root);

    gtk_css_provider_load_from_path(provider, "assets/styles/dark.css");
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                               GTK_STYLE_PROVIDER(provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    return window;
}
