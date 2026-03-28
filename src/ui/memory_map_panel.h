#ifndef HEAPLENS_UI_MEMORY_MAP_PANEL_H
#define HEAPLENS_UI_MEMORY_MAP_PANEL_H

#include <gtk/gtk.h>
#include <sys/types.h>

GtkWidget *heaplens_memory_map_panel_new(void);
void heaplens_memory_map_panel_load_pid(GtkWidget *panel, pid_t pid);

#endif
