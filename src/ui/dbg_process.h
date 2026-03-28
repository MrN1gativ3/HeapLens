#ifndef HEAPLENS_UI_DBG_PROCESS_H
#define HEAPLENS_UI_DBG_PROCESS_H

#include <gtk/gtk.h>

#include "../core/process_manager.h"

GtkWidget *heaplens_dbg_process_new(GtkWindow *parent);
void heaplens_dbg_process_set_target(GtkWidget *window, HeapLensTargetProcess *process);
void heaplens_dbg_process_clear(GtkWidget *window);

#endif
