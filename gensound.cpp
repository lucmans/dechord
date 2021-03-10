
#include "gensound.h"
#include "config.h"

#include <cmath>


void write_sine(float freq, float samples[], int start, int end) {
    for(int i = 0; start + i < end; i++) {
        samples[start + i] = sinf((2 * M_PI * i * freq) / SAMPLE_RATE);
    }
}

void write_sine(double freq, double samples[], int start, int end) {
    for(int i = 0; start + i < end; i++) {
        samples[start + i] = sin((2 * M_PI * i * freq) / SAMPLE_RATE);
    }
}
