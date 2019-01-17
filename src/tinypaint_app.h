//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#ifndef TINYPAINT_APP_H_
#define TINYPAINT_APP_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TINYPAINT_APP_TYPE_APPLICATION (tinypaint_app_get_type ())
G_DECLARE_FINAL_TYPE(TinyPaintApp, tinypaint_app, TINYPAINT_APP, APPLICATION, GtkApplication)

/* Returns a pointer to a new instance of TinyPaintApp. */
TinyPaintApp* tinypaint_app_new(void);

/* Spawns a newImage dialog, then creates and adds editorWindow to the app, based
on the parameters from the newImage dialog. */
void tinypaint_app_new_editor_window(GtkApplication *self);

/* Creates and adds a new editorWindow to the app, based on the filepath. */
void tinypaint_app_new_editor_window_from_file(GtkApplication *self, const char *filepath);

G_END_DECLS

#endif  // TINYPAINT_APP_H_
