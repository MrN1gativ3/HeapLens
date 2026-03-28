#include <gtk/gtk.h>

#include "ui/main_window.h"

static void heaplens_activate(GtkApplication *application, gpointer user_data) {
    GtkWidget *window = heaplens_main_window_new(application);
    (void) user_data;
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *application = gtk_application_new("com.heaplens.app", G_APPLICATION_DEFAULT_FLAGS);
    int status = 0;

#if !defined(__linux__) || !defined(__x86_64__)
    g_printerr("HeapLens targets Linux x86_64 only.\n");
#endif

    g_signal_connect(application, "activate", G_CALLBACK(heaplens_activate), NULL);
    status = g_application_run(G_APPLICATION(application), argc, argv);
    g_object_unref(application);
    return status;
}
