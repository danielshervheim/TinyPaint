//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#ifndef KERNEL_H_
#define KERNEL_H_

typedef struct kernel {
    int radius;
    int edgeLength;
    double *data;
} Kernel;

/* Returns a new kernel. */
Kernel kernel_new(int radius);

/* Frees the memory allocated for the kernel. */
void kernel_destroy(Kernel *self);

/* Returns the value of the kernel at x, y. */
double kernel_get_value(Kernel *self, int x, int y);

/* Sets the value of the kernel at x, y. */
void kernel_set_value(Kernel *self, int x, int y, double value);

/* Normalizes each value so the kernel sums to 1.0. */
void kernel_normalize(Kernel *self);

/* Scales each value in the kernel by a scalar. */
void kernel_scale(Kernel *self, double scale);

/* Returns the sum of all values in the kernel. */
double kernel_sum(Kernel *self);

#endif  // KERNEL_H_
