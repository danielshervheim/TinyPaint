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
    // instance variables of subclass go here

    int m_numWindows;
};

G_DEFINE_TYPE(TinyPaintApp, tinypaint_app, GTK_TYPE_APPLICATION);

// todo: add startup and shutdown signal handlers


void on_editorWindow_destroy(TinyPaintApp *self) {
    // todo: bug: when fork-execing -- this is still copied over in memory...
    printf("pre: %d, post %d\n", self->m_numWindows, self->m_numWindows-1);
    self->m_numWindows--;
    if (self->m_numWindows == 0) {
        gtk_main_quit();
    }
}



//
//
//

static void
tinypaint_app_init (TinyPaintApp *self) {
    // initialisation goes here
    self->m_numWindows = 0;
}

static void
tinypaint_app_activate(GApplication *app) {
    TinyPaintApp *self = (TinyPaintApp *)app;

    NewImageDialog *nidia = new_image_dialog_new();

    // run a new image dialog
    if (gtk_dialog_run(GTK_DIALOG(nidia)) == GTK_RESPONSE_OK) {
        // get parameters
        int width = new_image_dialog_get_width(nidia);
        int height = new_image_dialog_get_height(nidia);
        GdkRGBA color = new_image_dialog_get_color(nidia);

        // destroy the dialog
        gtk_widget_destroy(GTK_WIDGET(nidia));

        // make a new editorWindow instance
        EditorWindow *edwin = editor_window_new();

        // pass in the initialization parameters
        editor_window_canvas_init(edwin, width, height, color);

        // override the quit signal to quit the gtk main loop when the editorWindow closes
        g_signal_connect_swapped(GTK_WIDGET(edwin), "destroy", (GCallback)on_editorWindow_destroy, self);

        self->m_numWindows++;

        // show the editorWindow
        gtk_window_present(GTK_WINDOW(edwin));

        // and finally, start the main loop
        gtk_main();
    }
    else {
        gtk_widget_destroy(GTK_WIDGET(nidia));
    }
}

static void
tinypaint_app_open (GApplication *app, GFile **files,
gint n_files, const gchar *hint) {

    TinyPaintApp *self = (TinyPaintApp *)app;

    for (int i = 0; i < n_files; i++) {
        char *filepath = g_file_get_path(files[i]);

        EditorWindow *edwin = editor_window_new();
        editor_window_canvas_init_with_image_path(edwin, filepath);

        g_signal_connect_swapped(edwin, "destroy", (GCallback)on_editorWindow_destroy, self);
        self->m_numWindows++;

        gtk_window_present(GTK_WINDOW(edwin));
    }

    gtk_main();
}

static void tinypaint_app_class_init(TinyPaintAppClass *class) {
    // virtual function overrides go here
    G_APPLICATION_CLASS(class)->activate = tinypaint_app_activate;
    G_APPLICATION_CLASS(class)->open = tinypaint_app_open;

    // property and signal definitions go here
}

TinyPaintApp* tinypaint_app_new(void) {
    return g_object_new(TINYPAINT_APP_TYPE_APPLICATION,
                        "application-id", "danshervheim.tinypaint",
                        "flags", G_APPLICATION_HANDLES_OPEN, NULL);
}
