#ifndef __histogram_h__
#define __histogram_h__

using namespace cv;

struct histogram_t {
    histogram_t(void) {
        memset(data, 0, sizeof(data)); // Make sure all elements are zero
    }
    static const uint16_t nSize = 256;
    uint32_t data[nSize][3]; // Data for three channels
};

void printHistogram(const histogram_t *histogram, uint8_t channels);
histogram_t getHistogram(const Mat *image);
Mat drawHistogram(const histogram_t *histogram, const Mat *image, const Size imageSize, int thresholdValue = -1);

#endif
