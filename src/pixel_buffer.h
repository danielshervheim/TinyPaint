//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#ifndef PIXEL_BUFFER_H_
#define PIXEL_BUFFER_H_

#include <gdk/gdk.h>  // GdkRGBA

typedef struct pixelbuffer {
    int width;
    int height;
    GdkRGBA *data;
    GdkRGBA backgroundColor;
    float *rgbadata;
} PixelBuffer;

/* Returns a new pixelbuffer of width x height. */
PixelBuffer pixelbuffer_new(int width, int height);

/* Frees the memory allocated to the input buffer. */
void pixelbuffer_destroy(PixelBuffer *buf);

/* Copies and returns the input buffer. */
PixelBuffer pixelbuffer_copy(PixelBuffer *original);

/* Sets the pixel at x,y to color. */
void pixelbuffer_set_pixel(PixelBuffer *buf, int x, int y, GdkRGBA color);

/* Returns the color of the pixel at x,y. */
GdkRGBA pixelbuffer_get_pixel(PixelBuffer *buf, int x, int y);

/* Sets all pixels (and the backgroundColor) to color. */
void pixelbuffer_set_all_pixels(PixelBuffer *buf, GdkRGBA color);

#endif  // PIXEL_BUFFER_H_
