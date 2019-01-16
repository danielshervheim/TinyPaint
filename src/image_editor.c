//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "image_editor.h"

#include "utilities.h"

#include "lodepng.h"


#include <math.h>  // pow, sqrt













// private methods


void image_editor_history_clear_redo(ImageEditor *self) {
    for (int i = 0; i < self->m_redoIndex; i++) {
        pixelbuffer_destroy(&(self->m_redoStates[i]));
    }
    self->m_redoIndex = 0;
}

void image_editor_history_clear_undo(ImageEditor *self) {
    for (int i = 0; i < self->m_undoIndex; i++) {
        pixelbuffer_destroy(&(self->m_undoStates[i]));
    }
    self->m_undoIndex = 0;
}


void image_editor_history_update(ImageEditor *self) {
    // clear the redo history
    image_editor_history_clear_redo(self);

    // if the saved states array is full...
    if (self->m_undoIndex == MAX_HISTORY_STATES - 1) {
        // destroy the oldest saved state
        pixelbuffer_destroy(&(self->m_undoStates[0]));

        // and shift the remaining states to make room for a new one
        for (int i = 1; i < MAX_HISTORY_STATES; i++) {
            self->m_undoStates[i-1] = self->m_undoStates[i];
        }

        // and decrement the index
        self->m_undoIndex--;
    }

    // then make a copy of the current buffer
    PixelBuffer copy = pixelbuffer_copy(image_editor_get_current_pixelbuffer(self));
    self->m_undoIndex++;
    self->m_undoStates[self->m_undoIndex] = copy;
}







// public methods



ImageEditor image_editor_new() {
    ImageEditor tmp;
    tmp.m_tool = tool_new();
    tmp.m_undoIndex = 0;
    tmp.m_redoIndex = 0;
    return tmp;
}

void image_editor_init_from_parameters(ImageEditor *self, int width, int height, GdkRGBA backgroundColor) {
    // clear the history states, if for by some reason they were not already empty...
    image_editor_history_clear_redo(self);
    image_editor_history_clear_undo(self);

    self->m_undoStates[self->m_undoIndex] = pixelbuffer_new(width, height);
    pixelbuffer_set_all_pixels(image_editor_get_current_pixelbuffer(self), backgroundColor);
}

void image_editor_init_from_file(ImageEditor *self, const char *filepath) {
    // assumes the gui ensures the input filepath is valid

    unsigned error;
    unsigned char *tmp;
    unsigned width, height;

    // load the png from the filepath
    error = lodepng_decode32_file(&tmp, &width, &height, filepath);
    if (error) {
        printf("error %u: %s\n", error, lodepng_error_text(error));
    }

    // create the buffer with the correct size and default background color of white
    GdkRGBA color = {1.0, 1.0, 1.0, 1.0};
    image_editor_init_from_parameters(self, width, height, color);

    // set all the pixels in the buffer as from the loaded file
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int offset = (width * 4 * y) + (x*4);
            color.red = tmp[offset + 0] / (double)255.0;
            color.green = tmp[offset + 1] / (double)255.0;
            color.blue = tmp[offset + 2] / (double)255.0;
            color.alpha = tmp[offset + 3] / (double)255.0;
            pixelbuffer_set_pixel(image_editor_get_current_pixelbuffer(self), x, y, GdkRGBA_clamp(color, 0.0, 1.0));
        }
    }

    // free the temporary buffer
    free(tmp);
}

void image_editor_destroy(ImageEditor *self) {
    image_editor_history_clear_redo(self);
    image_editor_history_clear_undo(self);
}







PixelBuffer* image_editor_get_current_pixelbuffer(ImageEditor *self) {
    return &(self->m_undoStates[self->m_undoIndex]);
}

void image_editor_save_current_pixelbuffer(ImageEditor *self, const char *filepath) {
    // assumes gui enforces the passed in filepath is valid

    // get the current buffer
    PixelBuffer *current = image_editor_get_current_pixelbuffer(self);

    // allocate space for the temporary buffer
    unsigned char *tmp = malloc(4 * current->width * current->height);

    // fill the temporary buffer in the correct format
    for (int y = 0; y < current->height; y++) {
        for (int x = 0; x < current->width; x++) {
            GdkRGBA color = pixelbuffer_get_pixel(current, x, y);
            int offset = (current->width*4*y) + (x*4);
            memset(tmp + offset + 0, color.red * 255, 1);
            memset(tmp + offset + 1, color.green * 255, 1);
            memset(tmp + offset + 2, color.blue * 255, 1);
            memset(tmp + offset + 3, color.alpha * 255, 1);
        }
    }

    // attempt to encode with lodepng library
    unsigned error = lodepng_encode32_file(filepath, tmp, current->width, current->height);
    if (error) {
        printf("error %u: %s\n", error, lodepng_error_text(error));
    }

    // free the temporary buffer
    free(tmp);
}






void image_editor_stroke_start(ImageEditor *self, int x, int y) {
    image_editor_history_update(self);
    PixelBuffer *current = image_editor_get_current_pixelbuffer(self);
    if (x >= 0 && x < current->width && y >= 0 && y < current->height) {
        tool_apply_to_pixelbuffer(&(self->m_tool), current, x, y);
    }
}

void image_editor_stroke_hold(ImageEditor *self, int x, int y) {
    PixelBuffer *current = image_editor_get_current_pixelbuffer(self);

    if (x >= 0 && x < current->width && y >= 0 && y < current->height) {
        tool_apply_to_pixelbuffer(&(self->m_tool), current, x, y);
    }
}

void image_editor_stroke_move(ImageEditor *self, int x, int y, int prevX, int prevY) {
    if (!self->m_tool.isStamp) {
        PixelBuffer *current = image_editor_get_current_pixelbuffer(self);

        int distance = sqrt(pow(x-prevX, 2.0) + pow(y-prevY, 2.0));
        int stepSize = (1-self->m_tool.fillRate) * (distance-1) + 1;

        for (int i = 0; i < distance; i += stepSize) {
            double percent = i/(double)distance;
            int tmpX = (int)double_lerp(prevX, x, percent);
            int tmpY = (int)double_lerp(prevY, y, percent);

            if (tmpX >= 0 && tmpX < current->width && tmpY >= 0 && tmpY < current->height) {
                tool_apply_to_pixelbuffer(&(self->m_tool), current, tmpX, tmpY);
            }
        }
    }
}

void image_editor_stroke_end(ImageEditor *self, int x, int y) {
    if (!self->m_tool.isStamp) {
        PixelBuffer *current = image_editor_get_current_pixelbuffer(self);
        if (x >= 0 && x < current->width && y >= 0 && y < current->height) {
            tool_apply_to_pixelbuffer(&(self->m_tool), current, x, y);
        }
    }
}










void image_editor_undo(ImageEditor *self) {
    printf("undo\n");
    if (self->m_undoIndex > 0 && self->m_undoIndex < MAX_HISTORY_STATES) {
        self->m_redoStates[self->m_redoIndex] = self->m_undoStates[self->m_undoIndex];
        self->m_redoIndex++;
        self->m_undoIndex--;
    }
}

void image_editor_redo(ImageEditor *self) {
    printf("redo\n");
    if (self->m_redoIndex > 0 && self->m_redoIndex < MAX_HISTORY_STATES) {
        self->m_redoIndex--;
        self->m_undoIndex++;
        self->m_undoStates[self->m_undoIndex] = self->m_redoStates[self->m_redoIndex];
    }
}









// within these functions, make sure the first thing is calling history_update
void image_editor_apply_saturation_filter(ImageEditor *self, double scale) {
    printf("image_editor_apply_saturation_filter\n");
}

void image_editor_apply_channels_filter(ImageEditor *self, double r, double g, double b) {
    printf("image_editor_apply_channels_filter\n");
}

void image_editor_apply_invert_filter(ImageEditor *self) {
    printf("image_editor_apply_invert_filter\n");
}

void image_editor_apply_brightness_contrast_filter(ImageEditor *self, double brightness, double contrast) {
    printf("image_editor_apply_brightness_contrast_filter\n");
}

void image_editor_apply_gaussian_blur_filter(ImageEditor *self, int radius) {
    printf("image_editor_apply_gaussian_blur_filter\n");
}

void image_editor_apply_motion_blur_filter(ImageEditor *self, int radius, double angle) {
    printf("image_editor_apply_motion_blur_filter\n");
}

void image_editor_apply_sharpen_filter(ImageEditor *self, int radius) {
    printf("image_editor_apply_sharpen_filter\n");
}

void image_editor_apply_edge_detect_filter(ImageEditor *self) {
    printf("image_editor_apply_edge_detect_filter\n");
}

void image_editor_apply_posterize_filter(ImageEditor *self, int numBins) {
    printf("image_editor_apply_posterize_filter\n");
}

void image_editor_apply_threshold_filter(ImageEditor *self, double cutoff) {
    printf("image_editor_apply_threshold_filter\n");
}
