#ifndef HEAPLENS_UI_DISASM_PANEL_H
#define HEAPLENS_UI_DISASM_PANEL_H

#include <gtk/gtk.h>

GtkWidget *heaplens_disasm_panel_new(void);
void heaplens_disasm_panel_set_text(GtkWidget *panel, const char *text);

#endif
