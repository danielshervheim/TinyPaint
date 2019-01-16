//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "filter.h"

#include "utilities.h"

#include <gdk/gdk.h>  // GdkRGBA
#include <math.h>  //


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








void apply_basic_filter(FilterType type, void *params, PixelBuffer *buffer) {
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

    /*
        for (y < height) {
            for (x < width) {
                if (type == SATURATION) {
                    pixelbuffer(x, y) = calculate_pixel_saturation(currentColor, (SaturationParams *)params)
            }
        }
    }
    */
}

void kernel_scale(double *kernel, int edgeLength, double scale) {
    for (int y = 0; y < edgeLength; y++) {
        for (int x = 0; x < edgeLength; x++) {
            kernel[y * edgeLength + x] *= scale;
        }
    }
}


double kernel_sum(double *kernel, int edgeLength) {

    float sum = 0;

    for (int y = 0; y < edgeLength; y++) {
        for (int x = 0; x < edgeLength; x++) {
            sum += kernel[y * edgeLength + x];
        }
    }

    return sum;
}

void kernel_normalize(double *kernel, int edgeLength) {
    double sum = kernel_sum(kernel, edgeLength);

    if (sum <= 0) {
        return;
    }

    double val;

    for (int y = 0; y < edgeLength; y++) {
        for (int x = 0; x < edgeLength; x++) {
            val = kernel[y * edgeLength + x];
            kernel[y * edgeLength + x] = (val / sum);
        }
    }
}


double *create_kernel_gaussian_blur(GaussianBlurParams *params, int *radiusIn, int *edgeLengthIn) {

    // calculate the radius and edgeLength of the mask
    int radius = params->radius;
    int edgeLength = (2 * radius) + 1;

    double *kernel = malloc(sizeof(double) * edgeLength * edgeLength);

    for (int y = 0; y < edgeLength; y++) {
        for (int x = 0; x < edgeLength; x++) {
            int kernelIndex = y * edgeLength + x;

            double distanceFromCenter = sqrt(pow(x - radius, 2.0) + pow(y - radius, 2.0));
            kernel[kernelIndex] = double_gaussian(distanceFromCenter, radius);
        }
    }

    kernel_normalize(kernel, edgeLength);
    *radiusIn = radius;
    *edgeLengthIn = edgeLength;
    return kernel;
}

double *create_kernel_motion_blur(MotionBlurParams *params, int *radiusIn, int *edgeLengthIn) {
    // calculate the radius and edgeLength of the mask
    int radius = params->radius;
    int edgeLength = (2 * radius) + 1;

    double *kernel = malloc(sizeof(double) * edgeLength * edgeLength);

    for (int y = 0; y < edgeLength; y++) {
        for (int x = 0; x < edgeLength; x++) {
            // normalize u, v so [0, 0] in the center of kernel
            // rather than in the top left
            int u = x - radius;
            int v = y - radius;

            // a fudge factor to determine how much of a pixel must be on the
            // line to consider it part of the kernel
            double epsilon = 0.75;

            int kernelIndex = y * edgeLength + x;

            // set the value to 0 initially
            kernel[kernelIndex] = 0.0;
            if (params->angle < 0.25*M_PI || params->angle > 0.75*M_PI) {
                if (double_abs(v - (u * tan(params->angle))) < epsilon) {
                    kernel[kernelIndex] = 1.0;
                }
            }
            else {
                // we must solve for y rather than x to
                // avoid limits of tan tending to infinity
                if (double_abs(u - (v / tan(params->angle))) < epsilon) {
                    kernel[kernelIndex] = 1.0;
                }
            }
        }
    }

    kernel_normalize(kernel, edgeLength);
    *radiusIn = radius;
    *edgeLengthIn = edgeLength;
    return kernel;
}

double *create_kernel_sharpen(SharpenParams *params, int *radiusIn, int *edgeLengthIn) {
    // calculate the radius and edgeLength of the mask
    int radius;
    int edgeLength;

    double *kernel = create_kernel_gaussian_blur((GaussianBlurParams *)params, &radius, &edgeLength);
    kernel_scale(kernel, edgeLength, -1.0);

    int centerIndex = radius * (edgeLength + 1);
    kernel[centerIndex] += 2.0;

    *radiusIn = radius;
    *edgeLengthIn = edgeLength;
    return kernel;
}

double *create_kernel_edge_detect(int *radiusIn, int *edgeLengthIn) {
    // calculate the radius and edgeLength of the mask
    int radius = 1;
    int edgeLength = (2 * radius) + 1;

    double *kernel = malloc(sizeof(double) * edgeLength * edgeLength);


    for (int y = 0; y < edgeLength; y++) {
        for (int x = 0; x < edgeLength; x++) {
            int kernelIndex = y * edgeLength + x;
            kernel[kernelIndex] = -1.0;
        }
    }

    int centerIndex = radius*edgeLength + radius;
    kernel[centerIndex] = pow(edgeLength, 2.0) - 1;

    //kernel_normalize(kernel, edgeLength);
    *radiusIn = radius;
    *edgeLengthIn = edgeLength;
    return kernel;
}





void apply_convolution_filter(FilterType type, void *params, PixelBuffer *buffer) {
    double *kernel = NULL;
    int radius;
    int edgeLength;

    if (type == GAUSSIANBLUR) {
        kernel = create_kernel_gaussian_blur((GaussianBlurParams *)params, &radius, &edgeLength);
    }
    else if (type == MOTIONBLUR) {
        kernel = create_kernel_motion_blur((MotionBlurParams *)params, &radius, &edgeLength);
    }
    else if (type == SHARPEN) {
        kernel = create_kernel_sharpen((SharpenParams *)params, &radius, &edgeLength);
    }
    else if (type == EDGEDETECT) {
        kernel = create_kernel_edge_detect(&radius, &edgeLength);
    }

    // convolution filter requires a copy of the pixelbuffer
    PixelBuffer copy = pixelbuffer_copy(buffer);

    // for each pixel in the buffer...
    for (int y = 0; y < buffer->height; y++) {
        for (int x = 0; x < buffer->width; x++) {
            GdkRGBA accum = {0.0, 0.0, 0.0, 1.0};

            // convolve the kernel over it
            for (int v = 0; v < edgeLength; v++) {
                for (int u = 0; u < edgeLength; u++) {
                    int kernelIndex = (v * edgeLength + u);

                    int u_onBuffer = x + (u - radius);
                    int v_onBuffer = y + (v - radius);

                    u_onBuffer = int_clamp(u_onBuffer, 0, buffer->width-1);
                    v_onBuffer = int_clamp(v_onBuffer, 0, buffer->height-1);

                    accum = GdkRGBA_add(accum, GdkRGBA_scale(pixelbuffer_get_pixel(&copy, u_onBuffer, v_onBuffer), kernel[kernelIndex]));
                }
            }

            // and set the pixel
            pixelbuffer_set_pixel(buffer, x, y, GdkRGBA_clamp(accum, 0.0, 1.0));
        }
    }

    // then destroy the copy and free the kernel
    pixelbuffer_destroy(&copy);
    free(kernel);
}



void apply_filter_to_pixelbuffer(FilterType type, void *params, PixelBuffer *buffer) {
    if (type == SATURATION) {
        apply_basic_filter(type, params, buffer);
    }
    else if (type == CHANNELS) {
        apply_basic_filter(type, params, buffer);
    }
    else if (type == INVERT) {
        apply_basic_filter(type, params, buffer);
    }
    else if (type == BRIGHTNESSCONTRAST) {
        apply_basic_filter(type, params, buffer);
    }
    else if (type == GAUSSIANBLUR) {
        apply_convolution_filter(type, params, buffer);
    }
    else if (type == MOTIONBLUR) {
        apply_convolution_filter(type, params, buffer);
    }
    else if (type == SHARPEN) {
        apply_convolution_filter(type, params, buffer);
    }
    else if (type == EDGEDETECT) {
        apply_convolution_filter(type, params, buffer);
    }
    else if (type == POSTERIZE) {
        apply_basic_filter(type, params, buffer);
    }
    else if (type == THRESHOLD) {
        apply_basic_filter(type, params, buffer);
    }
}
