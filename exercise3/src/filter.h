/* Copyright (C) 2015 Kristian Sloth Lauszus. All rights reserved.

 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").

 Contact information
 -------------------

 Kristian Sloth Lauszus
 Web      :  http://www.lauszus.com
 e-mail   :  lauszus@gmail.com
*/

#ifndef __filter_h__
#define __filter_h__

using namespace cv;

Mat fractileFilter(const Mat *image, const uint8_t windowSize, const uint8_t percentile, bool skipBlackPixels);

class LinearFilter {
public:
    // Assumes that the kernel is symmetric
    LinearFilter(const float *coefficients, uint8_t _size) :
        n(-0.5f + 0.5f * sqrtf(_size)),
        m(n),
        rows(2 * n + 1),
        columns(2 * m + 1),
        size(_size),
        lastSum(0) {
        initCoefficients(coefficients, true);
    }

    // Set kernel using n and m
    LinearFilter(const float *coefficients, const uint8_t _n, const uint8_t _m) :
        n(_n),
        m(_m),
        rows(2 * n + 1),
        columns(2 * m + 1),
        size(rows * columns),
        lastSum(0) {
        initCoefficients(coefficients, true);
    };

    // Copy constructor
    LinearFilter(const LinearFilter& filter) {
        swap(filter);
    }

    ~LinearFilter() {
        delete[] c;
    }

    inline Mat operator () (const Mat *q) {
        return apply(q); // Apply filter
    }

    // Assignment operator
    LinearFilter& operator = (const LinearFilter& filter) {
       if (this != &filter)
           swap(filter);
       return *this;
   }

    // Used to combine two filter kernels into one
    const LinearFilter operator + (const LinearFilter& filter) {
        return *this += filter;
    }

    LinearFilter& operator += (const LinearFilter& filter) {
        if (lastSum != 0) { // Make sure that it has been normalized
            for (uint8_t i = 0; i < size; i++)
                c[i] *= lastSum; // Undo normalization
        }
        if (filter.lastSum != 0) { // Make sure that it has been normalized
            for (uint8_t i = 0; i < size; i++)
                c[i] *= filter.lastSum; // Undo normalization. We can just multiply the current filter with the other gain, as it has the same effect in the end
        }
        return *this = combineFilterKernels(*this, filter);
    }

    // Multiply all kernel coefficients with a gain
    LinearFilter& operator *= (const float gain) {
        for (uint8_t i = 0; i < size; i++)
            c[i] *= gain;
        if (gain != 0) { // Prevent division with zero
            if (lastSum == 0)
                lastSum = 1; // If it has not been set before, set it equal to the inverse of the gain
            lastSum /= gain; // Update last sum, so it can undo normalization correctly
        }
        return *this;
    }

    Mat apply(const Mat *q);

    void normalize(void) {
        float sum = 0;
        for (uint8_t i = 0; i < size; i++)
            sum += c[i];

        float sum_new = 0;
        if (sum < -0.0001f || sum > 0.0001f) { // Skip if value is very close to 0
            for (uint8_t i = 0; i < size; i++) {
                c[i] /= sum; // Normalize data
                sum_new += c[i];
            }
            //printf("Normalized: %f %.2f\n", sum, sum_new);
            assert(sum_new == 1.00f);
            lastSum = sum;
        }
    }

    void printKernel(void) {
        for (uint8_t i = 0; i < rows; i++) {
            for (uint8_t j = 0; j < columns; j++)
                printf("%.2f\t", c[i * rows + j]);
            printf("\n");
        }
    }

    uint8_t n, m;
    uint8_t rows, columns;
    uint8_t size;
    float *c;

private:
    void swap(const LinearFilter& filter) {
        n = filter.n;
        m = filter.m;
        rows = filter.rows;
        columns = filter.columns;
        size = filter.size;
        initCoefficients(filter.c, false); // Don't normalize, just copy data
    }

    void initCoefficients(const float *coefficients, bool _normalize) {
        //printf("n = %u m = %u rows = %u columns = %u size = %u normalize  = %u\n", n, m, rows, columns, size, _normalize);

        c = new float[size];

        float sum = 0;
        for (uint8_t i = 0; i < size; i++) {
            c[i] = coefficients[i];
            sum += c[i];
        }
        if (_normalize && sum != 0 && sum != 1)
            normalize();
    }

    LinearFilter combineFilterKernels(const LinearFilter filter1, const LinearFilter filter2);

    float lastSum; // Used to undo normalization
};

// Multiply filter all kernel coefficients with a gain
const LinearFilter operator * (const LinearFilter& filter, const float gain);
const LinearFilter operator * (const float gain, const LinearFilter& filter);

class LowpassFilter : public LinearFilter {
public:
    LowpassFilter() :
        LinearFilter(lowpass, sizeof(lowpass)/sizeof(lowpass[0])) {
    }

    static const float lowpass[3 * 3];
};

class HighpassFilter : public LinearFilter {
public:
    HighpassFilter() :
        LinearFilter(highpass, sizeof(highpass)/sizeof(highpass[0])) {
    }

    static const float highpass[3 * 3];
};

class LaplacianFilter : public LinearFilter {
public:
    LaplacianFilter() :
        LinearFilter(laplacian, sizeof(laplacian)/sizeof(laplacian[0])) {
    }

    static const float laplacian[3 * 3];
};

class LaplacianTriangularFilter : public LinearFilter {
public:
    LaplacianTriangularFilter() :
        LinearFilter(laplacianTriangular, sizeof(laplacianTriangular)/sizeof(laplacianTriangular[0])) {
    }

    static const float laplacianTriangular[3 * 3];
};

class LaplaceGaussianFilter : public LinearFilter {
public:
    LaplaceGaussianFilter() :
        LinearFilter(laplaceGaussian, sizeof(laplaceGaussian)/sizeof(laplaceGaussian[0])) {
    }

    static const float laplaceGaussian[9 * 9];
};

#endif
