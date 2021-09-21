
#include "find_peaks.h"
#include "config.h"

#include <vector>
#include <algorithm>


const int MID = KERNEL_WIDTH / 2;
double gaussian[KERNEL_WIDTH];


int calc_gaussian(double gaussian[KERNEL_WIDTH]) {
    for(int i = 0; i < KERNEL_WIDTH; i++)
        gaussian[i] = exp(-M_PI * ((double)(i - MID) / ((double)MID * SIGMA)) * ((double)(i - MID) / ((double)MID * SIGMA)));

    return 1;  // For static assignment (calculating only once)
}

void calc_envelope(const double norms[(FRAME_SIZE / 2) + 1], double envelope[(FRAME_SIZE / 2) + 1]) {
    // Static to only Gaussian calculate once
    static int init = calc_gaussian(gaussian);

    for(int i = 0; i < (FRAME_SIZE / 2) + 1; i++) {
        double sum = 0, weights = 0;
        for(int j = std::max(-MID, -i); j <= std::min(MID, (FRAME_SIZE / 2) - i); j++) {
            sum += norms[i + j] * gaussian[j + MID];
            weights += gaussian[j + MID];
        }
        envelope[i] = sum / weights;
    }

    // Prevent warning
    return;
    init++;
}


void all_max(const double norms[(FRAME_SIZE / 2) + 1], std::vector<int> &peaks) {
    for(int i = 1; i < (FRAME_SIZE / 2); i++) {
        if(norms[i - 1] < norms[i] && norms[i] > norms[i + 1])
            peaks.push_back(i);
    }

    // Filter quiet peaks
    for(size_t i = peaks.size(); i > 0; i--) {
        if(norms[peaks[i - 1]] < 3)
            peaks.erase(peaks.begin() + (i - 1));
    }
}

void envelope_peaks(const double norms[(FRAME_SIZE / 2) + 1], const double envelope[(FRAME_SIZE / 2) + 1], std::vector<int> &peaks) {
    for(int i = 5; i < (FRAME_SIZE / 2); i++) {
        if(norms[i - 1] < norms[i] && norms[i] > norms[i + 1]  // A local maximum
           && norms[i] > envelope[i]  // Higher than envelope
           && envelope[i] > 0.1)  // Filter quiet peaks
            peaks.push_back(i);
    }
}

void envelope_highest_peak(const double norms[(FRAME_SIZE / 2) + 1], const double envelope[(FRAME_SIZE / 2) + 1], std::vector<int> &peaks) {
    // int highest = -1;

    // for(int i = 1; i < (FRAME_SIZE / 2); i++) {
    //     // Peak in envelope
    //     if(norms[i - 1] < norms[i] && norms[i] > norms[i + 1]
    //        && norms[i] > envelope[i]) {
    //         if(highest == -1)
    //             highest = i;
    //         else if(norms[highest] < norms[i])
    //             highest = i;
    //     }

    //     // If going under envelope again
    //     else if(norms[i] <= envelope[i]) {
    //         peaks.push_back(highest);
    //         highest = -1;
    //     }
    // }

    bool envelope_entered = false;
    bool envelope_left = true;

    for(int i = 0; i < (FRAME_SIZE / 2) + 1; i++) {
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

void find_peaks(const double norms[(FRAME_SIZE / 2) + 1], const double envelope[(FRAME_SIZE / 2) + 1], std::vector<int> &peaks) {
    // all_max(norms, peaks);
    envelope_peaks(norms, envelope, peaks);
    // envelope_highest_peak(norms, envelope, peaks);
}
