#ifndef HEAPLENS_UI_SYSCALL_TRACE_H
#define HEAPLENS_UI_SYSCALL_TRACE_H

#include <gtk/gtk.h>

GtkWidget *heaplens_syscall_trace_new(void);
void heaplens_syscall_trace_append(GtkWidget *panel, const char *line);
void heaplens_syscall_trace_clear(GtkWidget *panel);

#endif
