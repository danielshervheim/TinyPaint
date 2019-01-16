//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "kernel.h"

#include <math.h>  // pow, sqrt
#include <stdlib.h>  // malloc

Kernel kernel_new(int radius) {
    Kernel tmp;
    tmp.radius = radius;
    tmp.edgeLength = (2*radius)+1;
    tmp.data = malloc(sizeof(double) * tmp.edgeLength * tmp.edgeLength);

    for (int y = 0; y < tmp.edgeLength; y++) {
        for (int x = 0; x < tmp.edgeLength; x++) {
            kernel_set_value(&tmp, x, y, 1.0);
        }
    }

    return tmp;
}

void kernel_destroy(Kernel *self) {
    self->radius = 0;
    self->edgeLength = 0;
    free(self->data);
}

double kernel_get_value(Kernel *self, int x, int y) {
    return self->data[y * self->edgeLength + x];
}

void kernel_set_value(Kernel *self, int x, int y, double value) {
    self->data[y * self->edgeLength + x] = value;
}

void kernel_normalize(Kernel *self) {
    double sum = kernel_sum(self);

    if (sum <= 0) {
        return;
    }

    for (int y = 0; y < self->edgeLength; y++) {
        for (int x = 0; x < self->edgeLength; x++) {
            kernel_set_value(self, x, y, kernel_get_value(self, x, y) / sum);
        }
    }
}

void kernel_scale(Kernel *self, double scale) {
    for (int y = 0; y < self->edgeLength; y++) {
        for (int x = 0; x < self->edgeLength; x++) {
            kernel_set_value(self, x, y, kernel_get_value(self, x, y) * scale);
        }
    }
}

double kernel_sum(Kernel *self) {
    double sum = 0;
    for (int y = 0; y < self->edgeLength; y++) {
        for (int x = 0; x < self->edgeLength; x++) {
            sum += kernel_get_value(self, x, y);
        }
    }
    return sum;
}
