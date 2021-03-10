
#ifndef PEAK_H
#define PEAK_H


// To represent a data point corresponding to x and y = f(x)
struct Data
{
    int x, y;
};

// function to interpolate the given data points using Lagrange's formula
// xi corresponds to the new data point whose value is to be obtained
// n represents the number of known data points
double interpolate(Data f[], int xi, int n);


#endif  // PEAK_H
