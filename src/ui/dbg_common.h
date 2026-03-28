#ifndef HEAPLENS_UI_DBG_COMMON_H
#define HEAPLENS_UI_DBG_COMMON_H

#include <gtk/gtk.h>

typedef struct {
    int width;
    int height;
    gboolean pinned;
} HeapLensDebuggerGeometry;

void heaplens_dbg_window_init(GtkWindow *window,
                              GtkWindow *parent,
                              const char *title,
                              const char *config_key,
                              int default_width,
                              int default_height);
void heaplens_dbg_window_save(GtkWindow *window, const char *config_key);
HeapLensDebuggerGeometry heaplens_dbg_window_load(const char *config_key,
                                                  int default_width,
                                                  int default_height);
char *heaplens_dbg_format_hex_dump(const guint8 *bytes, size_t size, uint64_t base_address);
void heaplens_dbg_apply_pinned_state(GtkWindow *window, GtkToggleButton *button);

#endif
