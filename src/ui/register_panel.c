#include "register_panel.h"

#include <string.h>

typedef struct {
    GtkWidget *text;
} HeapLensRegisterPanelState;

static void heaplens_register_panel_destroy(gpointer data) {
    g_free(data);
}

GtkWidget *heaplens_register_panel_new(void) {
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *title = gtk_label_new("Register Inspector");
    GtkWidget *scroller = gtk_scrolled_window_new();
    GtkWidget *text = gtk_text_view_new();
    HeapLensRegisterPanelState *state = g_new0(HeapLensRegisterPanelState, 1);

    state->text = text;
    g_object_set_data_full(G_OBJECT(root), "heaplens-register-state", state, heaplens_register_panel_destroy);

    gtk_widget_add_css_class(root, "card");
    gtk_widget_add_css_class(title, "title");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(text), TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), text);
    gtk_widget_set_vexpand(scroller, TRUE);

    gtk_box_append(GTK_BOX(root), title);
    gtk_box_append(GTK_BOX(root), scroller);
    return root;
}

void heaplens_register_panel_set_snapshot(GtkWidget *panel, const HeapLensRegisterSnapshot *snapshot) {
    HeapLensRegisterPanelState *state = g_object_get_data(G_OBJECT(panel), "heaplens-register-state");
    GtkTextBuffer *buffer;
    GString *text;
    char flags[128];
    int index = 0;

    if (!panel || !state) {
        return;
    }

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text));
    text = g_string_new("");

    if (!snapshot) {
        gtk_text_buffer_set_text(buffer, "No register snapshot loaded.", -1);
        g_string_free(text, TRUE);
        return;
    }

    heaplens_ptrace_format_rflags(snapshot->gpr.eflags, flags, sizeof(flags));
    g_string_append_printf(text,
                           "RIP=%#llx  RSP=%#llx  RBP=%#llx\n"
                           "RAX=%#llx  RBX=%#llx  RCX=%#llx  RDX=%#llx\n"
                           "RSI=%#llx  RDI=%#llx  R8=%#llx   R9=%#llx\n"
                           "R10=%#llx R11=%#llx R12=%#llx R13=%#llx\n"
                           "R14=%#llx R15=%#llx ORIG_RAX=%#llx\n"
                           "RFLAGS: %s\n",
                           (unsigned long long) snapshot->gpr.rip,
                           (unsigned long long) snapshot->gpr.rsp,
                           (unsigned long long) snapshot->gpr.rbp,
                           (unsigned long long) snapshot->gpr.rax,
                           (unsigned long long) snapshot->gpr.rbx,
                           (unsigned long long) snapshot->gpr.rcx,
                           (unsigned long long) snapshot->gpr.rdx,
                           (unsigned long long) snapshot->gpr.rsi,
                           (unsigned long long) snapshot->gpr.rdi,
                           (unsigned long long) snapshot->gpr.r8,
                           (unsigned long long) snapshot->gpr.r9,
                           (unsigned long long) snapshot->gpr.r10,
                           (unsigned long long) snapshot->gpr.r11,
                           (unsigned long long) snapshot->gpr.r12,
                           (unsigned long long) snapshot->gpr.r13,
                           (unsigned long long) snapshot->gpr.r14,
                           (unsigned long long) snapshot->gpr.r15,
                           (unsigned long long) snapshot->gpr.orig_rax,
                           flags);

    if (snapshot->have_fpregs) {
        for (index = 0; index < 8; ++index) {
            const unsigned int *base = &snapshot->fpregs.xmm_space[index * 4];
            unsigned long long low = ((unsigned long long) base[1] << 32) | base[0];
            unsigned long long high = ((unsigned long long) base[3] << 32) | base[2];
            g_string_append_printf(text, "XMM%d=%016llx:%016llx\n", index, high, low);
        }
    } else {
        g_string_append(text, "XMM registers unavailable for this snapshot.\n");
    }

    gtk_text_buffer_set_text(buffer, text->str, -1);
    g_string_free(text, TRUE);
}
