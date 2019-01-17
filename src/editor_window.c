//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "editor_window.h"

#include "image_editor.h"
#include "pixel_buffer.h"
#include "tinypaint_app.h"
#include "tools_window.h"
#include "utilities.h"

#include <GL/gl.h>  // openGL headers



struct _EditorWindow {
    GtkWindow parent_instance;

    // the editor
    ImageEditor m_editor;

    // instance properties
    ToolsWindow *m_tools_window;

    // the canvas glarea
    GtkGLArea *m_canvasGLArea;

    // the temporary buffer used to render the
    // pixelbuffer to the screen via opengl
    unsigned int *m_renderBuffer;

    // mouse tracking variables
    int m_mousePosX;
    int m_mousePosY;
    int m_mousePosX_prev;
    int m_mousePosY_prev;
    int m_mouseDown;
};

G_DEFINE_TYPE(EditorWindow, editor_window, GTK_TYPE_WINDOW);



//
// CANVAS / GL related methods
//

/* Rerenders the gtkGLArea associated with this EditorWindow instance, based on its current pixelbuffer */
void canvas_render(EditorWindow *self, GdkGLContext *context) {
    // make the context of the canvas current
    gdk_gl_context_make_current (context);

    // clear the buffer
    glClearColor (0, 0, 0, 1);
    glClear (GL_COLOR_BUFFER_BIT);

    // get the current pixelbuffer from the editor
    PixelBuffer *render = image_editor_get_current_pixelbuffer(&(self->m_editor));

    // update the renderBuffer based on the current PixelBuffer
    for (int y = 0; y < render->height; y++) {
        for (int x = 0; x < render->width; x++) {
            // OpenGL renders upside down so we must flip the y axis
            int y_modified = (render->height - 1) - y;

            int offset = (render->width * 4 * y_modified) + (4 * x);
            int size = (sizeof(unsigned int));

            GdkRGBA color = pixelbuffer_get_pixel(render, x, y);

            memset(self->m_renderBuffer + offset + 0, color.red * 255, size);
            memset(self->m_renderBuffer + offset + 1, color.green * 255, size);
            memset(self->m_renderBuffer + offset + 2, color.blue * 255, size);
            memset(self->m_renderBuffer + offset + 3, color.alpha * 255, size);
        }
    }

    // finally, draw the new array of pixels to the screen
    glDrawPixels(render->width, render->height, GL_RGBA, GL_UNSIGNED_INT, self->m_renderBuffer);
}

/* Refreshes this instance's GL canvas (basically triggers the "render" signal) */
void canvas_refresh(EditorWindow *self) {
    gtk_gl_area_queue_render(self->m_canvasGLArea);
}



//
// FILE I/O methods
//

/* Opens a new EditorWindow under the current TinyPaintApp instance. */
void file_new(EditorWindow *self) {
    GtkApplication *parent = gtk_window_get_application(GTK_WINDOW(self));

    if (parent != NULL) {
        tinypaint_app_new_editor_window(parent);
    }
    else {
        printf("file_new() ERROR: this EditorWindow has no application parent.\n");
    }
}

/* Opens the file as a new EditorWindow under the current TinyPaintApp instance. */
void file_open(EditorWindow *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/io_dialogs.glade");

    // get a reference to the openDialog from it
    GtkFileChooserDialog *openDialog =
        GTK_FILE_CHOOSER_DIALOG(gtk_builder_get_object(builder, "openDialog"));

    if (gtk_dialog_run(GTK_DIALOG(openDialog)) == GTK_RESPONSE_OK) {
        // get file string
        gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(openDialog));

        GtkApplication *parent = gtk_window_get_application(GTK_WINDOW(self));

        if (parent != NULL) {
            tinypaint_app_new_editor_window_from_file(parent, filename);
        }
        else {
            printf("file_open() RROR: this EditorWindow has no application parent.\n");
        }

        // free the memory allocated to the filename
        g_free(filename);
    }

    gtk_widget_destroy(GTK_WIDGET(openDialog));

    g_object_unref(builder);
}

/* Saves the current buffer of the ImageEditor to a file dictated by the SaveDialog. */
void file_save(EditorWindow *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/io_dialogs.glade");

    // get an instance of saveDialog from it
    GtkFileChooserDialog *saveDialog = GTK_FILE_CHOOSER_DIALOG(gtk_builder_get_object(builder, "saveDialog"));

    // and run the save dialog
    if (gtk_dialog_run(GTK_DIALOG(saveDialog)) == GTK_RESPONSE_OK) {
        // get file string
        gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(saveDialog));
        int filenameLength = strlen(filename);
        char filename_final[filenameLength+5];
        memset(filename_final, '\0', filenameLength+5);

        // verify if filename is valid
        if (string_ends_with(filename, ".png") == 0) {
            sprintf(filename_final, "%s", filename);
        }
        else {
            sprintf(filename_final, "%s.png", filename);
        }

        // and save the image
        image_editor_save_current_pixelbuffer(&(self->m_editor), filename_final);
    }

    // destroy saveDialog widget
    gtk_widget_destroy(GTK_WIDGET(saveDialog));

    // finally, unref the builder
    g_object_unref(builder);
}






//
// DEVICE I/O callbacks
//

/* Catches the keystrokes in the EditorWindow */
int device_keyPress(EditorWindow *self, GdkEventKey *event) {
    if (event->keyval == 122 && event->state == 20) {  // (CTRL+Z) undo
        image_editor_undo(&(self->m_editor));
        canvas_refresh(self);
    }
    else if (event->keyval == 90 && event->state == 21) {  // (CTRL+LSHIFT+Z) redo
        image_editor_redo(&(self->m_editor));
        canvas_refresh(self);
    }
    else if (event->keyval == 113 && event->state == 20) {  // (CTRL+Q) quit
        // will trigger the editor_window_quit method automatically.
        gtk_window_close(GTK_WINDOW(self));
    }
    else if (event->keyval == 110 && event->state == 20) {  // (CTRL+N) new
        file_new(self);
    }
    else if (event->keyval == 111 && event->state == 20) {  // (CTRL+O) open
        file_open(self);
    }
    else if (event->keyval == 115 && event->state == 20) {  // (CTRL+S) save
        file_save(self);
    }

    /* return false, i.e. other signals can interupt this one
    (to enable multi-key combinations) */
    return 0;
}

/* Fires when the mouse is held down on the canvas */
int device_mouseHold(EditorWindow *self) {
    image_editor_stroke_hold(&(self->m_editor), self->m_mousePosX, self->m_mousePosY);
    canvas_refresh(self);
    return self->m_mouseDown;  // will uninstall itself when the mouse is lifted
}

/* Fires when the mouse is moved on the canvas */
void device_mouseMove(EditorWindow *self, GdkEventMotion *event) {
    if (self->m_mouseDown) {
        // update the mouse tracking parameters
        self->m_mousePosX_prev = self->m_mousePosX;
        self->m_mousePosY_prev = self->m_mousePosY;
        self->m_mousePosX = event->x;
        self->m_mousePosY = event->y;

        image_editor_stroke_move(&(self->m_editor), self->m_mousePosX, self->m_mousePosY, self->m_mousePosX_prev, self->m_mousePosY_prev);
        canvas_refresh(self);
    }
}

/* Fires when the mouse is clicked on the canvas */
void device_mouseClick(EditorWindow *self, GdkEventMotion *event) {
    // update the mouse tracking parameters
    self->m_mouseDown = 1;
    self->m_mousePosX = event->x;
    self->m_mousePosY = event->y;
    self->m_mousePosX_prev = self->m_mousePosX;
    self->m_mousePosY_prev = self->m_mousePosY;

    // install the mouseHold polling function
    if (self->m_editor.m_tool.applyWhenStationary) {
        g_timeout_add(MOUSE_HOLD_POLL_RATE_MS, (void *)device_mouseHold, self);
    }

    image_editor_stroke_start(&(self->m_editor), self->m_mousePosX, self->m_mousePosY);
    canvas_refresh(self);
}

/* Fires when the mouse is unclicked on the canvas */
void device_mouseUnclick(EditorWindow *self, GdkEventMotion *event) {
    // update the mouse tracking parameters
    self->m_mouseDown = 0;
    self->m_mousePosX = event->x;
    self->m_mousePosY = event->y;

    image_editor_stroke_end(&(self->m_editor), self->m_mousePosX, self->m_mousePosY);
    canvas_refresh(self);
}



//
// FILTER methods
//

void filter_applySaturation(EditorWindow *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/filter_dialogs.glade");

    // save refernces to the necessary widgets
    GtkDialog *saturationDialog = GTK_DIALOG(gtk_builder_get_object(builder, "saturationDialog"));
    GtkAdjustment *saturationScale = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "saturationScale"));

    // run the dialog
    if (gtk_dialog_run(saturationDialog) == GTK_RESPONSE_APPLY) {
        image_editor_apply_saturation_filter(&(self->m_editor), gtk_adjustment_get_value(saturationScale));
        canvas_refresh(self);
    }

    // destroy the dialog widget
    gtk_widget_destroy(GTK_WIDGET(saturationDialog));

    // unref the builder
    g_object_unref(builder);
}

void filter_applyChannels(EditorWindow *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/filter_dialogs.glade");

    // save refernces to the necessary widgets
    GtkDialog *channelsDialog = GTK_DIALOG(gtk_builder_get_object(builder, "channelsDialog"));
    GtkAdjustment *rScale = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "rScale"));
    GtkAdjustment *gScale = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "gScale"));
    GtkAdjustment *bScale = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "bScale"));

    // run the dialog
    if (gtk_dialog_run(channelsDialog) == GTK_RESPONSE_APPLY) {
        image_editor_apply_channels_filter(&(self->m_editor), gtk_adjustment_get_value(rScale),
            gtk_adjustment_get_value(gScale), gtk_adjustment_get_value(bScale));
            canvas_refresh(self);
    }

    // destroy the dialog widget
    gtk_widget_destroy(GTK_WIDGET(channelsDialog));

    // unref the builder
    g_object_unref(builder);
}

void filter_applyBrightnessContrast(EditorWindow *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/filter_dialogs.glade");

    // save refernces to the necessary widgets
    GtkDialog *brightnessContrastDialog = GTK_DIALOG(gtk_builder_get_object(builder, "brightnessContrastDialog"));
    GtkAdjustment *brightnessScale = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "brightnessScale"));
    GtkAdjustment *contrastScale = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "contrastScale"));

    // run the dialog
    if (gtk_dialog_run(brightnessContrastDialog) == GTK_RESPONSE_APPLY) {
        image_editor_apply_brightness_contrast_filter(&(self->m_editor),
            gtk_adjustment_get_value(brightnessScale), gtk_adjustment_get_value(contrastScale));
            canvas_refresh(self);
    }

    // destroy the dialog widget
    gtk_widget_destroy(GTK_WIDGET(brightnessContrastDialog));

    // unref the builder
    g_object_unref(builder);
}

void filter_applyGaussianBlur(EditorWindow *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/filter_dialogs.glade");

    // save refernces to the necessary widgets
    GtkDialog *gaussianBlurDialog = GTK_DIALOG(gtk_builder_get_object(builder, "gaussianBlurDialog"));
    GtkAdjustment *gaussianBlurRadius = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "gaussianBlurRadius"));

    // run the dialog
    if (gtk_dialog_run(gaussianBlurDialog) == GTK_RESPONSE_APPLY) {
        image_editor_apply_gaussian_blur_filter(&(self->m_editor), (int)gtk_adjustment_get_value(gaussianBlurRadius));
        canvas_refresh(self);
    }

    // destroy the dialog widget
    gtk_widget_destroy(GTK_WIDGET(gaussianBlurDialog));

    // unref the builder
    g_object_unref(builder);
}

void filter_applyMotionBlur(EditorWindow *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/filter_dialogs.glade");

    // save refernces to the necessary widgets
    GtkDialog *motionBlurDialog = GTK_DIALOG(gtk_builder_get_object(builder, "motionBlurDialog"));
    GtkAdjustment *motionBlurRadius = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "motionBlurRadius"));
    GtkAdjustment *motionBlurAngle = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "motionBlurAngle"));

    // run the dialog
    if (gtk_dialog_run(motionBlurDialog) == GTK_RESPONSE_APPLY) {
        image_editor_apply_motion_blur_filter(&(self->m_editor),
            (int)gtk_adjustment_get_value(motionBlurRadius),
            (int)gtk_adjustment_get_value(motionBlurAngle));
            canvas_refresh(self);
    }

    // destroy the dialog widget
    gtk_widget_destroy(GTK_WIDGET(motionBlurDialog));

    // unref the builder
    g_object_unref(builder);
}

void filter_applySharpen(EditorWindow *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/filter_dialogs.glade");

    // save refernces to the necessary widgets
    GtkDialog *sharpenDialog = GTK_DIALOG(gtk_builder_get_object(builder, "sharpenDialog"));
    GtkAdjustment *sharpenRadius = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "sharpenRadius"));

    // run the dialog
    if (gtk_dialog_run(sharpenDialog) == GTK_RESPONSE_APPLY) {
        image_editor_apply_sharpen_filter(&(self->m_editor), (int)gtk_adjustment_get_value(sharpenRadius));
        canvas_refresh(self);
    }

    // destroy the dialog widget
    gtk_widget_destroy(GTK_WIDGET(sharpenDialog));

    // unref the builder
    g_object_unref(builder);
}

void filter_applyPosterize(EditorWindow *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/filter_dialogs.glade");

    // save refernces to the necessary widgets
    GtkDialog *posterizeDialog= GTK_DIALOG(gtk_builder_get_object(builder, "posterizeDialog"));
    GtkAdjustment *posterizeBins = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "posterizeBins"));

    // run the dialog
    if (gtk_dialog_run(posterizeDialog) == GTK_RESPONSE_APPLY) {
        image_editor_apply_posterize_filter(&(self->m_editor), (int)gtk_adjustment_get_value(posterizeBins));
        canvas_refresh(self);
    }

    // destroy the dialog widget
    gtk_widget_destroy(GTK_WIDGET(posterizeDialog));

    // unref the builder
    g_object_unref(builder);
}

void filter_applyThreshold(EditorWindow *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/filter_dialogs.glade");

    // save refernces to the necessary widgets
    GtkDialog *thresholdDialog = GTK_DIALOG(gtk_builder_get_object(builder, "thresholdDialog"));
    GtkAdjustment *thresholdCutoff = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "thresholdCutoff"));

    // run the dialog
    if (gtk_dialog_run(thresholdDialog) == GTK_RESPONSE_APPLY) {
        image_editor_apply_threshold_filter(&(self->m_editor), gtk_adjustment_get_value(thresholdCutoff));
        canvas_refresh(self);
    }

    // destroy the dialog widget
    gtk_widget_destroy(GTK_WIDGET(thresholdDialog));

    // unref the builder
    g_object_unref(builder);
}



//
// EDITORWINDOW setup methods
// One of these must be called immediately after getting a new EditorWindow instance.
//

/* Prepares the editorWindow instance based on an input width, height, and color
Should be called by the user exactly ONCE after getting a new instance of EditorWindow */
void editor_window_canvas_init_from_parameters(EditorWindow *self, int width, int height, GdkRGBA backgroundColor) {
    gtk_widget_set_size_request(GTK_WIDGET(self->m_canvasGLArea), width, height);

    // initialize the renderBuffer to be the correct size
    self->m_renderBuffer = malloc(sizeof(unsigned int) * 4 * width * height);

    image_editor_init_from_parameters(&(self->m_editor), width, height, backgroundColor);

    // refresh the canvas to show the initial pixelbuffer
    canvas_refresh(self);
}

/* Prepares the editorWindow instance based on an input filepath
Should be called by the user exactly ONCE after getting a new instance of EditorWindow */
void editor_window_canvas_init_from_file(EditorWindow *self, const char *filepath) {
    image_editor_init_from_file(&(self->m_editor), filepath);

    int width = image_editor_get_current_pixelbuffer(&(self->m_editor))->width;
    int height = image_editor_get_current_pixelbuffer(&(self->m_editor))->height;

    // set the size request
    gtk_widget_set_size_request(GTK_WIDGET(self->m_canvasGLArea), width, height);

    // initialize the renderBuffer to be the correct size
    self->m_renderBuffer = malloc(sizeof(unsigned int) * 4 * width * height);

    // refresh the canvas to show the initial pixelbuffer
    canvas_refresh(self);
}



//
// EDITORWINDOW lifecycle management
// These manage the editor windows life cycle, from initialization to destruction.
//

/* Returns a new instance of EditorWindow */
EditorWindow* editor_window_new () {
    return g_object_new (EDITOR_WINDOW_TYPE_WINDOW, NULL);
}

/* Frees the dynamically allocated memory associated with this window,
then destroys itself. */
void editor_window_destroy(EditorWindow *self) {
    image_editor_destroy(&(self->m_editor));
    free(self->m_renderBuffer);
    gtk_widget_destroy(GTK_WIDGET(self));
}

/* Asks the user if they want to save before quitting */
void editor_window_quit(EditorWindow *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/io_dialogs.glade");

    // get an instance of quit from it
    GtkDialog *quitConfirmationDialog =
        GTK_DIALOG(gtk_builder_get_object(builder, "quitConfirmationDialog"));

    // run the dialog, then destroy it
    int response = gtk_dialog_run(quitConfirmationDialog);
    gtk_widget_destroy(GTK_WIDGET(quitConfirmationDialog));

    // and take action based on the response
    if (response == GTK_RESPONSE_YES) {
        file_save(self);
    }

    // destroy the editor
    image_editor_destroy(&(self->m_editor));

    // then finally destroy this instance
    gtk_widget_destroy(GTK_WIDGET(self));
}

/* Initializes the EditorWindow class */
static void editor_window_class_init (EditorWindowClass *class) {
    // GObject property stuff would go here...
}

/* Initializies the instance of EditorWindow. This is called automatically when
you request a new EditorWindow instance with 'editor_window_new()''. */
static void editor_window_init (EditorWindow *self) {
    //
    // PARENT WINDOW setup
    //
    gtk_window_set_title(GTK_WINDOW(self), "TinyPaint");
    gtk_window_set_resizable(GTK_WINDOW(self), 0);
    g_signal_connect(self, "delete-event", (GCallback)editor_window_quit, NULL);



    //
    // EDITOR setup
    //
    self->m_editor = image_editor_new();



    //
    // TOOLS WINDOW setup
    //

    // create a new ToolsWindow instance and link it to this EditorWindow instance
    self->m_tools_window = tools_window_new();
    tools_window_link_editorWindow(self->m_tools_window, self);

    // create a new Tool instance and assign it to the ToolsWindow
    tools_window_link_tool(self->m_tools_window, &(self->m_editor.m_tool));

    // present the ToolsWindow
    gtk_window_present(GTK_WINDOW(self->m_tools_window));



    //
    // GUI setup
    //

    // create a gtk builder from the resources
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/editor_window.glade");

    // get a reference to the editorBox
    GtkWidget *editorBox = GTK_WIDGET(gtk_builder_get_object(builder, "editorBox"));

    // and add it to this window and show it
    gtk_container_add(GTK_CONTAINER(self), editorBox);
    gtk_widget_show_all(editorBox);



    //
    // MENU BUTTONS setup
    //

    // menu NEW
    GtkMenuItem *newButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "newButton"));
    g_signal_connect_swapped(newButton, "activate", (GCallback)file_new, self);

    // menu OPEN
    GtkMenuItem *openButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "openButton"));
    g_signal_connect_swapped(openButton, "activate", (GCallback)file_open, self);

    // menu SAVE
    GtkMenuItem *saveButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "saveButton"));
    g_signal_connect_swapped(saveButton, "activate", (GCallback)file_save, self);

    // menu QUIT
    GtkMenuItem *quitButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "quitButton"));
    g_signal_connect_swapped(quitButton, "activate", (GCallback)gtk_window_close, GTK_WINDOW(self));

    // menu UNDO
    GtkMenuItem *undoButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "undoButton"));
    g_signal_connect_swapped(undoButton, "activate", (GCallback)image_editor_undo, &(self->m_editor));
    g_signal_connect_swapped(undoButton, "activate", (GCallback)canvas_refresh, self);

    // menu REDO
    GtkMenuItem *redoButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "redoButton"));
    g_signal_connect_swapped(redoButton, "activate", (GCallback)image_editor_redo, &(self->m_editor));
    g_signal_connect_swapped(redoButton, "activate", (GCallback)canvas_refresh, self);

    // menu SATURATION
    GtkMenuItem *saturationButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "saturationButton"));
    g_signal_connect_swapped(saturationButton, "activate", (GCallback)filter_applySaturation, self);

    // menu CHANNELS
    GtkMenuItem *channelsButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "channelsButton"));
    g_signal_connect_swapped(channelsButton, "activate", (GCallback)filter_applyChannels, self);

    // menu BRIGHTNESS_CONTRAST
    GtkMenuItem *brightnessContrastButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "brightnessContrastButton"));
    g_signal_connect_swapped(brightnessContrastButton, "activate", (GCallback)filter_applyBrightnessContrast, self);

    // menu INVERT
    GtkMenuItem *invertButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "invertButton"));
    g_signal_connect_swapped(invertButton, "activate", (GCallback)image_editor_apply_invert_filter, &(self->m_editor));
    g_signal_connect_swapped(invertButton, "activate", (GCallback)canvas_refresh, self);

    // menu GAUSSIAN_BLUR
    GtkMenuItem *gaussianBlurButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "gaussianBlurButton"));
    g_signal_connect_swapped(gaussianBlurButton, "activate", (GCallback)filter_applyGaussianBlur, self);

    // menu MOTION_BLUR
    GtkMenuItem *motionBlurButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "motionBlurButton"));
    g_signal_connect_swapped(motionBlurButton, "activate", (GCallback)filter_applyMotionBlur, self);

    // menu SHARPEN
    GtkMenuItem *sharpenButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "sharpenButton"));
    g_signal_connect_swapped(sharpenButton, "activate", (GCallback)filter_applySharpen, self);

    // menu EDGE DETECT
    GtkMenuItem *edgeDetectButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "edgeDetectButton"));
    g_signal_connect_swapped(edgeDetectButton, "activate", (GCallback)image_editor_apply_edge_detect_filter, &(self->m_editor));
    g_signal_connect_swapped(edgeDetectButton, "activate", (GCallback)canvas_refresh, self);

    // menu POSTERIZE
    GtkMenuItem *posterizeButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "posterizeButton"));
    g_signal_connect_swapped(posterizeButton, "activate", (GCallback)filter_applyPosterize, self);

    // menu THRESHOLD
    GtkMenuItem *thresholdButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "thresholdButton"));
    g_signal_connect_swapped(thresholdButton, "activate", (GCallback)filter_applyThreshold, self);

    // menu ABOUT
    GtkMenuItem *aboutButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "aboutButton"));
    GtkDialog *aboutDialog = GTK_DIALOG(gtk_builder_get_object(builder, "aboutDialog"));
    gtk_window_set_transient_for(GTK_WINDOW(self), GTK_WINDOW(aboutDialog));
    g_signal_connect(aboutDialog, "response", (GCallback)gtk_widget_hide, NULL);
    g_signal_connect_swapped(aboutButton, "activate", (GCallback)gtk_dialog_run, aboutDialog);



    //
    // DEVICE I/O setup
    //
    GtkWidget *canvasEventBox = GTK_WIDGET(gtk_builder_get_object(builder, "canvasEventBox"));
    g_signal_connect_swapped(canvasEventBox, "button-press-event", (GCallback)device_mouseClick, self);
    g_signal_connect_swapped(canvasEventBox, "motion-notify-event", (GCallback)device_mouseMove, self);
    g_signal_connect_swapped(canvasEventBox, "button-release-event", (GCallback)device_mouseUnclick, self);
    g_signal_connect(self, "key-press-event", (GCallback)device_keyPress, NULL);



    //
    // MOUSE TRACKING variables setup
    //
    self->m_mousePosX = 0;
    self->m_mousePosY = 0;
    self->m_mousePosX_prev = 0;
    self->m_mousePosY_prev = 0;
    self->m_mouseDown = 0;



    //
    // CANVAS RENDER setup
    //
    self->m_canvasGLArea = GTK_GL_AREA(gtk_builder_get_object(builder, "canvasGLArea"));
    g_signal_connect_swapped(self->m_canvasGLArea, "render", (GCallback)canvas_render, self);



    // finally, unref the builder
    g_object_unref(builder);
}
