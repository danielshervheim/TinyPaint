//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "filter.h"

#include "kernel.h"
#include "utilities.h"

#include <gdk/gdk.h>  // GdkRGBA
#include <math.h>  // pow, sqrt
#include <pthread.h> // pthread

/* How many threads to spawn for the filter application. I have found 25 is the
goldilocks zone. On average, it reduces filter application times by a factor of 10. */
#define NUM_THREADS 25

/* Whether multithreading is enabled or not. This is mainly for debugging
purposes. 0 is disabled, 1 is enabled. */
#define MULTITHREADING 1



//
// CONVOLUTION KERNEL creation methods
//

Kernel create_gaussian_blur_kernel(GaussianBlurParams *params) {
    Kernel tmp = kernel_new(params->radius);

    for (int y = 0; y < tmp.edgeLength; y++) {
        for (int x = 0; x < tmp.edgeLength; x++) {
            double distanceFromCenter = sqrt(pow(x - tmp.radius, 2.0) + pow(y - tmp.radius, 2.0));
            kernel_set_value(&tmp, x, y, double_gaussian(distanceFromCenter, tmp.radius));
        }
    }

    kernel_normalize(&tmp);
    return tmp;
}

Kernel create_motion_blur_kernel(MotionBlurParams *params) {
    Kernel tmp = kernel_new(params->radius);

    for (int y = 0; y < tmp.edgeLength; y++) {
        for (int x = 0; x < tmp.edgeLength; x++) {
            // set to 0 initially
            kernel_set_value(&tmp, x, y, 0.0);

            // calculate the offset, so the center value is at 0,0
            int u = x - tmp.radius;
            int v = y - tmp.radius;

            // a fudge factor to determine what percent of a pixel must be on the
            // line to consider it as part of the kernel
            double epsilon = 0.75;

            if (params->angle < 0.25*M_PI || params->angle > 0.75*M_PI) {
                if (double_abs(v - (u * tan(params->angle))) < epsilon) {
                    kernel_set_value(&tmp, x, y, 1.0);
                }
            }
            else {
                // solve for x rather than y, else limits of tan tend towards infinity
                if (double_abs(u - (v / tan(params->angle))) < epsilon) {
                    kernel_set_value(&tmp, x, y, 1.0);
                }
            }
        }
    }

    kernel_normalize(&tmp);
    return tmp;
}

Kernel create_sharpen_kernel(SharpenParams *params) {
    Kernel tmp = create_gaussian_blur_kernel((GaussianBlurParams *)params);

    kernel_scale(&tmp, -1.0);

    kernel_set_value(&tmp, tmp.radius, tmp.radius,
        kernel_get_value(&tmp, tmp.radius, tmp.radius) + 2);

    return tmp;
}

Kernel create_edge_detect_kernel() {
    Kernel tmp = kernel_new(1);

    // kernel_new defaults to 1.0 all over the kernel
    kernel_scale(&tmp, -1.0);
    kernel_set_value(&tmp, tmp.radius, tmp.radius, pow(tmp.edgeLength, 2.0) - 1);

    return tmp;
}



//
// CONVOLUTION FILTER application methods
//

/* a struct to hold all the members needed to apply the convolution filter,
so we can pass it to a pthread. */
typedef struct convolution_worker_args {
    PixelBuffer *read;
    PixelBuffer *write;
    Kernel *kernel;
    int i;
    int n;
} ConvolutionWorkerArgs;

/* Many of these are spawned and work in parallel, on seperate parts of the image,
to convolve the kernel over the image and apply the filter. */
void* convolution_worker(void *data) {
    ConvolutionWorkerArgs *args = (ConvolutionWorkerArgs *)data;

    // this shouldn't happen in theory....
    if (args->read->width != args->write->width || args->read->height != args->write->height) {
        printf("ERROR: pixelbuffer dimension mismatch in convolution worker\n");
        return NULL;
    }

    int w = args->read->width;
    int h = args->read->height;

    // calculate the start and end points this specific thread will work on
    int start = (w/args->n) * args->i;
    int end = (w/args->n) * (args->i + 1) - 1;

    /* if it is the final thread, we must set the endpoint manually or else it
    could miss a few pixels due to rounding errors. */
    if (args->i == args->n - 1) {
        end = w - 1;
    }

    /* Applies the kernel to its portion of pixels. */
    for (int y = 0; y < h; y++) {
        for (int x = start; x <= end; x++) {
            // accumulator
            GdkRGBA accum = {0.0, 0.0, 0.0, 1.0};

            // convolve the kernel over the current pixel
            for (int v = 0; v < args->kernel->edgeLength; v++) {
                for (int u = 0; u < args->kernel->edgeLength; u++) {
                        // calculate the position of the current kernel value on the buffer
                        int u_onBuffer = x + (u - args->kernel->radius);
                        int v_onBuffer = y + (v - args->kernel->radius);

                        // and clamp it to be within the buffer bounds
                        u_onBuffer = int_clamp(u_onBuffer, 0, args->read->width-1);
                        v_onBuffer = int_clamp(v_onBuffer, 0, args->read->height-1);

                        // calculate the value of the current pixel convolved
                        GdkRGBA currentValue = GdkRGBA_scale(pixelbuffer_get_pixel(args->read,
                            u_onBuffer, v_onBuffer), kernel_get_value(args->kernel, u, v));

                        accum = GdkRGBA_add(accum, currentValue);
                }
            }

            // and set the updated pixel
            pixelbuffer_set_pixel(args->write, x, y, GdkRGBA_clamp(accum, 0.0, 1.0));
        }
    }

    return NULL;
}


void apply_convolution_filter_to_pixelbuffer(FilterType type, void *params, PixelBuffer *buffer) {
    Kernel kernel;

    if (type == GAUSSIANBLUR) {
        kernel = create_gaussian_blur_kernel((GaussianBlurParams *)params);
    }
    else if (type == MOTIONBLUR) {
        kernel = create_motion_blur_kernel((MotionBlurParams *)params);
    }
    else if (type == SHARPEN) {
        kernel = create_sharpen_kernel((SharpenParams *)params);
    }
    else if (type == EDGEDETECT) {
        kernel = create_edge_detect_kernel();
    }

    // convolution filter requires a copy of the pixelbuffer.
    PixelBuffer copy = pixelbuffer_copy(buffer);

    if (MULTITHREADING == 1) {
        // create an array of thread ids, and int ids to pass into the helper threads.
        pthread_t tids[NUM_THREADS];
        ConvolutionWorkerArgs args[NUM_THREADS];

        /* we define these first, and each in their own array or else race
        conditions in the worker thread can cause multiple threads to get the
        same id, and consequently work on the same part of the image. */
        for (int i = 0; i < NUM_THREADS; i++) {
            ConvolutionWorkerArgs arg;
            arg.read = &copy;
            arg.write = buffer;
            arg.kernel = &kernel;
            arg.i = i;
            arg.n = NUM_THREADS;
            args[i] = arg;
        }

        // spawn the helper threads and pass in the necessary info as a struct.
        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_create(&tids[i], NULL, convolution_worker, (void *)(&args[i]));
        }

        // then wait for each thread to finish.
        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_join(tids[i], NULL);
        }
    }
    else {  // multithreading disabled.
        for (int y = 0; y < buffer->height; y++) {
            for (int x = 0; x < buffer->width; x++) {
                // accumulator
                GdkRGBA accum = {0.0, 0.0, 0.0, 1.0};

                // convolve the kernel over the current pixel
                for (int v = 0; v < kernel.edgeLength; v++) {
                    for (int u = 0; u < kernel.edgeLength; u++) {
                        // calculate the position of the current kernel value on the buffer
                        int u_onBuffer = x + (u - kernel.radius);
                        int v_onBuffer = y + (v - kernel.radius);

                        // and clamp it to be within the buffer bounds
                        u_onBuffer = int_clamp(u_onBuffer, 0, buffer->width-1);
                        v_onBuffer = int_clamp(v_onBuffer, 0, buffer->height-1);

                        // calculate the value of the current pixel convolved
                        GdkRGBA currentValue = GdkRGBA_scale(pixelbuffer_get_pixel(&copy,
                            u_onBuffer, v_onBuffer), kernel_get_value(&kernel, u, v));

                        accum = GdkRGBA_add(accum, currentValue);
                    }
                }

                // and set the updated pixel
                pixelbuffer_set_pixel(buffer, x, y, GdkRGBA_clamp(accum, 0.0, 1.0));
            }
        }
    }

    // and free the temporarily allocated memory.
    pixelbuffer_destroy(&copy);
    kernel_destroy(&kernel);
}
