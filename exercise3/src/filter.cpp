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

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "filter.h"
#include "histogram.h"
#include "misc.h"

using namespace cv;

// Multiply all kernel coefficients with a gain
LinearFilter operator * (const LinearFilter& filter, const float gain) {
    return LinearFilter(filter) *= gain;
}

LinearFilter operator * (const float gain, const LinearFilter& filter) {
    return filter * gain;
}

// Some handy linear filter kernels
const float LowpassFilter::lowpass[3 * 3] = {
    1, 1, 1,
    1, 1, 1,
    1, 1, 1,
};

const float HighpassFilter::highpass[3 * 3] = {
    -1, -1, -1,
    -1,  8, -1,
    -1, -1, -1,
};

const float LaplacianFilter::laplacian[3 * 3] = {
    0,  1,  0,
    1, -4,  1,
    0,  1,  0,
};

const float LaplacianTriangularFilter::laplacianTriangular[3 * 3] = {
    1,   0,  1,
    0,  -4,  0,
    1,   0,  1,
};

const float LaplaceGaussianFilter::laplaceGaussian[9 * 9] = {
    0,  0,  1,   2,    2,   2,  1,  0, 0,
    0,  1,  5,  10,   12,  10,  5,  1, 0,
    1,  5, 15,  19,   16,  19, 15,  5, 1,
    2, 10, 19, -19,  -64, -19, 19, 10, 2,
    2, 12, 16, -64, -148, -64, 16, 12, 2,
    2, 10, 19, -19,  -64, -19, 19, 10, 2,
    1,  5, 15,  19,   16,  19, 15,  5, 1,
    0,  1,  5,  10,   12,  10,  5,  1, 0,
    0,  0,  1,   2,    2,   2,  1,  0, 0,
};

Mat LinearFilter::apply(const Mat *q) {
    const Size size = q->size();
    const int width = size.width;
    const int height = size.height;
    const uint8_t channels = q->channels();
    const size_t nPixels = q->total() * channels;

    Mat p(size, q->type());

    size_t index = 0;
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            float value[3] = { 0, 0, 0 };
            for (int8_t k = -n; k <= n; k++) {
                // Make sure that we are within the height limit
                if ((int32_t)(y + k) < 0)
                    k = -y;
                else if (y + k >= height)
                    break;

                for (int8_t l = -m; l <= m; l++) {
                    // Make sure that we are within the width limit
                    if ((int32_t)(x + l) < 0)
                        l = -x;
                    else if (x + l >= width)
                        break;

                    int32_t subIndex = index + (k * width + l) * channels;
                    bool overflow = subIndex <  0 || subIndex >= nPixels;
                    if (!overflow) {
                        for (uint8_t i = 0; i < channels; i++)
                            value[i] += c[rows * (k + n) + (l + m)] * (float)q->data[subIndex + i];
                    } else
                        printf("\nsubIndex: %d\n", subIndex);
                    assert(!overflow); // Prevent overflow
                }
            }
            for (uint8_t i = 0; i < channels; i++)
                p.data[index + i] = constrain(value[i], 0, 255); // Constrain data into valid range
            index += channels; // Increment with number of channels. Same as: index = (x + y * width) * channels;
        }
    }

    return p;
}

LinearFilter LinearFilter::combineFilterKernels(const LinearFilter filter1, const LinearFilter filter2) {
    // TODO: Can this be fixed by just using max row or just pad smallest kernel?
    assert(filter1.size == filter2.size); // Has to be equal sizes for now

    const uint8_t size1 = filter1.size;
    const uint8_t size2 = filter2.size;
    const uint8_t rows1 = sqrtf(size1);
    const uint8_t rows2 = sqrtf(size2);
    const uint8_t outRow =  rows1 + rows2 - 1;
    const uint8_t outSize = outRow * outRow;

    float c[outSize];
    for (uint8_t y = 0; y < outRow; y++) {
        for (uint8_t x = 0; x < outRow; x++) {
            size_t index = x + y * outRow;
            float total = 0;

            const uint8_t startI = max(0, y - rows1 + 1);
            const uint8_t stopI = min((uint8_t)(y + 1), rows1);

            const uint8_t startJ = max(0, x - rows1 + 1);
            const uint8_t stopJ = min((uint8_t)(x + 1), rows1);

            for (uint8_t i = startI; i < stopI; i++) {
                for (uint8_t j = startJ; j < stopJ; j++) {
                    uint8_t subIndex1 = i * rows1 + j;
                    uint8_t subIndex2 = (size2 - 1) - x - y * rows2 + (j + i * rows2);
                    total += filter1.c[subIndex1] * filter2.c[subIndex2];
                    //printf("Index: %lu %u %u\n", index, subIndex1, subIndex2);
                }
            }
            //printf("\n");
            c[index] = total;
        }
    }

    return LinearFilter(c, outSize); // Return new filter
}

static void addRemoveToFromHistogram(histogram_t *histogram, const Mat *image, bool add) {
    const Size size = image->size();
    const int width = size.width;
    const int height = size.height;
    const uint8_t channels = image->channels();

    size_t index = 0;
    for (size_t y = 0; y < height; y++) {
        for (uint8_t i = 0; i < channels; i++) {
            if (add) {
                uchar val = image->data[(index + i) + ((width - 1) * channels)]; // Read from the right side of image
                histogram->data[val][i]++;
            } else {
                uchar val = image->data[index + i]; // Read from left side of image
                if (histogram->data[val][i] > 0)
                    histogram->data[val][i]--;
                else {
                    printf("Size = %dx%d y = %lu i = %u index = %lu\n", width, height, y, i, index);
                    assert(histogram->data[val][i]); // This should never happen
                }
            }
        }
        index += width * channels; // Go to next row
    }
}

Mat fractileFilter(const Mat *image, const uint8_t windowSize, const uint8_t percentile, bool skipBlackPixels) {
    const Size size = image->size();
    const int width = size.width;
    const int height = size.height;
    const uint8_t channels = image->channels();

    assert(!skipBlackPixels || (skipBlackPixels && channels == 1)); // If skipping black pixels, then the image must be in black and white

    Mat filteredImage(size, image->type());
    memset(filteredImage.data, 0, filteredImage.total());

    // TODO: Just read directly from image instead of copying data to new window

    size_t index = 0;
    histogram_t histogram;
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            // If the picture is only black and white and looking for white pixels, then it is a good idea to set 'skipBlackPixels' to true, as it will save a lot of time!
            if (skipBlackPixels && image->data[index] == 0) {
                index += channels;
                continue;
            }

            // TODO: Fix problem around border, as it does not go to end position
            int32_t windowX = x - windowSize / 2;
            int32_t windowY = y - windowSize / 2;

            uint8_t croppedX = 0, croppedY = 0;
            if (windowX < 0) {
                croppedX = -windowX;
                windowX = 0;
            } if (windowY < 0) {
                croppedY = -windowY;
                windowY = 0;
            }

            size_t spanX = x + windowSize / 2;
            size_t spanY = y + windowSize / 2;

            uint8_t windowWidth = spanX >= width ? windowSize - (spanX - width) - 1: windowSize;
            uint8_t windowHeight = spanY >= height ? windowSize - (spanY - height) - 1: windowSize;
            windowWidth -= croppedX;
            windowHeight -= croppedY;

            //printf("%lu\t%d,%d\t%lu,%lu\t%u,%u\n", index, windowX, windowY, spanX, spanY, windowWidth, windowHeight);

            // Don't create new window each turn, simply read image directly
            Mat window = Mat(*image, Rect(windowX, windowY, windowWidth, windowHeight)).clone();
            /*imshow("Fractile window", window);
            cvWaitKey(100);*/

            // TODO: Fix this hack
            //static uint32_t counter = 0;
            if (!skipBlackPixels && windowWidth == windowSize && windowHeight == windowSize) // We have to recalculate the histogram every time if we are skipping pixels
                addRemoveToFromHistogram(&histogram, &window, true); // Add next side to histogram
            else {
                histogram = getHistogram(&window); // Only use this routine the first time
                //printf("Counter: %d %u %u %u %u\n", ++counter, windowX, windowY, windowWidth, windowHeight);
            }

            // Now find median from histogram
            // TODO: Optimize this
            const uint32_t medianPos = windowWidth * windowHeight * percentile / 100;
            uint32_t total[3] = { 0, 0, 0 };
            int median[3] =  { -1, -1, -1 };
            for (uint16_t i = 0; i < histogram.nSize; i++) {
                for (uint8_t j = 0; j < channels; j++) {
                    //printf("[%d] = %d\n", i, histogram.data[i][j]);
                    if (median[j] == -1) { // Only run if median is not found yet
                        total[j] += histogram.data[i][j];
                        if (total[j] >= medianPos) {
                            median[j] = i; // Median found

                            bool done = true;
                            for (uint8_t k = 0; k < channels; k++) {
                                if (median[k] == -1)
                                    done = false;
                            }
                            if (done) // Break if all medians are found
                                break;
                        }
                    }
                }
            }

            for (uint8_t i = 0; i < channels; i++) {
                //printf("Median[%u]: %d at %u\n", i, median[i], medianPos);
                filteredImage.data[index + i] = median[i];
            }
            index += channels;

            if (!skipBlackPixels && windowWidth == windowSize && windowHeight == windowSize)
                addRemoveToFromHistogram(&histogram, &window, false); // Remove left side of window from histogram
        }
    }

    /*histogram_t histogram = getHistogram(&filteredImage);
    imshow("His", drawHistogram(&histogram, &filteredImage, Size(600, 400)));*/

    return filteredImage;
}

// TODO: Optimize this, as this is a very slow approach
Mat morphologicalFilter(const Mat *image, MorphologicalType type, const uint8_t structuringElementSize, bool whitePixels) {
    assert(image->channels() == 1); // Picture must be a greyscale image

    const Size size = image->size();
    const int width = size.width;
    const int height = size.height;
    const uint8_t n = (structuringElementSize - 1)/2;

    Mat filteredImage = image->clone(); // Create copy of original image
    size_t index = 0;
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            uint8_t minMax = image->data[index];
            if ((type == DILATION && whitePixels) || (type == EROSION && !whitePixels)) { // Max is used for dilation when looking for white pixels
                if (minMax == 255) // It is already the maximum possible value
                    goto breakout;
            } else { // Max is used for erosion when looking for white pixels
                if (minMax == 0) // It is already the minimum possible value
                    goto breakout;
            }
            for (int i = -n; i <= n; i++) { // Look around image
                for (int j = -n; j <= n; j++) {
                    size_t subIndex = index + i * width + j;
                    if (subIndex > 0 && subIndex < width * height && !(i == 0 && j == 0)) { // Prevent overflow and make sure that we are not looking at the center pixel again
                        if ((type == DILATION && whitePixels) || (type == EROSION && !whitePixels)) { // Max is used for dilation when looking for white pixels
                            if (image->data[subIndex] > minMax)
                                minMax = image->data[subIndex]; // Update maximum value
                            if (minMax == 255)
                                goto breakout; // Maximum possible value is found
                        } else { // Max is used for erosion when looking for white pixels
                            if (image->data[subIndex] < minMax)
                                minMax = image->data[subIndex]; // Update minimum value
                            if (minMax == 0)
                                goto breakout; // Minimum possible value is found
                        }
                    }
                }
            }
breakout:
            filteredImage.data[index] = minMax; // Set pixel to min/max value of its neighbors
            index++; // Increment index
        }
    }
    return filteredImage;
}
