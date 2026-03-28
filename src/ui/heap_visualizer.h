#ifndef HEAPLENS_UI_HEAP_VISUALIZER_H
#define HEAPLENS_UI_HEAP_VISUALIZER_H

#include <gtk/gtk.h>

#include "../core/heap_parser.h"

GtkWidget *heaplens_heap_visualizer_new(void);
void heaplens_heap_visualizer_set_snapshot(GtkWidget *panel, const HeapLensHeapSnapshot *snapshot);
void heaplens_heap_visualizer_clear(GtkWidget *panel);

#endif
