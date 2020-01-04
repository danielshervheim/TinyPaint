//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#ifndef IMAGE_EDITOR_H_
#define IMAGE_EDITOR_H_

#include "pixel_buffer.h"  // PixelBuffer
#include "tool.h"  // Tool

#include <gdk/gdk.h>  // GdkRGBA

#define MAX_HISTORY_STATES 10

typedef struct image_editor {

    PixelBuffer m_undoStates[MAX_HISTORY_STATES];
    int m_undoIndex;
    PixelBuffer m_redoStates[MAX_HISTORY_STATES];
    int m_redoIndex;

    Tool m_tool;

} ImageEditor;

/* Returns a new ImadeEditor instance.
Note: you must immediately call init_from_parameters or init_from_file before using the new instance!! */
ImageEditor image_editor_new();

/* Sets up the instance with a new pixelbuffer based on the input parameters. */
void image_editor_init_from_parameters(ImageEditor *self, int width, int height, GdkRGBA backgroundColor);

/* Imports the file at filepath, and sets up the instance with a new pixelbuffer based on it.
Note: this assumes that the given filepath is valid! */
void image_editor_init_from_file(ImageEditor *self, const char *filepath);

/* Clears all the dynamic memory associated with the instance. */
void image_editor_destroy(ImageEditor *self);

/* Returns a pointer to the current pixelbuffer. This is the buffer that the gui should render to the screen. */
PixelBuffer* image_editor_get_current_pixelbuffer(ImageEditor *self);

/* Saves the current pixelbuffer to a file at filepath.
Note: this assumes that the given filepath is valid! */
void image_editor_save_current_pixelbuffer(ImageEditor *self, const char *filepath);

/* When the user has began a stroke on the canvas. */
void image_editor_stroke_start(ImageEditor *self, int x, int y);

/* When the user is holding a stroke in place on the canvas. */
void image_editor_stroke_hold(ImageEditor *self, int x, int y);

/* When the user is moving a stroke on the canvas. */
void image_editor_stroke_move(ImageEditor *self, int x, int y, int prevX, int prevY);

/* When the user has ended a stroke on the canvas. */
void image_editor_stroke_end(ImageEditor *self, int x, int y);

/* Undoes the previously saved state (either stroke or filter application). */
void image_editor_undo(ImageEditor *self);

/* Redoes the previously undone state (eiter stroke of filter application). */
void image_editor_redo(ImageEditor *self);

/* The following functions just apply their respective filters to the current pixelbuffer */
void image_editor_apply_saturation_filter(ImageEditor *self, double scale);
void image_editor_apply_channels_filter(ImageEditor *self, double r, double g, double b);
void image_editor_apply_invert_filter(ImageEditor *self);
void image_editor_apply_brightness_contrast_filter(ImageEditor *self, double brightness, double contrast);
void image_editor_apply_gaussian_blur_filter(ImageEditor *self, int radius);
void image_editor_apply_motion_blur_filter(ImageEditor *self, int radius, double angle);
void image_editor_apply_sharpen_filter(ImageEditor *self, int radius);
void image_editor_apply_edge_detect_filter(ImageEditor *self);
void image_editor_apply_posterize_filter(ImageEditor *self, int numBins);
void image_editor_apply_threshold_filter(ImageEditor *self, double cutoff);

#endif  // IMAGE_EDITOR_H_
