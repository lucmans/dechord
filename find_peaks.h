
#ifndef FIND_PEAKS
#define FIND_PEAKS


#include "config.h"

#include <vector>


void calc_envelope(const double norms[(WINDOW_SAMPLES / 2) + 1], double envelope[(WINDOW_SAMPLES / 2) + 1]);
void find_peaks(const double norms[(WINDOW_SAMPLES / 2) + 1], const double envelope[(WINDOW_SAMPLES / 2) + 1], std::vector<int> &peaks);


#endif  // FIND_PEAKS
