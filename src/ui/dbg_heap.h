#ifndef HEAPLENS_UI_DBG_HEAP_H
#define HEAPLENS_UI_DBG_HEAP_H

#include <gtk/gtk.h>

#include "../core/process_manager.h"

GtkWidget *heaplens_dbg_heap_new(GtkWindow *parent);
void heaplens_dbg_heap_set_snapshot(GtkWidget *window,
                                    HeapLensTargetProcess *process,
                                    const HeapLensHeapSnapshot *snapshot);
void heaplens_dbg_heap_clear(GtkWidget *window);
gboolean heaplens_dbg_heap_search(GtkWidget *window, const char *query);

#endif
