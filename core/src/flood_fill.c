//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "flood_fill.h"
#include "utilities.h"

/*
    TinyPaint uses Adam Milazzo's excellent improved flood-fill algorithm.
    The function(s) below are basically a one-to-one port of his pseudocode. I
    only extended the algorithm to compare 4-channel RGBA colors, rather than
    boolean values. His pseudocode, and website can be found at the link below:

    http://www.adammil.net/blog/v126_A_More_Efficient_Flood_Fill.html
*/

/* These function headers are declared here rather than in flood_fill.h because
they should not really be accessable to the programmer. */
void flood_fill_stage2(PixelBuffer *buffer, int x, int y, GdkRGBA target, GdkRGBA replacement);
void flood_fill_stage3(PixelBuffer *buffer, int x, int y, GdkRGBA target, GdkRGBA replacement);



/* Returns whether the current pixel needs replacing, based on the target color */
int pixelNeedsReplacement(PixelBuffer *buffer, int x, int y, GdkRGBA target) {
    return GdkRGBA_equals(pixelbuffer_get_pixel(buffer, x, y), target, 0.05);
}

void flood_fill(PixelBuffer *buffer, int x, int y, GdkRGBA target, GdkRGBA replacement) {
    if (x < 0 || x >= buffer->width) {
        return;
    }

    if (y < 0 || y >= buffer->height) {
        return;
    }

    if (GdkRGBA_equals(target, replacement, 0.05)) {
        return;
    }

    if (!pixelNeedsReplacement(buffer, x, y, target)) {
        return;
    }

    flood_fill_stage2(buffer, x, y, target, replacement);
}

void flood_fill_stage2(PixelBuffer *buffer, int x, int y, GdkRGBA target, GdkRGBA replacement) {
    while (1) {
        int ox = x;
        int oy = y;

        while (y != 0 && pixelNeedsReplacement(buffer, x, y-1, target)) {
            y--;
        }

        while (x != 0 && pixelNeedsReplacement(buffer, x-1, y, target)) {
            x--;
        }

        if (x == ox && y == oy) {
            break;
        }
    }
    flood_fill_stage3(buffer, x, y, target, replacement);
}

void flood_fill_stage3(PixelBuffer *buffer, int x, int y, GdkRGBA target, GdkRGBA replacement) {
    int lastRowLength = 0;
    do {
        int rowLength = 0;
        int sx = x;

        if (lastRowLength != 0 && !pixelNeedsReplacement(buffer, x, y, target)) {
            do {
                if(--lastRowLength == 0) {
                    return;
                }
            } while (!pixelNeedsReplacement(buffer, ++x, y, target));
            sx = x;
        }
        else {
            for (; x != 0 && pixelNeedsReplacement(buffer, x-1, y, target); rowLength++, lastRowLength++) {
                pixelbuffer_set_pixel(buffer, --x, y, replacement);

                if (y != 0 && pixelNeedsReplacement(buffer, x, y-1, target)) {
                    flood_fill_stage2(buffer, x, y-1, target, replacement);
                }
            }
        }

        for (; sx < buffer->width && pixelNeedsReplacement(buffer, sx, y, target); rowLength++, sx++) {
            pixelbuffer_set_pixel(buffer, sx, y, replacement);
        }

        if (rowLength < lastRowLength) {
            for (int end = x + lastRowLength; ++sx < end; ) {
                if (pixelNeedsReplacement(buffer, sx, y, target)) {
                    flood_fill_stage3(buffer, sx, y, target, replacement);
                }
            }
        }
        else if (rowLength > lastRowLength && y != 0) {
            for (int ux = x + lastRowLength; ++ux<sx; ) {
                if (pixelNeedsReplacement(buffer, ux, y-1, target)) {
                    flood_fill_stage2(buffer, ux, y-1, target, replacement);
                }
            }
        }

        lastRowLength = rowLength;
    } while (lastRowLength != 0 && ++y < buffer->height);
}
