#ifndef HEAPLENS_UI_THEORY_SIDEBAR_H
#define HEAPLENS_UI_THEORY_SIDEBAR_H

#include <gtk/gtk.h>

GtkWidget *heaplens_theory_sidebar_new(void);
void heaplens_theory_sidebar_set_text(GtkWidget *panel, const char *text);

#endif
