#ifndef HEAPLENS_UI_TERMINAL_WIDGET_H
#define HEAPLENS_UI_TERMINAL_WIDGET_H

#include <gtk/gtk.h>
#include <vte/vte.h>

GtkWidget *heaplens_terminal_widget_new(void);
void heaplens_terminal_widget_print_banner(GtkWidget *panel, const char *banner);
void heaplens_terminal_widget_apply_theme(GtkWidget *panel, guint theme_index);
VteTerminal *heaplens_terminal_widget_get_terminal(GtkWidget *panel);

#endif
