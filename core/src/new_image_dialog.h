//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#ifndef NEW_IMAGE_DIALOG_H_
#define NEW_IMAGE_DIALOG_H_

#include <gdk/gdk.h>  // GdkRGBA
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define NEW_IMAGE_DIALOG_TYPE_DIALOG (new_image_dialog_get_type ())
G_DECLARE_FINAL_TYPE(NewImageDialog, new_image_dialog, NEW_IMAGE_DIALOG, DIALOG, GtkDialog)

NewImageDialog* new_image_dialog_new(void);

int new_image_dialog_get_width(NewImageDialog *self);
int new_image_dialog_get_height(NewImageDialog *self);
GdkRGBA new_image_dialog_get_color(NewImageDialog *self);

G_END_DECLS

#endif  // NEW_IMAGE_DIALOG_H_
