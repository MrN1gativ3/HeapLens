#include "dbg_common.h"

#include <glib/gstdio.h>

typedef struct {
    char *config_key;
} HeapLensDebuggerWindowState;

static char *heaplens_dbg_config_path(void) {
    return g_build_filename(g_get_user_config_dir(), "heaplens", "debugger-windows.ini", NULL);
}

static GKeyFile *heaplens_dbg_key_file_load(void) {
    GKeyFile *key_file = g_key_file_new();
    char *path = heaplens_dbg_config_path();
    GError *error = NULL;

    if (!g_key_file_load_from_file(key_file, path, G_KEY_FILE_KEEP_COMMENTS, &error)) {
        g_clear_error(&error);
    }

    g_free(path);
    return key_file;
}

static void heaplens_dbg_key_file_save(GKeyFile *key_file) {
    char *path = heaplens_dbg_config_path();
    char *dir = g_path_get_dirname(path);
    char *data = NULL;
    gsize length = 0;

    g_mkdir_with_parents(dir, 0755);
    data = g_key_file_to_data(key_file, &length, NULL);
    g_file_set_contents(path, data, (gssize) length, NULL);

    g_free(data);
    g_free(dir);
    g_free(path);
}

static gboolean heaplens_dbg_window_close_request(GtkWindow *window, gpointer user_data) {
    HeapLensDebuggerWindowState *state = user_data;

    if (state && state->config_key) {
        heaplens_dbg_window_save(window, state->config_key);
    }
    return FALSE;
}

static void heaplens_dbg_window_state_free(gpointer data) {
    HeapLensDebuggerWindowState *state = data;

    if (!state) {
        return;
    }

    g_free(state->config_key);
    g_free(state);
}

void heaplens_dbg_apply_pinned_state(GtkWindow *window, GtkToggleButton *button) {
    GtkNative *native = NULL;
    GdkSurface *surface = NULL;

    if (!window || !button) {
        return;
    }

    native = gtk_widget_get_native(GTK_WIDGET(window));
    if (!native) {
        return;
    }

    surface = gtk_native_get_surface(native);
    if (!surface || !GDK_IS_TOPLEVEL(surface)) {
        return;
    }

    /* GTK4 does not expose a cross-backend setter for ALWAYS-ON-TOP.
     * We preserve the toggle state and aggressively present the window when pinned.
     */
    if (gtk_toggle_button_get_active(button)) {
        gtk_window_present(window);
    }
}

HeapLensDebuggerGeometry heaplens_dbg_window_load(const char *config_key,
                                                  int default_width,
                                                  int default_height) {
    HeapLensDebuggerGeometry geometry = {
        .width = default_width,
        .height = default_height,
        .pinned = FALSE
    };
    GKeyFile *key_file = NULL;

    if (!config_key) {
        return geometry;
    }

    key_file = heaplens_dbg_key_file_load();
    if (g_key_file_has_group(key_file, config_key)) {
        geometry.width = g_key_file_get_integer(key_file, config_key, "width", NULL);
        geometry.height = g_key_file_get_integer(key_file, config_key, "height", NULL);
        geometry.pinned = g_key_file_get_boolean(key_file, config_key, "pinned", NULL);
    }
    if (geometry.width <= 0) {
        geometry.width = default_width;
    }
    if (geometry.height <= 0) {
        geometry.height = default_height;
    }

    g_key_file_unref(key_file);
    return geometry;
}

void heaplens_dbg_window_save(GtkWindow *window, const char *config_key) {
    GKeyFile *key_file = NULL;
    GtkWidget *pin = NULL;
    int width = 0;
    int height = 0;

    if (!window || !config_key) {
        return;
    }

    width = gtk_widget_get_width(GTK_WIDGET(window));
    height = gtk_widget_get_height(GTK_WIDGET(window));
    pin = g_object_get_data(G_OBJECT(window), "heaplens-pin-button");

    key_file = heaplens_dbg_key_file_load();
    g_key_file_set_integer(key_file, config_key, "width", MAX(width, 1));
    g_key_file_set_integer(key_file, config_key, "height", MAX(height, 1));
    g_key_file_set_boolean(key_file,
                           config_key,
                           "pinned",
                           pin && GTK_IS_TOGGLE_BUTTON(pin) ? gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pin)) : FALSE);
    heaplens_dbg_key_file_save(key_file);
    g_key_file_unref(key_file);
}

void heaplens_dbg_window_init(GtkWindow *window,
                              GtkWindow *parent,
                              const char *title,
                              const char *config_key,
                              int default_width,
                              int default_height) {
    HeapLensDebuggerGeometry geometry;
    GtkWidget *header = NULL;
    GtkWidget *title_label = NULL;
    GtkWidget *pin_button = NULL;
    HeapLensDebuggerWindowState *state = NULL;

    if (!window) {
        return;
    }

    geometry = heaplens_dbg_window_load(config_key, default_width, default_height);

    gtk_window_set_title(window, title);
    gtk_window_set_default_size(window, geometry.width, geometry.height);
    gtk_window_set_transient_for(window, parent);
    gtk_window_set_modal(window, FALSE);
    gtk_window_set_hide_on_close(window, TRUE);

    header = gtk_header_bar_new();
    gtk_widget_add_css_class(header, "header-bar");
    gtk_widget_add_css_class(header, "window-titlebar");
    title_label = gtk_label_new(title);
    gtk_widget_add_css_class(title_label, "secondary");
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), title_label);

    pin_button = gtk_toggle_button_new();
    gtk_widget_add_css_class(pin_button, "icon-button");
    gtk_widget_set_tooltip_text(pin_button, "Pin window");
    gtk_button_set_icon_name(GTK_BUTTON(pin_button), "view-pin-symbolic");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pin_button), geometry.pinned);
    g_signal_connect_swapped(pin_button, "toggled", G_CALLBACK(heaplens_dbg_apply_pinned_state), window);

    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), pin_button);
    gtk_window_set_titlebar(window, header);
    g_object_set_data(G_OBJECT(window), "heaplens-pin-button", pin_button);

    state = g_new0(HeapLensDebuggerWindowState, 1);
    state->config_key = g_strdup(config_key);
    g_object_set_data_full(G_OBJECT(window), "heaplens-debugger-window-state", state, heaplens_dbg_window_state_free);
    g_signal_connect(window, "close-request", G_CALLBACK(heaplens_dbg_window_close_request), state);
}

char *heaplens_dbg_format_hex_dump(const guint8 *bytes, size_t size, uint64_t base_address) {
    GString *string = NULL;
    size_t offset = 0;

    if (!bytes || size == 0) {
        return g_strdup("No readable bytes at this address.");
    }

    string = g_string_new("");
    while (offset < size) {
        size_t line_index = 0;

        g_string_append_printf(string, "%08llx: ", (unsigned long long) (base_address + offset));
        for (line_index = 0; line_index < 16; ++line_index) {
            if (offset + line_index < size) {
                g_string_append_printf(string, "%02x ", bytes[offset + line_index]);
            } else {
                g_string_append(string, "   ");
            }
        }
        g_string_append(string, " ");
        for (line_index = 0; line_index < 16 && offset + line_index < size; ++line_index) {
            guint8 ch = bytes[offset + line_index];
            g_string_append_c(string, g_ascii_isprint(ch) ? (char) ch : '.');
        }
        g_string_append_c(string, '\n');
        offset += 16;
    }

    return g_string_free(string, FALSE);
}
