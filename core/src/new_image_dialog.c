//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "new_image_dialog.h"

struct _NewImageDialog {
    GtkDialog parent_instance;

    // instance properties
    GdkRGBA m_color;
    int m_height;
    int m_width;
};

G_DEFINE_TYPE(NewImageDialog, new_image_dialog, GTK_TYPE_DIALOG);



//
// COLOR property
//

/* Gets the color from the colorChooser and updates this instance's color property */
void on_imageColor_color_set(GtkColorButton *button, gpointer user_data) {
    NewImageDialog *self = (NewImageDialog *)user_data;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), &(self->m_color));
}

/* Returns this instance's current color property */
GdkRGBA new_image_dialog_get_color(NewImageDialog *self) {
    return self->m_color;
}



//
// HEIGHT property
//

/* Gets the height from the adjustment and updates this instance's height property */
void on_imageHeight_value_changed(GtkAdjustment *adjustment, gpointer user_data) {
    NewImageDialog *self = (NewImageDialog *)user_data;
    self->m_height = gtk_adjustment_get_value(adjustment);
}

/* Returns this instance's current height property */
int new_image_dialog_get_height(NewImageDialog *self) {
    return self->m_height;
}



//
// WIDTH property
//

/* Gets the width from the adjustment and updates this instance's width property */
void on_imageWidth_value_changed(GtkAdjustment *adjustment, gpointer user_data) {
    NewImageDialog *self = (NewImageDialog *)user_data;
    self->m_width = gtk_adjustment_get_value(adjustment);
}

/* Returns this instance's current width property */
int new_image_dialog_get_width(NewImageDialog *self) {
    return self->m_width;
}

//
// G Callbacks
//

/* Initializies the instance of NewImageDialog */
static void new_image_dialog_init (NewImageDialog *self) {
    // request the size of this window (this makes it look nice... and is purely superfluous)
    gtk_widget_set_size_request(GTK_WIDGET(self), 377.5, -1);

    // set the windows title
    gtk_window_set_title(GTK_WINDOW(self), "New Image");

    // add response buttons
    /*GtkWidget *okButton =*/ gtk_dialog_add_button(GTK_DIALOG(self), "Ok", GTK_RESPONSE_OK);
    /*GtkWidget *cancelButton =*/ gtk_dialog_add_button(GTK_DIALOG(self), "Cancel", GTK_RESPONSE_CANCEL);

    // create a gtk builder from the resources
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/new_image_dialog.glade");

    // get this dialog's content area and set its margins
    GtkWidget *contentArea = gtk_dialog_get_content_area(GTK_DIALOG(self));
    gtk_widget_set_margin_start(contentArea, 10);
    gtk_widget_set_margin_end(contentArea, 10);
    gtk_widget_set_margin_top(contentArea, 10);
    gtk_widget_set_margin_bottom(contentArea, 10);

    // get a reference to the settingsGrid widget
    GtkWidget *settingsGrid = GTK_WIDGET(gtk_builder_get_object(builder, "settingsGrid"));

    // add the settingsGrid to the contentArea and show it
    gtk_container_add(GTK_CONTAINER(contentArea), settingsGrid);
    gtk_widget_show_all(contentArea);

    // get references to the widgets from the settingsGrid
    GtkColorButton *imageColor = GTK_COLOR_BUTTON(gtk_builder_get_object(builder, "imageColor"));
    GtkAdjustment *imageHeight = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "imageHeight"));
    GtkAdjustment *imageWidth = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "imageWidth"));

    // and connect the appropriate signal handlers
    g_signal_connect(imageColor, "color-set", (GCallback)on_imageColor_color_set, self);
    g_signal_connect(imageHeight, "value-changed", (GCallback)on_imageHeight_value_changed, self);
    g_signal_connect(imageWidth, "value-changed", (GCallback)on_imageWidth_value_changed, self);

    // then call these handlers to initialize this instance's properties
    on_imageColor_color_set(imageColor, self);
    on_imageHeight_value_changed(imageHeight, self);
    on_imageWidth_value_changed(imageWidth, self);

    // and finally, unref the builder as its no longer needed
    g_object_unref(builder);
}

/* Returns a new instance of NewImageDialog */
NewImageDialog *new_image_dialog_new () {
    return g_object_new (NEW_IMAGE_DIALOG_TYPE_DIALOG, NULL);
}

/* Initializes the NewImageDialog class */
static void new_image_dialog_class_init (NewImageDialogClass *class) {
    // not necessary???
    // GObject property stuff would go here...
}
