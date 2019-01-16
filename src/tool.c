//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "tool.h"

#include <math.h>  // pow(), sqrt()

#include "utilities.h"
#include "flood_fill.h"

// todo: switch all of these to pass colors by reference to save time

void tool_apply_to_pixelbuffer(Tool *tool, PixelBuffer *buffer, int x, int y) {
    int edgeLength = (2 * tool->radius) + 1;

    // go through each cell in the tool mask
    for (int j = 0; j < edgeLength; j++) {
        for (int i = 0; i < edgeLength; i++) {
            int maskIndex = j * edgeLength + i;
            int i_bufferPos = x + (i - tool->radius);
            int j_bufferPos = y + (j - tool->radius);

            // verify that the current mask cell is still within the canvas bounds
            if (i_bufferPos >= 0 && i_bufferPos < buffer->width &&
                j_bufferPos >= 0 && j_bufferPos < buffer->height &&
                tool->mask[maskIndex] > 0.0) {
                GdkRGBA currentColor = pixelbuffer_get_pixel(buffer, i_bufferPos, j_bufferPos);

                // set the current pixel color based on the tooltype
                switch (tool->tooltype) {
                case PENCIL:
                    pixelbuffer_set_pixel(buffer, i_bufferPos, j_bufferPos,
                        GdkRGBA_lerp(currentColor, tool->color, tool->mask[maskIndex]));
                    break;
                case BRUSH:
                    pixelbuffer_set_pixel(buffer, i_bufferPos, j_bufferPos,
                        GdkRGBA_lerp(currentColor, tool->color, tool->mask[maskIndex]));
                    break;
                case MARKER:
                    pixelbuffer_set_pixel(buffer, i_bufferPos, j_bufferPos,
                        GdkRGBA_lerp(currentColor, GdkRGBA_min(currentColor, tool->color), tool->mask[maskIndex]));
                    break;
                case SPRAYCAN:
                    pixelbuffer_set_pixel(buffer, i_bufferPos, j_bufferPos,
                        GdkRGBA_lerp(currentColor, tool->color, tool->mask[maskIndex] * 0.05));
                    break;
                case FLOODFILL:
                    flood_fill(buffer, x, y, currentColor, tool->color);
                    break;
                case ERASER:
                    pixelbuffer_set_pixel(buffer, i_bufferPos, j_bufferPos,
                        GdkRGBA_lerp(currentColor, buffer->backgroundColor, tool->mask[maskIndex]));
                    break;
                }
            }
        }
    }
}

void tool_reset_parameters(Tool *tool) {
    // set the "default" tool parameters
    tool->applyWhenStationary = 0;
    tool->fillRate = 1.0;
    tool->isStamp = 0;

    // then change as needed based on the tooltype
    switch (tool->tooltype) {
    case PENCIL:
        break;
    case BRUSH:
        break;
    case MARKER:
        tool->applyWhenStationary = 1;
        tool->fillRate = 0.85;
        break;
    case SPRAYCAN:
        tool->applyWhenStationary = 1;
        tool->fillRate = 0.375;
        break;
    case FLOODFILL:
        tool->isStamp = 1;
        break;
    case ERASER:
        break;
    }
}

void tool_update_mask(Tool *tool) {
    // if the mask has memory allocated to it, then free it first
    // NOTE: Tool instances MUST be created with "tool_new()" method, or else
    // their "mask" parameter will default to random memory location and this
    // free() call will cause coredump
    if (tool->mask != NULL) {
        free(tool->mask);
    }

    // calculate the radius and edgeLength of the mask
    double radius = tool->radius;
    int edgeLength = (2 * radius) + 1;

    // then allocate a new memory block of the correct size
    tool->mask = malloc(sizeof(double) * edgeLength * edgeLength);

    // and fill the mask array based on the tooltype
    for (int x = 0; x < edgeLength; x++) {
        for (int y = 0; y < edgeLength; y++) {
            int maskIndex = y * edgeLength + x;
            double distanceFromCenter = sqrt(pow(x - radius, 2.0) + pow(y - radius, 2.0));
            double distanceNormalized = (radius == 0) ? 0.0 : distanceFromCenter / radius;

            switch (tool->tooltype) {
            case PENCIL:
                tool->mask[maskIndex] = distanceFromCenter <= radius;
                break;
            case BRUSH:
                tool->mask[maskIndex] = 1.0 - double_clamp(distanceNormalized, 0.0, 1.0);
                break;
            case MARKER:
                tool->mask[maskIndex] = (x / (double)edgeLength) >= 0.25 && (x / (double)edgeLength) <= 0.75;
                break;
            case SPRAYCAN:
                tool->mask[maskIndex] = 1.0 - double_clamp(distanceNormalized, 0.0, 1.0);
                break;
            case FLOODFILL:
                tool->mask[maskIndex] = 1.0;  // not technically required but in apply code
                // we check if the mask is not 0.0 and don't bother applying in that case
                break;
            case ERASER:
                tool->mask[maskIndex] = distanceFromCenter <= radius;
                break;
            }
        }
    }
}

Tool tool_new() {
    Tool tmp;
    GdkRGBA color = {1.0, 0.0, 0.0, 1.0};
    tmp.tooltype = PENCIL;
    tmp.radius = 5;
    tmp.color = color;
    tmp.mask = NULL;
    tmp.applyWhenStationary = 0;
    tmp.isStamp = 0;
    tmp.fillRate = 1.0;
    return tmp;
}
