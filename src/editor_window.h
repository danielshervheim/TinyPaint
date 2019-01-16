//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#ifndef EDITOR_WINDOW_H_
#define EDITOR_WINDOW_H_

#include <gtk/gtk.h>

#include <gdk/gdk.h>  // GdkRGBA

#define MAX_HISTORY_STATES 10
#define MOUSE_HOLD_POLL_RATE_MS 150

G_BEGIN_DECLS

#define EDITOR_WINDOW_TYPE_WINDOW (editor_window_get_type ())
G_DECLARE_FINAL_TYPE(EditorWindow, editor_window, EDITOR_WINDOW, WINDOW, GtkWindow)

EditorWindow* editor_window_new(void);

void editor_window_canvas_init(EditorWindow *self, int width, int height, GdkRGBA backgroundColor);
void editor_window_canvas_init_with_image_path(EditorWindow *self, const char *path);

G_END_DECLS

#endif  // EDITOR_WINDOW_H_
