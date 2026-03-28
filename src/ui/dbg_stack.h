#ifndef HEAPLENS_UI_DBG_STACK_H
#define HEAPLENS_UI_DBG_STACK_H

#include <gtk/gtk.h>

#include "../core/process_manager.h"

GtkWidget *heaplens_dbg_stack_new(GtkWindow *parent);
void heaplens_dbg_stack_set_snapshot(GtkWidget *window,
                                     HeapLensTargetProcess *process,
                                     const HeapLensRegisterSnapshot *snapshot);
void heaplens_dbg_stack_clear(GtkWidget *window);

#endif
