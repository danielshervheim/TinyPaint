//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "tinypaint_app.h"

#include "new_image_dialog.h"
#include "editor_window.h"

#include "tinypaint_gresource.h"

#include <gdk/gdk.h>  // GdkRGBA

struct _TinyPaintApp {
    GtkApplication parent_instance;
};

G_DEFINE_TYPE(TinyPaintApp, tinypaint_app, GTK_TYPE_APPLICATION);



//
// Function headers
// (necessary because each one needs to know about the other to install signal handlers)
//
void add_editor_window(GtkApplication *self);
void add_editor_window_from_file(GtkApplication *self, const char *filepath);



//
// TINYPAINT_APP methods
//

/* Creates a new editorWindow based on parameters from a newImageDialog and
adds it to the application. */
void add_editor_window(GtkApplication *self) {
    NewImageDialog *newImageDialog = new_image_dialog_new();

    if (gtk_dialog_run(GTK_DIALOG(newImageDialog)) == GTK_RESPONSE_OK) {
        int width = new_image_dialog_get_width(newImageDialog);
        int height = new_image_dialog_get_height(newImageDialog);
        GdkRGBA color = new_image_dialog_get_color(newImageDialog);

        EditorWindow *editorWindow = editor_window_new();
        editor_window_canvas_init_from_parameters(editorWindow, width, height, color);
        
        g_signal_connect_swapped(editorWindow, "editor-open", (GCallback)add_editor_window_from_file, self);
        g_signal_connect_swapped(editorWindow, "editor-new", (GCallback)add_editor_window, self);

        gtk_application_add_window(self, GTK_WINDOW(editorWindow));
        gtk_window_present(GTK_WINDOW(editorWindow));
    }

    gtk_widget_destroy(GTK_WIDGET(newImageDialog));
}

/* Creates a new editorWindow from the file, and adds it to the application. */
void add_editor_window_from_file(GtkApplication *self, const char *filepath) {
    EditorWindow *editorWindow = editor_window_new();
    editor_window_canvas_init_from_file(editorWindow, filepath);

    g_signal_connect_swapped(editorWindow, "editor-open", (GCallback)add_editor_window_from_file, self);
    g_signal_connect_swapped(editorWindow, "editor-new", (GCallback)add_editor_window, self);

    gtk_application_add_window(self, GTK_WINDOW(editorWindow));
    gtk_window_present(GTK_WINDOW(editorWindow));
}



//
// GTK_APPLICATION overidden methods
//

/* initializes the instance */
static void tinypaint_app_init (TinyPaintApp *self) { }

/* Fires when the user opens TinyPaint without arguments (i.e. from the launcher) */
static void tinypaint_app_activate(GApplication *app) {
    add_editor_window(GTK_APPLICATION(app));
}

/* Fires when the user opens TinyPaint with arguments (i.e. right click->open in) */
static void tinypaint_app_open (GApplication *app, GFile **files, gint n_files, const gchar *hint) {
    // spawns a new editor window and adds it to the app for each file passed in.
    for (int i = 0; i < n_files; i++) {
        // assumes the filepath is valid (i.e. that input checking is done via gui).
        char *filepath = g_file_get_path(files[i]);
        add_editor_window_from_file(GTK_APPLICATION(app), filepath);
    }
}

static void tinypaint_app_class_init(TinyPaintAppClass *class) {
    // virtual function overrides go here
    G_APPLICATION_CLASS(class)->activate = tinypaint_app_activate;
    G_APPLICATION_CLASS(class)->open = tinypaint_app_open;
}

TinyPaintApp* tinypaint_app_new(void) {
    return g_object_new(TINYPAINT_APP_TYPE_APPLICATION,
                        "application-id", "danshervheim.tinypaint",
                        "flags", G_APPLICATION_HANDLES_OPEN, NULL);
}
