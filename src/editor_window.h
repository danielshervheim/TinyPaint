//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#ifndef EDITOR_WINDOW_H_
#define EDITOR_WINDOW_H_

#include <gdk/gdk.h>  // GdkRGBA
#include <gtk/gtk.h>

/* The rate (in milliseconds) at which the gui checks if the mouse has moved
from its previous position. */
#define MOUSE_HOLD_POLL_RATE_MS 150

G_BEGIN_DECLS

#define EDITOR_WINDOW_TYPE_WINDOW (editor_window_get_type ())
G_DECLARE_FINAL_TYPE(EditorWindow, editor_window, EDITOR_WINDOW, WINDOW, GtkWindow)

/* Returns a new EditorWindow instance. */
EditorWindow* editor_window_new(void);

/* Initializes an EditorWindow instance from a set of parameters. */
void editor_window_canvas_init_from_parameters(EditorWindow *self, int width, int height, GdkRGBA backgroundColor

/* Initializes an EditorWindow instance from a filepath. */
void editor_window_canvas_init_from_file(EditorWindow *self, const char *filepath);

G_END_DECLS

#endif  // EDITOR_WINDOW_H_
