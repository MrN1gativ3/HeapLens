#ifndef HEAPLENS_UI_TOP_BAR_H
#define HEAPLENS_UI_TOP_BAR_H

#include <gtk/gtk.h>

typedef enum {
    HEAPLENS_TOP_ACTION_RESET = 0,
    HEAPLENS_TOP_ACTION_RUN,
    HEAPLENS_TOP_ACTION_PAUSE,
    HEAPLENS_TOP_ACTION_STEP,
    HEAPLENS_TOP_ACTION_STEP_OVER,
    HEAPLENS_TOP_ACTION_STEP_OUT,
    HEAPLENS_TOP_ACTION_AUTORUN,
    HEAPLENS_TOP_ACTION_SNAPSHOT,
    HEAPLENS_TOP_ACTION_SEARCH,
    HEAPLENS_TOP_ACTION_COPY,
    HEAPLENS_TOP_ACTION_SETTINGS,
    HEAPLENS_TOP_ACTION_TERMINAL,
    HEAPLENS_TOP_ACTION_WINDOW_HEAP,
    HEAPLENS_TOP_ACTION_WINDOW_STACK,
    HEAPLENS_TOP_ACTION_WINDOW_PROCESS,
    HEAPLENS_TOP_ACTION_WINDOW_REGS,
    HEAPLENS_TOP_ACTION_COUNT
} HeapLensTopBarAction;

typedef enum {
    HEAPLENS_MITIGATION_ASLR = 0,
    HEAPLENS_MITIGATION_PIE,
    HEAPLENS_MITIGATION_RELRO,
    HEAPLENS_MITIGATION_NX,
    HEAPLENS_MITIGATION_CANARY,
    HEAPLENS_MITIGATION_SAFE_LINK,
    HEAPLENS_MITIGATION_TCACHE_KEY,
    HEAPLENS_MITIGATION_VTABLE,
    HEAPLENS_MITIGATION_COUNT
} HeapLensMitigation;

GtkWidget *heaplens_top_bar_new(void);
GtkDropDown *heaplens_top_bar_get_technique_dropdown(GtkWidget *bar);
GtkDropDown *heaplens_top_bar_get_glibc_dropdown(GtkWidget *bar);
GtkScale *heaplens_top_bar_get_autorun_scale(GtkWidget *bar);
GtkWidget *heaplens_top_bar_get_action_button(GtkWidget *bar, HeapLensTopBarAction action);
void heaplens_top_bar_set_action_active(GtkWidget *bar, HeapLensTopBarAction action, gboolean active);
void heaplens_top_bar_set_mitigation(GtkWidget *bar,
                                     HeapLensMitigation mitigation,
                                     gboolean active,
                                     const char *summary,
                                     gboolean requires_bypass);

#endif
