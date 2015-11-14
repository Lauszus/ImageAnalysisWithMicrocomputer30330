#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include "histogram.h"
#include "misc.h"

using namespace std;
using namespace cv;

static int map(int x, int in_min, int in_max, int out_min, int out_max) {
    int value = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min; // From Arduino source code: https://github.com/arduino/Arduino/blob/ide-1.5.x/hardware/arduino/avr/cores/arduino/WMath.cpp
    return constrain(value, out_min, out_max); // Limit output
}

void printHistogram(const histogram_t *histogram, uint8_t channels) {
    for (uint16_t i = 0; i < histogram->nSize; i++) {
        for (uint8_t j = 0; j < channels; j++) {
            if (histogram->data[i][j])
                printf("[%u][%u] %u\n", i, j, histogram->data[i][j]);
        }
    }
}

histogram_t getHistogram(const Mat *image) {
    histogram_t histogram;
    const uint8_t channels = image->channels();

    size_t index = 0;
    for (size_t i = 0; i < image->total(); i++) {
        for (uint8_t j = 0; j < channels; j++) {
            uchar value = image->data[index + j];
            histogram.data[value][j]++;
        }
        index += channels;
    }

#if 0
    uint32_t total = 0;
    for (uint16_t i = 0; i < histogram.nSize; i++) {
        for (uint8_t j = 0; j < channels; j++) {
            //printf("[%u][%u] = %d\n", i, j, histogram.data[i][j]);
            total += histogram.data[i][j];
        }
    }
    printf("%u == %lu\n", total, image->total() * channels); // Should be equal to "image.total() * image.channels()"
    assert(total == image->total() * channels);
#endif

    return histogram;
}

Mat drawHistogram(const histogram_t *histogram, const Mat *image, const Size imageSize, int thresholdValue /*= -1*/) {
    Mat hist(imageSize, CV_8UC3);
    rectangle(hist, Point(0, 0), hist.size(), Scalar(255, 255, 255, 0), CV_FILLED); // White background

    static const uint8_t offset = 50;
    static const uint8_t startX = offset;
    static const uint8_t startY = offset;
    const int endX = hist.size().width - offset;
    const int endY = hist.size().height - offset;

    line(hist, Point(startX, endY), Point(endX, endY), Scalar(0)); // x-axis
    line(hist, Point(startX, startY), Point(startX, endY), Scalar(0)); // y-axis

    int maxHist = 0;
    for (uint16_t i = 0; i < histogram->nSize; i++) {
        for (uint8_t j = 0; j < image->channels(); j++) {
            if (histogram->data[i][j] > maxHist)
                maxHist = histogram->data[i][j];
        }
    }

    for (uint16_t i = 0; i < histogram->nSize; i++) {
        int posX = map(i, 0, histogram->nSize, startX + 5, endX - 5);

        if (i == thresholdValue)
            line(hist, Point(posX, startY), Point(posX, endY), Scalar(0, 0, 255, 0)); // Draw red threshold line

        for (uint8_t j = 0; j < image->channels(); j++)
            line(hist, Point(posX, endY - map(histogram->data[i][j], 0, maxHist, 0, endY - offset)), Point(posX, endY),
                                            /* Draw in colors if color image, else draw black if greyscale */
                                            Scalar(j == 0 && image->channels() > 1 ? 255 : 0,
                                                     j == 1 ? 255 : 0,
                                                     j == 2 ? 255 : 0, 0));
    }
    //imshow("Histogram", hist);
    return hist;
}
