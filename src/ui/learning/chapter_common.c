#include "chapter_common.h"

#include <stdio.h>

GtkWidget *heaplens_learning_build_chapter_page(const char *title,
                                                const char *summary,
                                                const char *const topics[],
                                                size_t topic_count,
                                                const char *interactive_note,
                                                const char *code_block,
                                                GtkWidget *extra_widget) {
    GtkWidget *scroller = gtk_scrolled_window_new();
    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    GtkWidget *title_label = gtk_label_new(title);
    GtkWidget *summary_label = gtk_label_new(summary);
    size_t index = 0;

    gtk_widget_add_css_class(content, "card");
    gtk_widget_add_css_class(title_label, "title");
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(summary_label), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(summary_label), TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), content);
    gtk_widget_set_hexpand(scroller, TRUE);
    gtk_widget_set_vexpand(scroller, TRUE);

    gtk_box_append(GTK_BOX(content), title_label);
    gtk_box_append(GTK_BOX(content), summary_label);

    for (index = 0; index < topic_count; ++index) {
        GtkWidget *topic = gtk_label_new(NULL);
        char bullet[1024];
        snprintf(bullet, sizeof(bullet), "- %s", topics[index]);
        gtk_label_set_text(GTK_LABEL(topic), bullet);
        gtk_label_set_xalign(GTK_LABEL(topic), 0.0f);
        gtk_label_set_wrap(GTK_LABEL(topic), TRUE);
        gtk_box_append(GTK_BOX(content), topic);
    }

    if (interactive_note && interactive_note[0]) {
        GtkWidget *interactive = gtk_label_new(interactive_note);
        gtk_label_set_xalign(GTK_LABEL(interactive), 0.0f);
        gtk_label_set_wrap(GTK_LABEL(interactive), TRUE);
        gtk_widget_add_css_class(interactive, "subtitle");
        gtk_box_append(GTK_BOX(content), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_box_append(GTK_BOX(content), interactive);
    }

    if (code_block && code_block[0]) {
        GtkWidget *code = gtk_label_new(code_block);
        gtk_label_set_xalign(GTK_LABEL(code), 0.0f);
        gtk_label_set_selectable(GTK_LABEL(code), TRUE);
        gtk_label_set_wrap(GTK_LABEL(code), TRUE);
        gtk_box_append(GTK_BOX(content), code);
    }

    if (extra_widget) {
        gtk_box_append(GTK_BOX(content), extra_widget);
    }

    return scroller;
}
