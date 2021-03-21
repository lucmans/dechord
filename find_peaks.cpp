
#include "find_peaks.h"
#include "config.h"

#include <vector>
#include <algorithm>


const int KERNEL_WIDTH = 45;  // Choose odd value
const int MID = KERNEL_WIDTH / 2;
const double SIGMA = 1.2;  // Higher values of sigma make values close to kernel center weight more


void calc_gaussian(double gaussian[KERNEL_WIDTH]) {
    for(int i = 0; i < KERNEL_WIDTH; i++)
        gaussian[i] = exp(-M_PI * ((double)(i - MID) / ((double)MID * SIGMA)) * ((double)(i - MID) / ((double)MID * SIGMA)));
}

void calc_envelope(const double norms[(WINDOW_SAMPLES / 2) + 1], double envelope[(WINDOW_SAMPLES / 2) + 1]) {
    double gaussian[KERNEL_WIDTH];
    calc_gaussian(gaussian);  // TODO: Only calculate once

    for(int i = 0; i < (WINDOW_SAMPLES / 2) + 1; i++) {
        double sum = 0, weights = 0;
        for(int j = std::max(-MID, -i); j <= std::min(MID, (WINDOW_SAMPLES / 2) - i); j++) {
            sum += norms[i + j] * gaussian[j + MID];
            weights += gaussian[j + MID];
        }
        envelope[i] = sum / weights;
    }
}


void all_max(const double norms[(WINDOW_SAMPLES / 2) + 1], std::vector<int> &peaks) {
    for(int i = 1; i < (WINDOW_SAMPLES / 2); i++) {
        if(norms[i - 1] < norms[i] && norms[i] > norms[i + 1])
            peaks.push_back(i);
    }

    // Filter quiet peaks
    for(size_t i = peaks.size(); i > 0; i--) {
        if(norms[peaks[i - 1]] < 3)
            peaks.erase(peaks.begin() + (i - 1));
    }
}

void envelope_peaks(const double norms[(WINDOW_SAMPLES / 2) + 1], const double envelope[(WINDOW_SAMPLES / 2) + 1], std::vector<int> &peaks) {
    for(int i = 1; i < (WINDOW_SAMPLES / 2); i++) {
        if(norms[i - 1] < norms[i] && norms[i] > norms[i + 1] && norms[i] > envelope[i])
            peaks.push_back(i);
    }

    // Filter quiet peaks
    for(size_t i = peaks.size(); i > 0; i--) {
        if(envelope[peaks[i - 1]] < 0.1)
            peaks.erase(peaks.begin() + (i - 1));
    }
}

void envelope_highest_peak(const double norms[(WINDOW_SAMPLES / 2) + 1], const double envelope[(WINDOW_SAMPLES / 2) + 1], std::vector<int> &peaks) {
    bool envelope_entered = false;
    bool envelope_left = true;

    for(int i = 0; i < (WINDOW_SAMPLES / 2) + 1; i++) {
        if(norms[i] > envelope[i] && envelope_left) {
            envelope_entered = true;
            envelope_left = false;
        }

        if(norms[i] < envelope[i] && envelope_entered) {
            envelope_entered = false;
            envelope_left = true;
        }

        if(norms[i - 1] < norms[i] && norms[i] > norms[i + 1] && envelope_entered) {
            peaks.push_back(i);
        }
    }
}

void find_peaks(const double norms[(WINDOW_SAMPLES / 2) + 1], const double envelope[(WINDOW_SAMPLES / 2) + 1], std::vector<int> &peaks) {
    // all_max(norms, peaks);
    envelope_peaks(norms, envelope, peaks);
    // envelope_highest_peak(norms, envelope, peaks);
}
