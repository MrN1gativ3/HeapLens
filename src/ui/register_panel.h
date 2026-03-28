#ifndef HEAPLENS_UI_REGISTER_PANEL_H
#define HEAPLENS_UI_REGISTER_PANEL_H

#include <gtk/gtk.h>

#include "../core/ptrace_backend.h"

GtkWidget *heaplens_register_panel_new(void);
void heaplens_register_panel_set_snapshot(GtkWidget *panel, const HeapLensRegisterSnapshot *snapshot);

#endif
