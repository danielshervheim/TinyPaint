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



ImageEditor image_editor_new();

// you must call one of these immediately after getting a new instance
void image_editor_init_from_parameters(ImageEditor *self, int width, int height, GdkRGBA backgroundColor);
void image_editor_init_from_file(ImageEditor *self, const char *filepath);



void image_editor_destroy(ImageEditor *self);

PixelBuffer* image_editor_get_current_pixelbuffer(ImageEditor *self);
void image_editor_save_current_pixelbuffer(ImageEditor *self, const char *filepath);

void image_editor_stroke_start(ImageEditor *self, int x, int y);
void image_editor_stroke_hold(ImageEditor *self, int x, int y);
void image_editor_stroke_move(ImageEditor *self, int x, int y, int prevX, int prevY);
void image_editor_stroke_end(ImageEditor *self, int x, int y);

void image_editor_undo(ImageEditor *self);
void image_editor_redo(ImageEditor *self);

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

/*
private

void image_editor_history_update(ImageEditor *self);
void image_editor_history_clear_redo(ImageEditor *self);
void image_editor_history_clear_undo(ImageEditor *self);


*/





#endif  // IMAGE_EDITOR_H_
