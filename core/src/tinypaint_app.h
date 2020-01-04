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

G_END_DECLS

#endif  // TINYPAINT_APP_H_
