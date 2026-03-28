#ifndef HEAPLENS_UI_THEME_H
#define HEAPLENS_UI_THEME_H

#include <gtk/gtk.h>
#include <vte/vte.h>

guint heaplens_theme_count(void);
guint heaplens_theme_default_index(void);
const char *heaplens_theme_get_label(guint index);
void heaplens_theme_apply_css(GtkCssProvider *provider, guint index);
void heaplens_theme_apply_terminal(VteTerminal *terminal, guint index);
const char *heaplens_theme_css_path(void);

#endif
