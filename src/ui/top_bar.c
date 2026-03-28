#include "top_bar.h"

#include <string.h>

typedef struct {
    GtkWidget *root;
    GtkWidget *technique_dropdown;
    GtkWidget *glibc_dropdown;
    GtkWidget *autorun_scale;
    GtkWidget *actions[HEAPLENS_TOP_ACTION_COUNT];
    GtkWidget *badges[HEAPLENS_MITIGATION_COUNT];
    GtkWidget *badge_details[HEAPLENS_MITIGATION_COUNT];
} HeapLensTopBarState;

static GtkWidget *heaplens_top_bar_labeled_button(const char *icon_name,
                                                  const char *label,
                                                  const char *tooltip,
                                                  gboolean toggle) {
    GtkWidget *button = toggle ? gtk_toggle_button_new() : gtk_button_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *icon = gtk_image_new_from_icon_name(icon_name);

    gtk_widget_add_css_class(button, "icon-button");
    gtk_widget_set_tooltip_text(button, tooltip);
    gtk_widget_set_valign(button, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 16);
    gtk_box_append(GTK_BOX(box), icon);

    if (label && label[0]) {
        GtkWidget *text = gtk_label_new(label);
        gtk_box_append(GTK_BOX(box), text);
    }

    gtk_button_set_child(GTK_BUTTON(button), box);
    return button;
}

static GtkWidget *heaplens_top_bar_icon_button(const char *icon_name,
                                               const char *tooltip,
                                               gboolean toggle) {
    GtkWidget *button = toggle ? gtk_toggle_button_new() : gtk_button_new();
    GtkWidget *icon = gtk_image_new_from_icon_name(icon_name);

    gtk_widget_add_css_class(button, "icon-button");
    gtk_widget_set_tooltip_text(button, tooltip);
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 16);
    gtk_button_set_child(GTK_BUTTON(button), icon);
    return button;
}

static GtkWidget *heaplens_top_bar_build_preferences_popover(const char *title) {
    GtkWidget *popover = gtk_popover_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget *heading = gtk_label_new(title);
    GtkWidget *theme = gtk_label_new("Theme: Mission Center dark");
    GtkWidget *layout = gtk_label_new("Layout: compact GTK4 workspace");
    GtkWidget *terminal = gtk_label_new("Terminal: toggle with Ctrl+T");

    gtk_widget_add_css_class(popover, "settings-popover");
    gtk_widget_set_margin_top(box, 8);
    gtk_widget_set_margin_bottom(box, 8);
    gtk_widget_set_margin_start(box, 8);
    gtk_widget_set_margin_end(box, 8);
    gtk_label_set_xalign(GTK_LABEL(heading), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(theme), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(layout), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(terminal), 0.0f);
    gtk_widget_add_css_class(heading, "accent-text");
    gtk_widget_add_css_class(theme, "secondary");
    gtk_widget_add_css_class(layout, "secondary");
    gtk_widget_add_css_class(terminal, "secondary");

    gtk_box_append(GTK_BOX(box), heading);
    gtk_box_append(GTK_BOX(box), theme);
    gtk_box_append(GTK_BOX(box), layout);
    gtk_box_append(GTK_BOX(box), terminal);
    gtk_popover_set_child(GTK_POPOVER(popover), box);
    return popover;
}

static GtkWidget *heaplens_top_bar_build_badge(const char *name, GtkWidget **detail_label_out) {
    GtkWidget *button = gtk_button_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *label = gtk_label_new(name);
    GtkWidget *dot = gtk_label_new("●");
    GtkWidget *popover = gtk_popover_new();
    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget *detail_title = gtk_label_new(name);
    GtkWidget *detail = gtk_label_new("Mitigation details unavailable.");

    gtk_widget_add_css_class(button, "mitigation-badge");
    gtk_widget_add_css_class(dot, "dot");
    gtk_widget_add_css_class(detail_title, "accent-text");
    gtk_widget_add_css_class(detail, "secondary");
    gtk_label_set_xalign(GTK_LABEL(detail_title), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(detail), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(detail), TRUE);
    gtk_box_append(GTK_BOX(box), label);
    gtk_box_append(GTK_BOX(box), dot);
    gtk_button_set_child(GTK_BUTTON(button), box);

    gtk_widget_set_margin_top(content, 8);
    gtk_widget_set_margin_bottom(content, 8);
    gtk_widget_set_margin_start(content, 8);
    gtk_widget_set_margin_end(content, 8);
    gtk_box_append(GTK_BOX(content), detail_title);
    gtk_box_append(GTK_BOX(content), detail);
    gtk_popover_set_child(GTK_POPOVER(popover), content);
    gtk_popover_set_has_arrow(GTK_POPOVER(popover), TRUE);
    gtk_popover_set_position(GTK_POPOVER(popover), GTK_POS_BOTTOM);
    gtk_widget_set_parent(popover, button);
    g_signal_connect_swapped(button, "clicked", G_CALLBACK(gtk_popover_popup), popover);

    if (detail_label_out) {
        *detail_label_out = detail;
    }
    return button;
}

static HeapLensTopBarState *heaplens_top_bar_state(GtkWidget *bar) {
    return bar ? g_object_get_data(G_OBJECT(bar), "heaplens-top-bar-state") : NULL;
}

GtkWidget *heaplens_top_bar_new(void) {
    static const char *versions[] = {"2.23", "2.27", "2.29", "2.31", "2.32", "2.34", "2.35", "2.38", "2.39", NULL};
    static const struct {
        HeapLensTopBarAction action;
        const char *icon;
        const char *label;
        const char *tooltip;
        gboolean toggle;
    } action_specs[] = {
        {HEAPLENS_TOP_ACTION_RESET, "go-first-symbolic", NULL, "Reset", FALSE},
        {HEAPLENS_TOP_ACTION_RUN, "media-playback-start-symbolic", NULL, "Run", FALSE},
        {HEAPLENS_TOP_ACTION_PAUSE, "media-playback-pause-symbolic", NULL, "Pause", FALSE},
        {HEAPLENS_TOP_ACTION_STEP, "media-skip-forward-symbolic", NULL, "Step", FALSE},
        {HEAPLENS_TOP_ACTION_STEP_OVER, "media-seek-forward-symbolic", NULL, "Step Over", FALSE},
        {HEAPLENS_TOP_ACTION_STEP_OUT, "go-up-symbolic", NULL, "Step Out", FALSE},
        {HEAPLENS_TOP_ACTION_AUTORUN, "view-refresh-symbolic", NULL, "Auto-run", TRUE},
        {HEAPLENS_TOP_ACTION_SNAPSHOT, "camera-photo-symbolic", NULL, "Snapshot state to JSON", FALSE},
        {HEAPLENS_TOP_ACTION_SEARCH, "system-search-symbolic", NULL, "Search heap memory", FALSE},
        {HEAPLENS_TOP_ACTION_COPY, "edit-copy-symbolic", NULL, "Copy current state", FALSE},
        {HEAPLENS_TOP_ACTION_SETTINGS, "preferences-system-symbolic", NULL, "Settings", FALSE},
        {HEAPLENS_TOP_ACTION_TERMINAL, "utilities-terminal-symbolic", NULL, "Toggle terminal", TRUE},
        {HEAPLENS_TOP_ACTION_WINDOW_HEAP, "media-optical-symbolic", "Heap", "Toggle Heap window", TRUE},
        {HEAPLENS_TOP_ACTION_WINDOW_STACK, "view-list-symbolic", "Stack", "Toggle Stack window", TRUE},
        {HEAPLENS_TOP_ACTION_WINDOW_PROCESS, "application-x-executable-symbolic", "Process", "Toggle Process window", TRUE},
        {HEAPLENS_TOP_ACTION_WINDOW_REGS, "view-grid-symbolic", "Regs", "Toggle Registers window", TRUE}
    };
    static const char *mitigation_names[] = {"ASLR", "PIE", "RELRO", "NX", "Canary", "Safe-Link", "tcache-key", "vtable"};
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *left = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    GtkWidget *center = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *right = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *right_windows = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    GtkWidget *badges = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *menu_button = gtk_menu_button_new();
    GtkWidget *technique_label = gtk_label_new("TECHNIQUE");
    GtkWidget *technique_dropdown = NULL;
    GtkWidget *glibc_label = gtk_label_new("GLIBC");
    GtkWidget *glibc_dropdown = NULL;
    GtkWidget *speed_scale = NULL;
    GtkStringList *technique_model = gtk_string_list_new(NULL);
    GtkStringList *version_model = gtk_string_list_new(versions);
    HeapLensTopBarState *state = g_new0(HeapLensTopBarState, 1);
    size_t index = 0;

    gtk_widget_add_css_class(root, "toolbar-row");
    gtk_widget_add_css_class(left, "panel-flat");
    gtk_widget_add_css_class(center, "panel-flat");
    gtk_widget_add_css_class(right, "panel-flat");
    gtk_widget_add_css_class(right_windows, "panel-flat");
    gtk_widget_add_css_class(badges, "panel-flat");
    gtk_widget_add_css_class(technique_label, "toolbar-label");
    gtk_widget_add_css_class(glibc_label, "toolbar-label");
    gtk_box_set_spacing(GTK_BOX(left), 2);
    gtk_box_set_spacing(GTK_BOX(center), 4);
    gtk_box_set_spacing(GTK_BOX(right), 6);
    gtk_box_set_spacing(GTK_BOX(right_windows), 2);
    gtk_box_set_spacing(GTK_BOX(badges), 4);
    gtk_widget_set_hexpand(spacer, TRUE);

    gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(menu_button), "open-menu-symbolic");
    gtk_widget_add_css_class(menu_button, "icon-button");
    gtk_widget_set_tooltip_text(menu_button, "Preferences");
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(menu_button), heaplens_top_bar_build_preferences_popover("App preferences"));

    technique_dropdown = gtk_drop_down_new(G_LIST_MODEL(technique_model), NULL);
    glibc_dropdown = gtk_drop_down_new(G_LIST_MODEL(version_model), NULL);
    gtk_widget_set_size_request(technique_dropdown, 240, -1);
    gtk_widget_set_size_request(glibc_dropdown, 80, -1);

    gtk_box_append(GTK_BOX(left), menu_button);

    for (index = 0; index < G_N_ELEMENTS(action_specs); ++index) {
        GtkWidget *button = (action_specs[index].label && action_specs[index].label[0])
            ? heaplens_top_bar_labeled_button(action_specs[index].icon,
                                              action_specs[index].label,
                                              action_specs[index].tooltip,
                                              action_specs[index].toggle)
            : heaplens_top_bar_icon_button(action_specs[index].icon,
                                           action_specs[index].tooltip,
                                           action_specs[index].toggle);

        if (action_specs[index].action >= HEAPLENS_TOP_ACTION_WINDOW_HEAP) {
            gtk_widget_add_css_class(button, "window-toggle");
            gtk_box_append(GTK_BOX(right_windows), button);
        } else if (action_specs[index].action == HEAPLENS_TOP_ACTION_SEARCH ||
                   action_specs[index].action == HEAPLENS_TOP_ACTION_COPY ||
                   action_specs[index].action == HEAPLENS_TOP_ACTION_SETTINGS ||
                   action_specs[index].action == HEAPLENS_TOP_ACTION_TERMINAL ||
                   action_specs[index].action == HEAPLENS_TOP_ACTION_SNAPSHOT) {
            gtk_box_append(GTK_BOX(right), button);
        } else {
            gtk_box_append(GTK_BOX(center), button);
        }
        state->actions[action_specs[index].action] = button;
    }

    speed_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1.0, 2000.0, 1.0);
    gtk_widget_set_size_request(speed_scale, 100, -1);
    gtk_scale_set_draw_value(GTK_SCALE(speed_scale), FALSE);
    gtk_range_set_value(GTK_RANGE(speed_scale), 500.0);
    gtk_box_append(GTK_BOX(center), speed_scale);

    gtk_box_append(GTK_BOX(right), technique_label);
    gtk_box_append(GTK_BOX(right), technique_dropdown);
    gtk_box_append(GTK_BOX(right), glibc_label);
    gtk_box_append(GTK_BOX(right), glibc_dropdown);

    for (index = 0; index < G_N_ELEMENTS(mitigation_names); ++index) {
        state->badges[index] = heaplens_top_bar_build_badge(mitigation_names[index], &state->badge_details[index]);
        gtk_box_append(GTK_BOX(badges), state->badges[index]);
    }

    gtk_box_append(GTK_BOX(root), left);
    gtk_box_append(GTK_BOX(root), center);
    gtk_box_append(GTK_BOX(root), spacer);
    gtk_box_append(GTK_BOX(root), right);
    gtk_box_append(GTK_BOX(root), badges);
    gtk_box_append(GTK_BOX(root), right_windows);

    state->root = root;
    state->technique_dropdown = technique_dropdown;
    state->glibc_dropdown = glibc_dropdown;
    state->autorun_scale = speed_scale;
    g_object_set_data_full(G_OBJECT(root), "heaplens-top-bar-state", state, g_free);

    gtk_drop_down_set_selected(GTK_DROP_DOWN(glibc_dropdown), 6);
    g_object_unref(technique_model);
    g_object_unref(version_model);
    return root;
}

GtkDropDown *heaplens_top_bar_get_technique_dropdown(GtkWidget *bar) {
    HeapLensTopBarState *state = heaplens_top_bar_state(bar);
    return state ? GTK_DROP_DOWN(state->technique_dropdown) : NULL;
}

GtkDropDown *heaplens_top_bar_get_glibc_dropdown(GtkWidget *bar) {
    HeapLensTopBarState *state = heaplens_top_bar_state(bar);
    return state ? GTK_DROP_DOWN(state->glibc_dropdown) : NULL;
}

GtkScale *heaplens_top_bar_get_autorun_scale(GtkWidget *bar) {
    HeapLensTopBarState *state = heaplens_top_bar_state(bar);
    return state ? GTK_SCALE(state->autorun_scale) : NULL;
}

GtkWidget *heaplens_top_bar_get_action_button(GtkWidget *bar, HeapLensTopBarAction action) {
    HeapLensTopBarState *state = heaplens_top_bar_state(bar);
    return (state && action < HEAPLENS_TOP_ACTION_COUNT) ? state->actions[action] : NULL;
}

void heaplens_top_bar_set_action_active(GtkWidget *bar, HeapLensTopBarAction action, gboolean active) {
    GtkWidget *button = heaplens_top_bar_get_action_button(bar, action);

    if (!button) {
        return;
    }

    if (GTK_IS_TOGGLE_BUTTON(button)) {
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != active) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), active);
        }
    }

    if (active) {
        gtk_widget_add_css_class(button, "accent");
    } else {
        gtk_widget_remove_css_class(button, "accent");
    }
}

void heaplens_top_bar_set_mitigation(GtkWidget *bar,
                                     HeapLensMitigation mitigation,
                                     gboolean active,
                                     const char *summary,
                                     gboolean requires_bypass) {
    HeapLensTopBarState *state = heaplens_top_bar_state(bar);
    GtkWidget *button = NULL;
    GtkWidget *detail = NULL;
    GtkWidget *child = NULL;
    GtkWidget *dot = NULL;
    char detail_text[512];

    if (!state || mitigation >= HEAPLENS_MITIGATION_COUNT) {
        return;
    }

    button = state->badges[mitigation];
    detail = state->badge_details[mitigation];
    child = gtk_button_get_child(GTK_BUTTON(button));
    if (child) {
        dot = gtk_widget_get_last_child(child);
    }

    gtk_widget_remove_css_class(button, "active");
    gtk_widget_remove_css_class(button, "inactive");
    gtk_widget_add_css_class(button, active ? "active" : "inactive");
    if (dot) {
        gtk_widget_remove_css_class(dot, "green");
        gtk_widget_remove_css_class(dot, "red");
        gtk_widget_add_css_class(dot, active ? "green" : "red");
    }

    g_snprintf(detail_text,
               sizeof(detail_text),
               "%s\n\nCurrent state: %s\nTechnique impact: %s",
               summary ? summary : "Mitigation details unavailable.",
               active ? "active" : "bypassed or absent",
               requires_bypass ? "This technique needs a bypass or leak." : "No direct bypass is required.");
    if (detail) {
        gtk_label_set_text(GTK_LABEL(detail), detail_text);
    }
}
