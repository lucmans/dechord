
#include "peak.h"


double interpolate(Data f[], int xi, int n)
{
    double result = 0; // Initialize result
 
    for (int i=0; i<n; i++)
    {
        // Compute individual terms of above formula
        double term = f[i].y;
        for (int j=0;j<n;j++)
        {
            if (j!=i)
                term = term*(xi - f[j].x)/double(f[i].x - f[j].x);
        }
 
        // Add current term to result
        result += term;
    }
 
    return result;
} 