//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "filter.h"

#include "utilities.h"

#include <gdk/gdk.h>  // GdkRGBA
#include <math.h>  // round



//
// PIXEL CALCULATION methods
//

GdkRGBA calculate_pixel_saturation(GdkRGBA color, SaturationParams *params) {
    double lum = GdkRGBA_luminance(color);
    GdkRGBA desat = {lum, lum, lum, color.alpha};
    return GdkRGBA_lerp(desat, color, params->scale);
}

GdkRGBA calculate_pixel_channels(GdkRGBA color, ChannelsParams *params) {
    GdkRGBA scale = {params->r_scale, params->g_scale, params->b_scale, 1.0};
    return GdkRGBA_multiply(color, scale);
}

GdkRGBA calculate_pixel_invert(GdkRGBA color) {
    GdkRGBA tmp = {1.0 - color.red, 1.0 - color.green, 1.0 - color.blue, color.alpha};
    return tmp;
}

GdkRGBA calculate_pixel_brightness_contrast(GdkRGBA color, BrightnessContrastParams *params) {
    // adjust the brightness
    GdkRGBA scalar = {params->brightness_scale, params->brightness_scale, params->brightness_scale, 0.0};
    GdkRGBA tmp = GdkRGBA_add(color, scalar);
    tmp = GdkRGBA_clamp(tmp, 0.0, 1.0);

    // calculate the contrast adjustment factor "F"
    double c = (255*params->contrast_scale);
    double f = (259 * (c + 255)) / (255 * (259 - c));

    double r = (tmp.red*255);
    double g = (tmp.green*255);
    double b = (tmp.blue*255);

    r = f*(r - 128) + 128;
    g = f*(g - 128) + 128;
    b = f*(b - 128) + 128;

    tmp.red = double_clamp(r, 0, 255)/255.0;
    tmp.green = double_clamp(g, 0, 255)/255.0;
    tmp.blue = double_clamp(b, 0, 255)/255.0;
    tmp.alpha = color.alpha;

    return tmp;
}

GdkRGBA calculate_pixel_posterize(GdkRGBA color, PosterizeParams *params) {
    // num bins = 1 ... 256
    int num_steps = params->num_bins - 1;
    double r = round(color.red * num_steps) / num_steps;
    double g = round(color.green * num_steps) / num_steps;
    double b = round(color.blue * num_steps) / num_steps;
    GdkRGBA tmp = {r, g, b, color.alpha};
    return tmp;
}

GdkRGBA calculate_pixel_threshold(GdkRGBA color, ThresholdParams *params) {

    double lum = GdkRGBA_luminance(color);

    int num = (lum > params->cutoff);
    GdkRGBA tmp = {num, num, num, 1.0};
    return tmp;
}



//
// FILTER APPLICATION method
//
void apply_basic_filter_to_pixelbuffer(FilterType type, void *params, PixelBuffer *buffer) {
    printf("applying basic");
    for (int y = 0; y < buffer->height; y++) {
        for (int x = 0; x < buffer->width; x++) {
            GdkRGBA currentColor = pixelbuffer_get_pixel(buffer, x, y);
            GdkRGBA newColor;
            if (type == SATURATION) {
                newColor = calculate_pixel_saturation(currentColor, (SaturationParams *)params);
            }
            else if (type == CHANNELS) {
                newColor = calculate_pixel_channels(currentColor, (ChannelsParams *)params);
            }
            else if (type == INVERT) {
                newColor = calculate_pixel_invert(currentColor);
            }
            else if (type == BRIGHTNESSCONTRAST) {
                newColor = calculate_pixel_brightness_contrast(currentColor, (BrightnessContrastParams *)params);
            }
            else if (type == POSTERIZE) {
                newColor = calculate_pixel_posterize(currentColor, (PosterizeParams *)params);
            }
            else if (type == THRESHOLD) {
                newColor = calculate_pixel_threshold(currentColor, (ThresholdParams *)params);
            }

            pixelbuffer_set_pixel(buffer, x, y, GdkRGBA_clamp(newColor, 0.0, 1.0));
        }
    }
}
