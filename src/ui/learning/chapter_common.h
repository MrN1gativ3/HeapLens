#ifndef HEAPLENS_UI_LEARNING_CHAPTER_COMMON_H
#define HEAPLENS_UI_LEARNING_CHAPTER_COMMON_H

#include <gtk/gtk.h>

GtkWidget *heaplens_learning_build_chapter_page(const char *title,
                                                const char *summary,
                                                const char *const topics[],
                                                size_t topic_count,
                                                const char *interactive_note,
                                                const char *code_block,
                                                GtkWidget *extra_widget);

#endif
