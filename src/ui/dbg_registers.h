#ifndef HEAPLENS_UI_DBG_REGISTERS_H
#define HEAPLENS_UI_DBG_REGISTERS_H

#include <gtk/gtk.h>

#include "../core/process_manager.h"

GtkWidget *heaplens_dbg_registers_new(GtkWindow *parent);
void heaplens_dbg_registers_set_snapshot(GtkWidget *window,
                                         HeapLensTargetProcess *process,
                                         const HeapLensRegisterSnapshot *snapshot,
                                         const HeapLensHeapSnapshot *heap_snapshot);
void heaplens_dbg_registers_clear(GtkWidget *window);

#endif
