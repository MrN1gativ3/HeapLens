#include "dbg_registers.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "../core/symbol_resolver.h"
#include "dbg_common.h"

typedef struct {
    GtkWidget *name_label;
    GtkWidget *value_button;
    GtkWidget *value_label;
    GtkWidget *detail_label;
} HeapLensRegisterRow;

typedef struct {
    GtkWidget *rip_label;
    GtkWidget *dump_view;
    GtkWidget *dump_popover;
    GtkWidget *flag_pills[9];
    GtkWidget *segment_label;
    GtkWidget *xmm_label;
    HeapLensRegisterRow rows[17];
    HeapLensTargetProcess *process;
    HeapLensRegisterSnapshot current;
    HeapLensRegisterSnapshot previous;
    HeapLensHeapSnapshot heap;
    HeapLensMemoryMapSnapshot maps;
} HeapLensDbgRegistersState;

static const char *heaplens_register_names[] = {
    "RAX", "RBX", "RCX", "RDX", "RSI", "RDI", "RBP", "RSP",
    "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15", "RIP"
};

static guint64 heaplens_dbg_register_get_value(const HeapLensRegisterSnapshot *snapshot, size_t index) {
    if (!snapshot) {
        return 0;
    }

    switch (index) {
        case 0: return snapshot->gpr.rax;
        case 1: return snapshot->gpr.rbx;
        case 2: return snapshot->gpr.rcx;
        case 3: return snapshot->gpr.rdx;
        case 4: return snapshot->gpr.rsi;
        case 5: return snapshot->gpr.rdi;
        case 6: return snapshot->gpr.rbp;
        case 7: return snapshot->gpr.rsp;
        case 8: return snapshot->gpr.r8;
        case 9: return snapshot->gpr.r9;
        case 10: return snapshot->gpr.r10;
        case 11: return snapshot->gpr.r11;
        case 12: return snapshot->gpr.r12;
        case 13: return snapshot->gpr.r13;
        case 14: return snapshot->gpr.r14;
        case 15: return snapshot->gpr.r15;
        case 16: return snapshot->gpr.rip;
        default: return 0;
    }
}

static HeapLensDbgRegistersState *heaplens_dbg_registers_state(GtkWidget *window) {
    return window ? g_object_get_data(G_OBJECT(window), "heaplens-dbg-registers-state") : NULL;
}

static void heaplens_dbg_registers_heap_copy(HeapLensHeapSnapshot *dst, const HeapLensHeapSnapshot *src) {
    size_t index = 0;

    memset(dst, 0, sizeof(*dst));
    if (!src) {
        return;
    }

    *dst = *src;
    dst->chunks = NULL;
    if (src->chunk_count == 0) {
        return;
    }

    dst->chunks = g_new0(HeapLensChunkInfo, src->chunk_count);
    for (index = 0; index < src->chunk_count; ++index) {
        dst->chunks[index] = src->chunks[index];
    }
}

static void heaplens_dbg_registers_state_free(gpointer data) {
    HeapLensDbgRegistersState *state = data;

    if (!state) {
        return;
    }

    heaplens_heap_snapshot_free(&state->heap);
    heaplens_memory_map_snapshot_free(&state->maps);
    g_free(state);
}

static const char *heaplens_dbg_register_classify(HeapLensDbgRegistersState *state, guint64 value) {
    const HeapLensMemoryMapEntry *map = NULL;

    if (value == 0) {
        return "dim";
    }
    if (value >= state->heap.heap_start && value < state->heap.heap_end) {
        return "green";
    }

    map = heaplens_memory_map_find_containing(&state->maps, value);
    if (map) {
        if (strstr(map->pathname, "libc")) {
            return "purple";
        }
        if (strstr(map->pathname, "[stack]")) {
            return "blue";
        }
    }
    return NULL;
}

static void heaplens_dbg_register_ascii(guint64 value, char *buffer, size_t buffer_size) {
    int index = 0;

    if (!buffer || buffer_size < 9) {
        return;
    }

    for (index = 0; index < 8; ++index) {
        guint8 ch = (guint8) ((value >> (index * 8)) & 0xff);
        buffer[index] = isprint(ch) ? (char) ch : '.';
    }
    buffer[8] = '\0';
}

static void heaplens_dbg_register_show_dump(GtkButton *button, gpointer user_data) {
    HeapLensDbgRegistersState *state = user_data;
    guint64 *value_ptr = NULL;
    guint64 value = 0;
    guint8 bytes[256];
    char *dump = NULL;
    char text[32];

    if (!state || !state->process) {
        return;
    }

    value_ptr = g_object_get_data(G_OBJECT(button), "heaplens-value");
    if (!value_ptr) {
        return;
    }
    value = *value_ptr;
    if (!value || !heaplens_memory_reader_read_fully(&state->process->reader, value, bytes, sizeof(bytes))) {
        gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->dump_view)),
                                 "Pointer is not readable in the current process.",
                                 -1);
    } else {
        dump = heaplens_dbg_format_hex_dump(bytes, sizeof(bytes), value);
        gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->dump_view)), dump, -1);
        g_free(dump);
    }

    if (gdk_display_get_default()) {
        g_snprintf(text, sizeof(text), "0x%llx", (unsigned long long) value);
        gdk_clipboard_set_text(gdk_display_get_clipboard(gdk_display_get_default()), text);
    }
    gtk_popover_popup(GTK_POPOVER(state->dump_popover));
}

static void heaplens_dbg_register_update_flags(HeapLensDbgRegistersState *state) {
    static const struct {
        const char *name;
        guint64 mask;
    } flags[] = {
        {"CF", 1ULL << 0},
        {"PF", 1ULL << 2},
        {"AF", 1ULL << 4},
        {"ZF", 1ULL << 6},
        {"SF", 1ULL << 7},
        {"TF", 1ULL << 8},
        {"IF", 1ULL << 9},
        {"DF", 1ULL << 10},
        {"OF", 1ULL << 11}
    };
    size_t index = 0;

    for (index = 0; index < G_N_ELEMENTS(flags); ++index) {
        char text[32];
        gboolean active = (state->current.gpr.eflags & flags[index].mask) != 0;

        g_snprintf(text, sizeof(text), "%s:%d", flags[index].name, active ? 1 : 0);
        gtk_label_set_text(GTK_LABEL(state->flag_pills[index]), text);
        gtk_widget_remove_css_class(state->flag_pills[index], "active");
        gtk_widget_remove_css_class(state->flag_pills[index], "inactive");
        gtk_widget_add_css_class(state->flag_pills[index], active ? "active" : "inactive");
    }
}

static void heaplens_dbg_register_update_xmm(HeapLensDbgRegistersState *state) {
    GString *text = g_string_new("");
    int index = 0;

    if (!state->current.have_fpregs) {
        gtk_label_set_text(GTK_LABEL(state->xmm_label), "XMM registers unavailable.");
        g_string_free(text, TRUE);
        return;
    }

    for (index = 0; index < 8; ++index) {
        const unsigned int *base = &state->current.fpregs.xmm_space[index * 4];
        unsigned long long low = ((unsigned long long) base[1] << 32) | base[0];
        unsigned long long high = ((unsigned long long) base[3] << 32) | base[2];
        g_string_append_printf(text, "XMM%d: %016llx %016llx\n", index, high, low);
    }

    gtk_label_set_text(GTK_LABEL(state->xmm_label), text->str);
    g_string_free(text, TRUE);
}

static void heaplens_dbg_register_update_widgets(HeapLensDbgRegistersState *state) {
    size_t index = 0;
    HeapLensResolvedSymbol symbol;
    const HeapLensMemoryMapEntry *rip_map = heaplens_memory_map_find_containing(&state->maps, state->current.gpr.rip);
    char rip_text[256];

    if (rip_map && rip_map->pathname[0] &&
        heaplens_symbol_resolve_address(rip_map->pathname, rip_map->start, state->current.gpr.rip, &symbol)) {
        g_snprintf(rip_text,
                   sizeof(rip_text),
                   "0x%016llx  <%s+0x%llx>",
                   (unsigned long long) state->current.gpr.rip,
                   symbol.name,
                   (unsigned long long) symbol.offset);
    } else {
        g_snprintf(rip_text, sizeof(rip_text), "0x%016llx", (unsigned long long) state->current.gpr.rip);
    }
    gtk_label_set_text(GTK_LABEL(state->rip_label), rip_text);

    for (index = 0; index < G_N_ELEMENTS(heaplens_register_names); ++index) {
        guint64 value = heaplens_dbg_register_get_value(&state->current, index);
        guint64 previous = heaplens_dbg_register_get_value(&state->previous, index);
        char value_text[32];
        char detail[192];
        char ascii[16];
        const char *klass = heaplens_dbg_register_classify(state, value);
        gboolean changed = value != previous;
        guint64 *stored_value = g_object_get_data(G_OBJECT(state->rows[index].value_button), "heaplens-value");

        g_snprintf(value_text, sizeof(value_text), "0x%016llx", (unsigned long long) value);
        heaplens_dbg_register_ascii(value, ascii, sizeof(ascii));
        g_snprintf(detail,
                   sizeof(detail),
                   "hex:   0x%016llx\ndec:   %llu\nascii: \"%s\"",
                   (unsigned long long) value,
                   (unsigned long long) value,
                   ascii);

        gtk_label_set_text(GTK_LABEL(state->rows[index].value_label), value_text);
        gtk_label_set_text(GTK_LABEL(state->rows[index].detail_label), detail);
        if (!stored_value) {
            stored_value = g_new0(guint64, 1);
            g_object_set_data_full(G_OBJECT(state->rows[index].value_button), "heaplens-value", stored_value, g_free);
        }
        *stored_value = value;

        gtk_widget_remove_css_class(state->rows[index].value_label, "yellow");
        gtk_widget_remove_css_class(state->rows[index].value_label, "green");
        gtk_widget_remove_css_class(state->rows[index].value_label, "purple");
        gtk_widget_remove_css_class(state->rows[index].value_label, "blue");
        gtk_widget_remove_css_class(state->rows[index].value_label, "dim");
        gtk_widget_remove_css_class(state->rows[index].value_label, "red");
        if (changed) {
            gtk_widget_add_css_class(state->rows[index].value_label, "yellow");
        } else if (klass) {
            gtk_widget_add_css_class(state->rows[index].value_label, klass);
        }
    }

    heaplens_dbg_register_update_flags(state);
    heaplens_dbg_register_update_xmm(state);

    {
        char segments[128];
        g_snprintf(segments,
                   sizeof(segments),
                   "CS: 0x%02llx   DS: 0x%02llx   SS: 0x%02llx   ES: 0x%02llx   FS: 0x%02llx   GS: 0x%02llx",
                   (unsigned long long) state->current.gpr.cs,
                   (unsigned long long) state->current.gpr.ds,
                   (unsigned long long) state->current.gpr.ss,
                   (unsigned long long) state->current.gpr.es,
                   (unsigned long long) state->current.gpr.fs,
                   (unsigned long long) state->current.gpr.gs);
        gtk_label_set_text(GTK_LABEL(state->segment_label), segments);
    }
}

GtkWidget *heaplens_dbg_registers_new(GtkWindow *parent) {
    GtkWidget *window = gtk_window_new();
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *rip_banner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *rip_label = gtk_label_new("0x0000000000000000");
    GtkWidget *gpr_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *gpr_title = gtk_label_new("General Registers");
    GtkWidget *grid = gtk_grid_new();
    GtkWidget *flags_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *segments_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *segments_title = gtk_label_new("Segments");
    GtkWidget *segment_label = gtk_label_new("CS: 0x00   DS: 0x00   SS: 0x00   ES: 0x00   FS: 0x00   GS: 0x00");
    GtkWidget *xmm_expander = gtk_expander_new("XMM Registers");
    GtkWidget *xmm_label = gtk_label_new("XMM registers unavailable.");
    GtkWidget *dump_popover = gtk_popover_new();
    GtkWidget *dump_scroller = gtk_scrolled_window_new();
    GtkWidget *dump_view = gtk_text_view_new();
    HeapLensDbgRegistersState *state = g_new0(HeapLensDbgRegistersState, 1);
    size_t index = 0;

    heaplens_dbg_window_init(GTK_WINDOW(window), parent, "Registers", "registers", 380, 560);
    gtk_widget_set_margin_top(root, 8);
    gtk_widget_set_margin_bottom(root, 8);
    gtk_widget_set_margin_start(root, 8);
    gtk_widget_set_margin_end(root, 8);

    gtk_widget_add_css_class(rip_banner, "rip-banner");
    gtk_widget_add_css_class(rip_label, "monospace");
    gtk_widget_add_css_class(gpr_panel, "panel");
    gtk_widget_add_css_class(gpr_title, "panel-heading");
    gtk_widget_add_css_class(segments_panel, "panel");
    gtk_widget_add_css_class(segments_title, "panel-heading");
    gtk_label_set_xalign(GTK_LABEL(rip_label), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(gpr_title), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(segments_title), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(segment_label), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(segment_label), TRUE);

    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    for (index = 0; index < G_N_ELEMENTS(heaplens_register_names); ++index) {
        GtkWidget *name = gtk_label_new(heaplens_register_names[index]);
        GtkWidget *value_button = gtk_button_new();
        GtkWidget *value_label = gtk_label_new("0x0000000000000000");
        GtkWidget *detail = gtk_label_new("hex: 0x0\ndec: 0\nascii: \"........\"");

        gtk_widget_add_css_class(name, "secondary");
        gtk_widget_add_css_class(value_label, "monospace");
        gtk_widget_add_css_class(detail, "monospace");
        gtk_label_set_xalign(GTK_LABEL(name), 0.0f);
        gtk_label_set_xalign(GTK_LABEL(value_label), 0.0f);
        gtk_label_set_xalign(GTK_LABEL(detail), 0.0f);
        gtk_label_set_wrap(GTK_LABEL(detail), TRUE);
        gtk_button_set_child(GTK_BUTTON(value_button), value_label);
        g_signal_connect(value_button, "clicked", G_CALLBACK(heaplens_dbg_register_show_dump), state);

        gtk_grid_attach(GTK_GRID(grid), name, 0, (int) index, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), value_button, 1, (int) index, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), detail, 2, (int) index, 1, 1);

        state->rows[index].name_label = name;
        state->rows[index].value_button = value_button;
        state->rows[index].value_label = value_label;
        state->rows[index].detail_label = detail;
    }

    for (index = 0; index < G_N_ELEMENTS(state->flag_pills); ++index) {
        state->flag_pills[index] = gtk_label_new("CF:0");
        gtk_widget_add_css_class(state->flag_pills[index], "flag-pill");
        gtk_box_append(GTK_BOX(flags_box), state->flag_pills[index]);
    }

    gtk_text_view_set_editable(GTK_TEXT_VIEW(dump_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(dump_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(dump_view), TRUE);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(dump_scroller), TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(dump_scroller), dump_view);
    gtk_widget_set_size_request(dump_scroller, 420, 200);
    gtk_popover_set_child(GTK_POPOVER(dump_popover), dump_scroller);
    gtk_widget_set_parent(dump_popover, root);

    gtk_box_append(GTK_BOX(rip_banner), rip_label);
    gtk_box_append(GTK_BOX(gpr_panel), gpr_title);
    gtk_box_append(GTK_BOX(gpr_panel), grid);
    gtk_box_append(GTK_BOX(segments_panel), segments_title);
    gtk_box_append(GTK_BOX(segments_panel), segment_label);
    gtk_widget_add_css_class(xmm_label, "monospace");
    gtk_label_set_xalign(GTK_LABEL(xmm_label), 0.0f);
    gtk_expander_set_child(GTK_EXPANDER(xmm_expander), xmm_label);

    gtk_box_append(GTK_BOX(root), rip_banner);
    gtk_box_append(GTK_BOX(root), gpr_panel);
    gtk_box_append(GTK_BOX(root), flags_box);
    gtk_box_append(GTK_BOX(root), segments_panel);
    gtk_box_append(GTK_BOX(root), xmm_expander);
    gtk_window_set_child(GTK_WINDOW(window), root);

    state->rip_label = rip_label;
    state->dump_view = dump_view;
    state->dump_popover = dump_popover;
    state->segment_label = segment_label;
    state->xmm_label = xmm_label;
    g_object_set_data_full(G_OBJECT(window), "heaplens-dbg-registers-state", state, heaplens_dbg_registers_state_free);
    return window;
}

void heaplens_dbg_registers_set_snapshot(GtkWidget *window,
                                         HeapLensTargetProcess *process,
                                         const HeapLensRegisterSnapshot *snapshot,
                                         const HeapLensHeapSnapshot *heap_snapshot) {
    HeapLensDbgRegistersState *state = heaplens_dbg_registers_state(window);

    if (!state || !process || !snapshot) {
        return;
    }

    state->process = process;
    state->previous = state->current;
    state->current = *snapshot;
    heaplens_heap_snapshot_free(&state->heap);
    heaplens_dbg_registers_heap_copy(&state->heap, heap_snapshot);
    heaplens_memory_map_snapshot_free(&state->maps);
    memset(&state->maps, 0, sizeof(state->maps));
    heaplens_memory_reader_read_maps(process->pid, &state->maps);
    heaplens_dbg_register_update_widgets(state);
}

void heaplens_dbg_registers_clear(GtkWidget *window) {
    HeapLensDbgRegistersState *state = heaplens_dbg_registers_state(window);

    if (!state) {
        return;
    }

    state->process = NULL;
    memset(&state->current, 0, sizeof(state->current));
    memset(&state->previous, 0, sizeof(state->previous));
    heaplens_heap_snapshot_free(&state->heap);
    heaplens_memory_map_snapshot_free(&state->maps);
    heaplens_dbg_register_update_widgets(state);
}
