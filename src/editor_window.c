//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "editor_window.h"

#include "tools_window.h"
#include "tool.h"
#include "pixel_buffer.h"
#include "utilities.h"  // double_lerp()

#include "filter.h"

#include "lodepng.h"

#include <GL/gl.h>  // openGL headers
#include <math.h>  // pow(), sqrt()

struct _EditorWindow {
    GtkWindow parent_instance;

    // instance properties
    ToolsWindow *m_tools_window;
    Tool m_current_tool;

    // the canvas
    GtkGLArea *m_canvasGLArea;

    // the history states
    PixelBuffer m_historyUndoStates[MAX_HISTORY_STATES];  // todo: eventually put this number in a pref file
    int m_historyUndo_i;

    PixelBuffer m_historyRedoStates[MAX_HISTORY_STATES];
    int m_historyRedo_i;

    unsigned int *m_renderBuffer;

    // mouse tracking
    int m_mousePosX;
    int m_mousePosY;
    int m_mousePosX_prev;
    int m_mousePosY_prev;
    int m_mouseDown;
    double m_mouseDelta;
};

G_DEFINE_TYPE(EditorWindow, editor_window, GTK_TYPE_WINDOW);








//
// CANVAS / GL related methods
//

/* Rerenders the gtkGLArea associated with this EditorWindow instance, based on its current pixelbuffer */
void editor_window_canvas_render(EditorWindow *self, GdkGLContext *context) {
    // make the context of the canvas current
    gdk_gl_context_make_current (context);

    // clear the buffer
    glClearColor (0, 0, 0, 1);
    glClear (GL_COLOR_BUFFER_BIT);

    // get a reference to the instances current PixelBuffer (this is just to save
    // space when typing. you could just use the longhand "self->m_history....")
    PixelBuffer *render = &(self->m_historyUndoStates[self->m_historyUndo_i]);

    // update this instances renderBuffer based on the current PixelBuffer
    for (int y = 0; y < render->height; y++) {
        for (int x = 0; x < render->width; x++) {
            // OpenGL renders upside down... not sure why...
            int y_modified = (render->height - 1) - y;
            GdkRGBA currentColor = pixelbuffer_get_pixel(render, x, y);
            memset(self->m_renderBuffer + (render->width * 4 * y_modified) + (4 * x) + 0, currentColor.red * 255, sizeof(unsigned int));
            memset(self->m_renderBuffer + (render->width * 4 * y_modified) + (4 * x) + 1, currentColor.green * 255, sizeof(unsigned int));
            memset(self->m_renderBuffer + (render->width * 4 * y_modified) + (4 * x) + 2, currentColor.blue * 255, sizeof(unsigned int));
            memset(self->m_renderBuffer + (render->width * 4 * y_modified) + (4 * x) + 3, currentColor.alpha * 255, sizeof(unsigned int));
        }
    }

    // finally, draw the new array of pixels to the screen
    glDrawPixels(render->width, render->height, GL_RGBA, GL_UNSIGNED_INT, self->m_renderBuffer);
}

/* Refreshes this instance's GL canvas (basically triggers the "render" signal) */
void editor_window_canvas_refresh(EditorWindow *self) {
    gtk_gl_area_queue_render(self->m_canvasGLArea);
}



//
// HISTORY methods
//

/* Replaces the current pixelbuffer with the previously saved one */
void history_undo(EditorWindow *self) {
    if (self->m_historyUndo_i > 0 && self->m_historyUndo_i < MAX_HISTORY_STATES) {
        self->m_historyRedoStates[self->m_historyRedo_i] = self->m_historyUndoStates[self->m_historyUndo_i];
        self->m_historyRedo_i++;
        self->m_historyUndo_i--;
    }
    else {
        // nothing to undo
    }

    editor_window_canvas_refresh(self);
}

/* Replaces the current pixelbuffer with the next-recent one */
void history_redo(EditorWindow *self) {
    if (self->m_historyRedo_i > 0 && self->m_historyRedo_i < MAX_HISTORY_STATES) {
        self->m_historyRedo_i--;
        self->m_historyUndo_i++;
        self->m_historyUndoStates[self->m_historyUndo_i] = self->m_historyRedoStates[self->m_historyRedo_i];
    }
    else {
        // nothing to redo
    }

    editor_window_canvas_refresh(self);
}

/* Clears one of the m_historyXXXStates arrays */
void history_clear(PixelBuffer array[], int *index) {
    // clear out the array
    for (int i = 0; i < *index; i++) {
        pixelbuffer_destroy(&array[i]);
    }

    // and set the index back to 0
    *index = 0;
}

/* Copies the current pixelbuffer into the history array */
void history_update(EditorWindow *self) {
    // an update should "wipeout" any saved redo states, so
    // clear the redo array
    history_clear(self->m_historyRedoStates, &(self->m_historyRedo_i));

    if (self->m_historyUndo_i == MAX_HISTORY_STATES - 1) {
    	// destroy the oldest state
        pixelbuffer_destroy(&(self->m_historyUndoStates[0]));

        // shift remaining states down
        for (int i = 1; i < MAX_HISTORY_STATES; i++) {
            self->m_historyUndoStates[i-1] = self->m_historyUndoStates[i];
        }

        // decrement index accordingly
        self->m_historyUndo_i--;
    }

    // otherwise, copy the current pixelbuffer, advance the index, and save the copy
    PixelBuffer copy = pixelbuffer_copy(&(self->m_historyUndoStates[self->m_historyUndo_i]));
    self->m_historyUndo_i++;
    self->m_historyUndoStates[self->m_historyUndo_i] = copy;

	editor_window_canvas_refresh(self);
}



//
// EDITOR WINDOW input/output
//

/* Saves the current pixelbuffer of the EditorWindow instance to a .png file */
void editor_window_save(EditorWindow *self) {
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

        // destroy saveDialog widget
        gtk_widget_destroy(GTK_WIDGET(saveDialog));

        // attempt to save file
        int width = self->m_historyUndoStates[self->m_historyUndo_i].width;
        int height = self->m_historyUndoStates[self->m_historyUndo_i].height;
        unsigned char *tmp = malloc(4 * width * height);

        // fill the char array with the current pixel buffer
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                GdkRGBA currentColor = pixelbuffer_get_pixel(&(self->m_historyUndoStates[self->m_historyUndo_i]), x, y);
                int offset = (width * 4 * y) + (x*4);
                memset(tmp + offset + 0, currentColor.red * 255, 1);
                memset(tmp + offset + 1, currentColor.green * 255, 1);
                memset(tmp + offset + 2, currentColor.blue * 255, 1);
                memset(tmp + offset + 3, currentColor.alpha * 255, 1);
            }
        }

        // attempt to encode with lodepng library
        unsigned error = lodepng_encode32_file(filename_final, tmp, width, height);
        if(error) {
            printf("error %u: %s\n", error, lodepng_error_text(error));
        }

        // free the tmp buffer
        free(tmp);
    }
    else {
        gtk_widget_destroy(GTK_WIDGET(saveDialog));
    }

    // finally, unref the builder
    g_object_unref(builder);
}



//
// EDITOR WINDOW management
//

/* Fires when the user intends to close the window */
void editor_window_quit(EditorWindow *self) {
    printf("quitting\n");

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
        editor_window_save(self);
    }

    // then finally destroy this instance
    gtk_widget_destroy(GTK_WIDGET(self));
}

/* Forks a new process and starts a new TinyPaintApp instance */
void editor_window_start_new() {
    int pid = fork();

    if (pid < 0) {
        printf("fork() failed\n");
    }
    else if (pid == 0) {
        execl("/usr/bin/tinypaint", "tinypaint", (char *)NULL);
        printf("exec() failed\n");
    }
}

/* Asks the user for a file to open, then fork-execs a new process to open it */
void editor_window_open_new() {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/io_dialogs.glade");

    // get a reference to the openDialog from it
    GtkFileChooserDialog *openDialog =
        GTK_FILE_CHOOSER_DIALOG(gtk_builder_get_object(builder, "openDialog"));

    if (gtk_dialog_run(GTK_DIALOG(openDialog)) == GTK_RESPONSE_OK) {
        // get file string
        gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(openDialog));

        printf("filename: %s\n", filename);

        int pid = fork();

        if (pid < 0) {
            printf("fork() failed\n");
        }
        else if (pid == 0) {
            execl("/usr/bin/tinypaint", "tinypaint", filename, (char *)NULL);
            printf("exec() failed\n");
        }
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(openDialog));

    g_object_unref(builder);
}

/* Fires when the user presses a key when the EditorWindow is active */
int editor_window_keyPress(EditorWindow *self, GdkEventKey *event) {
    if (event->keyval == 122 && event->state == 20) {
        // (CTRL+Z) undo
        history_undo(self);
    }
    else if (event->keyval == 90 && event->state == 21) {
        // (CTRL+LSHIFT+Z) redo
        history_redo(self);
    }
    else if (event->keyval == 113 && event->state == 20) {
        // (CTRL+Q) quit
        gtk_window_close(GTK_WINDOW(self));
    }
    else if (event->keyval == 110 && event->state == 20) {
        // (CTRL+N) new
        editor_window_start_new();
    }
    else if (event->keyval == 111 && event->state == 20) {
        // (CTRL+O) open
        editor_window_open_new();
    }
    else if (event->keyval == 115 && event->state == 20) {
        // (CTRL+S) save
        editor_window_save(self);
    }

    // return false, i.e. other signals can interupt this one (to enable multi-key combinations)
    return 0;
}



//
// CANVAS interaction related methods
//

/* Fires when the mouse is held down on the canvas */
int canvas_mouseHold(EditorWindow *self) {
    PixelBuffer current = self->m_historyUndoStates[self->m_historyUndo_i];
    if (self->m_mousePosX >= 0 && self->m_mousePosX < current.width &&
        self->m_mousePosY >= 0 && self->m_mousePosY < current.height) {
        tool_apply_to_pixelbuffer(&(self->m_current_tool), &self->m_historyUndoStates[self->m_historyUndo_i],
            self->m_mousePosX, self->m_mousePosY);
    }

    editor_window_canvas_refresh(self);
    return self->m_mouseDown;
}

/* Fires when the mouse is moved on the canvas */
void canvas_mouseMove(EditorWindow *self, GdkEventMotion *event) {
    if (self->m_mouseDown) {
        // update the mouse tracking parameters
        self->m_mousePosX_prev = self->m_mousePosX;
        self->m_mousePosY_prev = self->m_mousePosY;
        self->m_mousePosX = event->x;
        self->m_mousePosY = event->y;
        self->m_mouseDelta = sqrt(pow(self->m_mousePosX - self->m_mousePosX_prev,
            2.0) + pow(self->m_mousePosY - self->m_mousePosY_prev, 2.0));

        if (!self->m_current_tool.isStamp) {
            // stroke cont
            PixelBuffer current = self->m_historyUndoStates[self->m_historyUndo_i];
            int distance = (int)self->m_mouseDelta;
            int stepSize = (1 - self->m_current_tool.fillRate) * (distance-1) + 1;

            for (int i = 0; i < distance; i += stepSize) {
                double progress = (i/(double)distance);
                int x = (int)double_lerp(self->m_mousePosX_prev, self->m_mousePosX, progress);
                int y = (int)double_lerp(self->m_mousePosY_prev, self->m_mousePosY, progress);

                if (x >= 0 && x < current.width && y >= 0 && y < current.height) {
                    tool_apply_to_pixelbuffer(&(self->m_current_tool), &(self->m_historyUndoStates[self->m_historyUndo_i]), x, y);
                }
            }

            editor_window_canvas_refresh(self);
        }
    }
}

/* Fires when the mouse is pressed down on the canvas */
void canvas_mouseDown(EditorWindow *self, GdkEventMotion *event) {
    // update the mouse tracking parameters
    self->m_mouseDown = 1;
    self->m_mousePosX = event->x;
    self->m_mousePosY = event->y;
    self->m_mousePosX_prev = self->m_mousePosX;
    self->m_mousePosY_prev = self->m_mousePosY;

    // install the mouseHold polling function
    if (self->m_current_tool.applyWhenStationary) {
        g_timeout_add(MOUSE_HOLD_POLL_RATE_MS, (void *)canvas_mouseHold, self);
    }

    // stroke start
    history_update(self);
    PixelBuffer current = self->m_historyUndoStates[self->m_historyUndo_i];
    if (self->m_mousePosX >= 0 && self->m_mousePosX < current.width &&
        self->m_mousePosY >= 0 && self->m_mousePosY < current.height) {
        tool_apply_to_pixelbuffer(&(self->m_current_tool), &self->m_historyUndoStates[self->m_historyUndo_i],
            self->m_mousePosX, self->m_mousePosY);
        editor_window_canvas_refresh(self);
    }
}

/* Fires when the mouse is depressed on the canvas */
void canvas_mouseUp(EditorWindow *self, GdkEventMotion *event) {
    // update the mouse tracking parameters
    self->m_mouseDown = 0;
    self->m_mousePosX = event->x;
    self->m_mousePosY = event->y;

    if (!self->m_current_tool.isStamp) {
        //stroke end
        PixelBuffer current = self->m_historyUndoStates[self->m_historyUndo_i];
        if (self->m_mousePosX >= 0 && self->m_mousePosX < current.width &&
            self->m_mousePosY >= 0 && self->m_mousePosY < current.height) {
            tool_apply_to_pixelbuffer(&(self->m_current_tool), &self->m_historyUndoStates[self->m_historyUndo_i],
                self->m_mousePosX, self->m_mousePosY);
        }
        editor_window_canvas_refresh(self);
    }
}





//
// FILTER methods
//




void apply_saturation_filter(EditorWindow *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/filter_dialogs.glade");

    // save refernces to the necessary widgets
    GtkDialog *saturationDialog = GTK_DIALOG(gtk_builder_get_object(builder, "saturationDialog"));
    GtkAdjustment *saturationScale = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "saturationScale"));

    // run the dialog
    if (gtk_dialog_run(saturationDialog) == GTK_RESPONSE_APPLY) {
        // save a copy of the current buffer
        history_update(self);

        /// get the parameters
        SaturationParams params = {gtk_adjustment_get_value(saturationScale)};

        // and apply the filter
        apply_filter_to_pixelbuffer(SATURATION, (void *)(&params), &(self->m_historyUndoStates[self->m_historyUndo_i]));
    }

    // destroy the dialog widget
    gtk_widget_destroy(GTK_WIDGET(saturationDialog));

    // unref the builder
    g_object_unref(builder);
}

void apply_channels_filter(EditorWindow *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/filter_dialogs.glade");

    // save refernces to the necessary widgets
    GtkDialog *channelsDialog = GTK_DIALOG(gtk_builder_get_object(builder, "channelsDialog"));
    GtkAdjustment *rScale = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "rScale"));
    GtkAdjustment *gScale = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "gScale"));
    GtkAdjustment *bScale = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "bScale"));

    // run the dialog
    if (gtk_dialog_run(channelsDialog) == GTK_RESPONSE_APPLY) {
        // save a copy of the current buffer
        history_update(self);

        /// get the parameters
        ChannelsParams params = {gtk_adjustment_get_value(rScale), gtk_adjustment_get_value(gScale), gtk_adjustment_get_value(bScale)};

        // and apply the filter
        apply_filter_to_pixelbuffer(CHANNELS, (void *)(&params), &(self->m_historyUndoStates[self->m_historyUndo_i]));
    }

    // destroy the dialog widget
    gtk_widget_destroy(GTK_WIDGET(channelsDialog));

    // unref the builder
    g_object_unref(builder);
}

void apply_brightness_contrast_filter(EditorWindow *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/filter_dialogs.glade");

    // save refernces to the necessary widgets
    GtkDialog *brightnessContrastDialog = GTK_DIALOG(gtk_builder_get_object(builder, "brightnessContrastDialog"));
    GtkAdjustment *brightnessScale = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "brightnessScale"));
    GtkAdjustment *contrastScale = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "contrastScale"));

    // run the dialog
    if (gtk_dialog_run(brightnessContrastDialog) == GTK_RESPONSE_APPLY) {
        // save a copy of the current buffer
        history_update(self);

        /// get the parameters
        BrightnessContrastParams params = {gtk_adjustment_get_value(brightnessScale), gtk_adjustment_get_value(contrastScale)};

        // and apply the filter
        apply_filter_to_pixelbuffer(BRIGHTNESSCONTRAST, (void *)(&params), &(self->m_historyUndoStates[self->m_historyUndo_i]));
    }

    // destroy the dialog widget
    gtk_widget_destroy(GTK_WIDGET(brightnessContrastDialog));

    // unref the builder
    g_object_unref(builder);
}

void apply_invert_filter(EditorWindow *self) {
    history_update(self);

    apply_filter_to_pixelbuffer(INVERT, NULL, &(self->m_historyUndoStates[self->m_historyUndo_i]));
}

void apply_gaussian_blur_filter(EditorWindow *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/filter_dialogs.glade");

    // save refernces to the necessary widgets
    GtkDialog *gaussianBlurDialog = GTK_DIALOG(gtk_builder_get_object(builder, "gaussianBlurDialog"));
    GtkAdjustment *gaussianBlurRadius = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "gaussianBlurRadius"));

    // run the dialog
    if (gtk_dialog_run(gaussianBlurDialog) == GTK_RESPONSE_APPLY) {
        // save a copy of the current buffer
        history_update(self);

        /// get the parameters
        GaussianBlurParams params = {(int)gtk_adjustment_get_value(gaussianBlurRadius)};

        // and apply the filter
        apply_filter_to_pixelbuffer(GAUSSIANBLUR, (void *)(&params), &(self->m_historyUndoStates[self->m_historyUndo_i]));
    }

    // destroy the dialog widget
    gtk_widget_destroy(GTK_WIDGET(gaussianBlurDialog));

    // unref the builder
    g_object_unref(builder);
}

void apply_motion_blur_filter(EditorWindow *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/filter_dialogs.glade");

    // save refernces to the necessary widgets
    GtkDialog *motionBlurDialog = GTK_DIALOG(gtk_builder_get_object(builder, "motionBlurDialog"));
    GtkAdjustment *motionBlurRadius = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "motionBlurRadius"));
    GtkAdjustment *motionBlurAngle = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "motionBlurAngle"));

    // run the dialog
    if (gtk_dialog_run(motionBlurDialog) == GTK_RESPONSE_APPLY) {
        // save a copy of the current buffer
        history_update(self);

        /// get the parameters
        MotionBlurParams params = {(int)gtk_adjustment_get_value(motionBlurRadius), gtk_adjustment_get_value(motionBlurAngle)};

        // and apply the filter
        apply_filter_to_pixelbuffer(MOTIONBLUR, (void *)(&params), &(self->m_historyUndoStates[self->m_historyUndo_i]));
    }

    // destroy the dialog widget
    gtk_widget_destroy(GTK_WIDGET(motionBlurDialog));

    // unref the builder
    g_object_unref(builder);
}

void apply_sharpen_filter(EditorWindow *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/filter_dialogs.glade");

    // save refernces to the necessary widgets
    GtkDialog *sharpenDialog = GTK_DIALOG(gtk_builder_get_object(builder, "sharpenDialog"));
    GtkAdjustment *sharpenRadius = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "sharpenRadius"));

    // run the dialog
    if (gtk_dialog_run(sharpenDialog) == GTK_RESPONSE_APPLY) {
        // save a copy of the current buffer
        history_update(self);

        /// get the parameters
        SharpenParams params = {(int)gtk_adjustment_get_value(sharpenRadius)};

        // and apply the filter
        apply_filter_to_pixelbuffer(SHARPEN, (void *)(&params), &(self->m_historyUndoStates[self->m_historyUndo_i]));
    }

    // destroy the dialog widget
    gtk_widget_destroy(GTK_WIDGET(sharpenDialog));

    // unref the builder
    g_object_unref(builder);
}

void apply_edge_detect_filter(EditorWindow *self) {
    // save a copy of the current buffer
    history_update(self);
    // and apply the filter
    apply_filter_to_pixelbuffer(EDGEDETECT, (void *)NULL, &(self->m_historyUndoStates[self->m_historyUndo_i]));
}

void apply_posterize_filter(EditorWindow *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/filter_dialogs.glade");

    // save refernces to the necessary widgets
    GtkDialog *posterizeDialog= GTK_DIALOG(gtk_builder_get_object(builder, "posterizeDialog"));
    GtkAdjustment *posterizeBins = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "posterizeBins"));

    // run the dialog
    if (gtk_dialog_run(posterizeDialog) == GTK_RESPONSE_APPLY) {
        // save a copy of the current buffer
        history_update(self);

        /// get the parameters
        PosterizeParams params = {(int)gtk_adjustment_get_value(posterizeBins)};

        // and apply the filter
        apply_filter_to_pixelbuffer(POSTERIZE, (void *)(&params), &(self->m_historyUndoStates[self->m_historyUndo_i]));
    }

    // destroy the dialog widget
    gtk_widget_destroy(GTK_WIDGET(posterizeDialog));

    // unref the builder
    g_object_unref(builder);
}

void apply_threshold_filter(EditorWindow *self) {
    // get a reference to the builder
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/filter_dialogs.glade");

    // save refernces to the necessary widgets
    GtkDialog *thresholdDialog = GTK_DIALOG(gtk_builder_get_object(builder, "thresholdDialog"));
    GtkAdjustment *thresholdCutoff = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "thresholdCutoff"));

    // run the dialog
    if (gtk_dialog_run(thresholdDialog) == GTK_RESPONSE_APPLY) {
        // save a copy of the current buffer
        history_update(self);

        /// get the parameters
        ThresholdParams params = {gtk_adjustment_get_value(thresholdCutoff)};

        // and apply the filter
        apply_filter_to_pixelbuffer(THRESHOLD, (void *)(&params), &(self->m_historyUndoStates[self->m_historyUndo_i]));
    }

    // destroy the dialog widget
    gtk_widget_destroy(GTK_WIDGET(thresholdDialog));

    // unref the builder
    g_object_unref(builder);
}




//
//  EDITOR initialization methods
//


/* Prepares the editorWindow instance based on an input width, height, and color
Should be called by the user exactly ONCE after getting a new instance of EditorWindow */
void editor_window_canvas_init(EditorWindow *self, int width, int height, GdkRGBA backgroundColor) {
    gtk_widget_set_size_request(GTK_WIDGET(self->m_canvasGLArea), width, height);

    // initialize the renderBuffer to be the correct size
    self->m_renderBuffer = malloc(sizeof(unsigned int) * 4 * width * height);

    // create the initial PixelBuffer and set its background color
    self->m_historyUndoStates[0] = pixelbuffer_new(width, height);
    pixelbuffer_set_all_pixels(&(self->m_historyUndoStates[0]), backgroundColor);

    // refresh the canvas to show the initial pixelbuffer
    editor_window_canvas_refresh(self);
}

/* Prepares the editorWindow instance based on an input filepath
Should be called by the user exactly ONCE after getting a new instance of EditorWindow */
void editor_window_canvas_init_with_image_path(EditorWindow *self, const char *path) {
    printf("%s\n", path);
    // load the image into memory
    unsigned error;
    unsigned char* data;
    unsigned width, height;

    error = lodepng_decode32_file(&data, &width, &height, path);

    // if there was a problem loading, then quit
    if (error) {
        printf("error %u: %s\n", error, lodepng_error_text(error));
        gtk_widget_destroy(GTK_WIDGET(self));
    }

    // set the size request
    gtk_widget_set_size_request(GTK_WIDGET(self->m_canvasGLArea), width, height);

    // initialize the renderBuffer to be the correct size
    self->m_renderBuffer = malloc(sizeof(unsigned int) * 4 * width * height);

    // create the initial PixelBuffer and set its background color
    self->m_historyUndoStates[0] = pixelbuffer_new(width, height);

    // set its backgroundColor to be white by default
    GdkRGBA color = {1.0, 1.0, 1.0, 1.0};
    pixelbuffer_set_all_pixels(&(self->m_historyUndoStates[0]), color);

    // set all pixels of pixelbuffer from input image
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int offset = (width * 4 * y) + (x*4);
            color.red = data[offset + 0] / (double)255.0;
            color.green = data[offset + 1] / (double)255.0;
            color.blue = data[offset + 2] / (double)255.0;
            color.alpha = data[offset + 3] / (double)255.0;
            pixelbuffer_set_pixel(&(self->m_historyUndoStates[0]), x, y, GdkRGBA_clamp(color, 0.0, 1.0));
        }
    }

    // clear loaded png from memory
    free(data);

    // refresh the canvas to show the initial pixelbuffer
    editor_window_canvas_refresh(self);
}

/* Initializies the instance of EditorWindow */
static void editor_window_init (EditorWindow *self) {
    //
    // INSTANCE setup
    //

    // set the window title and properties
    gtk_window_set_title(GTK_WINDOW(self), "TinyPaint");
    gtk_window_set_resizable(GTK_WINDOW(self), 0);

    // set the destroy signal overide
    g_signal_connect(self, "delete-event", (GCallback)editor_window_quit, NULL);
    g_signal_connect(self, "key-press-event", (GCallback)editor_window_keyPress, NULL);



    //
    // TOOLS WINDOW setup
    //

    // create a new ToolsWindow instance and link it to this EditorWindow instance
    self->m_tools_window = tools_window_new();
    tools_window_link_editorWindow(self->m_tools_window, self);

    // create a new Tool instance and assign it to the ToolsWindow
    self->m_current_tool = tool_new();
    tools_window_link_tool(self->m_tools_window, &(self->m_current_tool));

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
    // MENU setup
    //

    // menu NEW
    GtkMenuItem *newButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "newButton"));
    g_signal_connect(newButton, "activate", (GCallback)editor_window_start_new, NULL);

    // menu OPEN
    GtkMenuItem *openButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "openButton"));
    g_signal_connect(openButton, "activate", (GCallback)editor_window_open_new, NULL);

    // menu SAVE
    GtkMenuItem *saveButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "saveButton"));
    g_signal_connect_swapped(saveButton, "activate", (GCallback)editor_window_save, self);

    // menu QUIT
    GtkMenuItem *quitButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "quitButton"));
    g_signal_connect_swapped(quitButton, "activate", (GCallback)gtk_window_close, GTK_WINDOW(self));

    // menu UNDO
    GtkMenuItem *undoButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "undoButton"));
    g_signal_connect_swapped(undoButton, "activate", (GCallback)history_undo, self);

    // menu REDO
    GtkMenuItem *redoButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "redoButton"));
    g_signal_connect_swapped(redoButton, "activate", (GCallback)history_redo, self);

    // menu SATURATION
    GtkMenuItem *saturationButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "saturationButton"));
    g_signal_connect_swapped(saturationButton, "activate", (GCallback)apply_saturation_filter, self);

    // menu CHANNELS
    GtkMenuItem *channelsButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "channelsButton"));
    g_signal_connect_swapped(channelsButton, "activate", (GCallback)apply_channels_filter, self);

    // menu BRIGHTNESS_CONTRAST
    GtkMenuItem *brightnessContrastButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "brightnessContrastButton"));
    g_signal_connect_swapped(brightnessContrastButton, "activate", (GCallback)apply_brightness_contrast_filter, self);

    // menu INVERT
    GtkMenuItem *invertButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "invertButton"));
    g_signal_connect_swapped(invertButton, "activate", (GCallback)apply_invert_filter, self);

    // menu GAUSSIAN_BLUR
    GtkMenuItem *gaussianBlurButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "gaussianBlurButton"));
    g_signal_connect_swapped(gaussianBlurButton, "activate", (GCallback)apply_gaussian_blur_filter, self);

    // menu MOTION_BLUR
    GtkMenuItem *motionBlurButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "motionBlurButton"));
    g_signal_connect_swapped(motionBlurButton, "activate", (GCallback)apply_motion_blur_filter, self);

    // menu SHARPEN
    GtkMenuItem *sharpenButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "sharpenButton"));
    g_signal_connect_swapped(sharpenButton, "activate", (GCallback)apply_sharpen_filter, self);

    // menu EDGE DETECT
    GtkMenuItem *edgeDetectButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "edgeDetectButton"));
    g_signal_connect_swapped(edgeDetectButton, "activate", (GCallback)apply_edge_detect_filter, self);

    // menu POSTERIZE
    GtkMenuItem *posterizeButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "posterizeButton"));
    g_signal_connect_swapped(posterizeButton, "activate", (GCallback)apply_posterize_filter, self);

    // menu THRESHOLD
    GtkMenuItem *thresholdButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "thresholdButton"));
    g_signal_connect_swapped(thresholdButton, "activate", (GCallback)apply_threshold_filter, self);

    // menu ABOUT
    GtkMenuItem *aboutButton = GTK_MENU_ITEM(gtk_builder_get_object(builder, "aboutButton"));
    GtkDialog *aboutDialog = GTK_DIALOG(gtk_builder_get_object(builder, "aboutDialog"));
    gtk_window_set_transient_for(GTK_WINDOW(self), GTK_WINDOW(aboutDialog));
    g_signal_connect(aboutDialog, "response", (GCallback)gtk_widget_hide, NULL);
    g_signal_connect_swapped(aboutButton, "activate", (GCallback)gtk_dialog_run, aboutDialog);



    //
    // CANVAS interaction setup
    //

    // get canvasEventBox reference and set their signal handlers
    GtkWidget *canvasEventBox = GTK_WIDGET(gtk_builder_get_object(builder, "canvasEventBox"));
    g_signal_connect_swapped(canvasEventBox, "button-press-event", (GCallback)canvas_mouseDown, self);
    g_signal_connect_swapped(canvasEventBox, "motion-notify-event", (GCallback)canvas_mouseMove, self);
    g_signal_connect_swapped(canvasEventBox, "button-release-event", (GCallback)canvas_mouseUp, self);



    //
    // INSTANCE PARAMETERS setup
    //

    // get canvas reference (and save it as instance parameter) and set its signal handler
    self->m_canvasGLArea = GTK_GL_AREA(gtk_builder_get_object(builder, "canvasGLArea"));
    g_signal_connect_swapped(self->m_canvasGLArea, "render", (GCallback)editor_window_canvas_render, self);

    // set the initial history indexes
    self->m_historyUndo_i = 0;
    self->m_historyRedo_i = 0;

    // set the initial prev pos
    self->m_mousePosX_prev = 0;
    self->m_mousePosY_prev = 0;

    // finally, unref the builder
    g_object_unref(builder);
}

/* Returns a new instance of EditorWindow */
EditorWindow* editor_window_new () {
    return g_object_new (EDITOR_WINDOW_TYPE_WINDOW, NULL);
}

/* Initializes the EditorWindow class */
static void editor_window_class_init (EditorWindowClass *class) {
    // GObject property stuff would go here...
}
