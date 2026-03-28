#include "theme.h"

#define RGBA_HEX(red, green, blue) {(red) / 255.0, (green) / 255.0, (blue) / 255.0, 1.0}

static const GdkRGBA heaplens_terminal_foreground = RGBA_HEX(255, 255, 255);
static const GdkRGBA heaplens_terminal_background = RGBA_HEX(30, 30, 30);
static const GdkRGBA heaplens_terminal_cursor = RGBA_HEX(53, 132, 228);
static const GdkRGBA heaplens_terminal_palette[16] = {
    RGBA_HEX(30, 30, 30),   RGBA_HEX(255, 123, 99),  RGBA_HEX(87, 227, 137), RGBA_HEX(249, 240, 107),
    RGBA_HEX(98, 160, 234), RGBA_HEX(192, 97, 203),  RGBA_HEX(98, 160, 234), RGBA_HEX(170, 170, 170),
    RGBA_HEX(61, 61, 61),   RGBA_HEX(255, 123, 99),  RGBA_HEX(87, 227, 137), RGBA_HEX(249, 240, 107),
    RGBA_HEX(53, 132, 228), RGBA_HEX(192, 97, 203),  RGBA_HEX(98, 160, 234), RGBA_HEX(255, 255, 255)
};

guint heaplens_theme_count(void) {
    return 1;
}

guint heaplens_theme_default_index(void) {
    return 0;
}

const char *heaplens_theme_get_label(guint index) {
    (void) index;
    return "Mission Center";
}

const char *heaplens_theme_css_path(void) {
    return "assets/styles/app.css";
}

void heaplens_theme_apply_css(GtkCssProvider *provider, guint index) {
    GtkSettings *settings = gtk_settings_get_default();
    (void) index;

    if (!provider) {
        return;
    }

    if (settings) {
        g_object_set(settings, "gtk-application-prefer-dark-theme", TRUE, NULL);
    }
    gtk_css_provider_load_from_path(provider, heaplens_theme_css_path());
}

void heaplens_theme_apply_terminal(VteTerminal *terminal, guint index) {
    (void) index;

    if (!terminal) {
        return;
    }

    vte_terminal_set_colors(terminal,
                            &heaplens_terminal_foreground,
                            &heaplens_terminal_background,
                            heaplens_terminal_palette,
                            G_N_ELEMENTS(heaplens_terminal_palette));
    vte_terminal_set_color_cursor(terminal, &heaplens_terminal_cursor);
    vte_terminal_set_color_cursor_foreground(terminal, &heaplens_terminal_background);
}
