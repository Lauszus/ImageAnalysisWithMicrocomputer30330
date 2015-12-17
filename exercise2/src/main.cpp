#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "moments.h"

using namespace std;
using namespace cv;

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

static int map(int x, int in_min, int in_max, int out_min, int out_max) {
    int value = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min; // From Arduino source code: https://github.com/arduino/Arduino/blob/ide-1.5.x/hardware/arduino/avr/cores/arduino/WMath.cpp
    return constrain(value, out_min, out_max); // Limit output
}

static bool thresholdChanged;

void thresholdCallBack(int pos) {
    thresholdChanged  = true;
}

typedef struct histogram_t {
    static const int nSize = 256;
    int data[nSize][3]; // Data for three channels
} histogram_t;

static Mat drawHistogram(Mat *image, int thresholdValue) {
    histogram_t histogram;
    memset(histogram.data, 0, sizeof(histogram.data));

    for (int i = 0; i < image->total(); i++) {
        for (int j = 0; j < image->channels(); j++) {
            const size_t index = image->channels() * i + j;
            unsigned char value = image->data[index];
            histogram.data[value][j]++;
        }
    }

#if 0
    uint32_t total = 0;
    for (int i = 0; i < histogram.nSize; i++) {
        for (int j = 0; j < image->channels(); j++) {
            //printf("[%d][%d] = %d\n", i, j, histogram.data[i][j]);
            total += histogram.data[i][j];
        }
    }
    printf("%u == %lu\n", total, image->total() * image->channels()); // Should be equal to "image.total() * image.channels()"
    assert(total == image->total() * image->channels());
#endif

    Mat hist(Size(640/*image.size().width*/, image->size().height), CV_8UC3);
    rectangle(hist, Point(0, 0), hist.size(), Scalar(255, 255, 255, 0), CV_FILLED); // White background

    static const int offset = 50;
    const int startX = offset;
    const int endX = hist.size().width - offset;
    const int startY = offset;
    const int endY = hist.size().height - offset;

    line(hist, Point(startX, endY), Point(endX, endY), Scalar(0)); // x-axis
    line(hist, Point(startX, startY), Point(startX, endY), Scalar(0)); // y-axis

    int maxHist = 0;
    for (int i = 0; i < histogram.nSize; i++) {
        for (int j = 0; j < image->channels(); j++) {
            if (histogram.data[i][j] > maxHist)
                maxHist = histogram.data[i][j];
        }
    }

    for (int i = 0; i < histogram.nSize; i++) {
        int posX = map(i, 0, histogram.nSize, startX + 5, endX - 5);

        if (i == thresholdValue)
            line(hist, Point(posX, startY), Point(posX, endY), Scalar(0, 0, 255, 0)); // Draw red threshold line

        for (int j = 0; j < image->channels(); j++)
            line(hist, Point(posX, endY - map(histogram.data[i][j], 0, maxHist, 0, endY - offset)), Point(posX, endY),
                                            /* Draw in colors if color image, else draw black if greyscale */
                                            Scalar(j == 0 && image->channels() > 1 ? 255 : 0,
                                                     j == 1 ? 255 : 0,
                                                     j == 2 ? 255 : 0, 0));
    }
    //imshow("Histogram", hist);
    return hist;
}

int main(int argc, char *argv[]) {
    for (int i = 0; i < argc; i++)
        printf("argv[%d] = %s\n", i, argv[i]);

#if 0
    VideoCapture capture(0); // Capture video from webcam
    if (!capture.isOpened()) {
        printf("Could not open Webcam\n");
        return 1;
    }

restart:
    Mat image;
    if (!capture.read(image)) {
        printf("Could not read Webcam\n");
        return 1;
    }
    flip(image, image, 1); // Flip image so it acts like a mirror
    resize(image, image, image.size() / 2); // Resize image
    cvtColor(image, image, COLOR_BGR2GRAY);
    imshow("Image", image);
    imwrite("image.png", image);
#else

    const char *controlWindow = "Control";
    cvNamedWindow(controlWindow, CV_WINDOW_AUTOSIZE); // Create a window called "Control"
    int thresholdValue = 72; // 105
    cvCreateTrackbar("Threshold", controlWindow, &thresholdValue, 255, thresholdCallBack);
    moveWindow(controlWindow, 0, 500); // Move window

restart:
#if 0
    static Mat imageFull = imread("../files/PEN.pgm", IMREAD_GRAYSCALE);
    static Mat image = imageFull(Rect(2, 2, imageFull.size().width - 2 * 2, imageFull.size().height - 2 * 2)).clone(); // Crop two pixels from both sides
#else
    Mat image;
    static uint8_t imageN = 0;
    char buf[30], fileName[10];
    if (imageN == 0)
        strcpy(fileName, "image1");
    else if (imageN == 1)
        strcpy(fileName, "image2");
    else
        strcpy(fileName, "image3");
    sprintf(buf, "%s%s", fileName, ".png");
    image = imread(buf, IMREAD_GRAYSCALE);

    if (++imageN > 2)
        imageN = 0;
#endif

#if 1
    printf("\nThreshold %d\n", thresholdValue);
    printf("Image size: %lu, width: %d, height: %d, total: %lu, channels: %d\n",
            image.total() * image.channels(), image.size().width, image.size().height, image.total(), image.channels());
#endif

    Mat hist = drawHistogram(&image, thresholdValue);

    Mat imageThreshold = image.clone();
    for (int i = 0; i < imageThreshold.total() * imageThreshold.channels(); i++)
        imageThreshold.data[i] = imageThreshold.data[i] >= thresholdValue ? 255 : 0; // Draw threshold image

    moments_t moments = calculateMoments(&imageThreshold, false);
    image = drawMoments(&image, &moments, 5, 100);

    // Draw window
    Mat window(image.size().height, image.size().width + imageThreshold.size().width + hist.size().width, CV_8UC3);

    Mat left(window, Rect(0, 0, image.size().width, image.size().height));
    image.copyTo(left);

    Mat right(window, Rect(image.size().width, 0, imageThreshold.size().width, imageThreshold.size().height));
    cvtColor(imageThreshold, imageThreshold, COLOR_GRAY2BGR); // Convert image to BGR, so it actually shows up
    imageThreshold.copyTo(right);

    Mat farRight(window, Rect(image.size().width + imageThreshold.size().width, 0, hist.size().width, hist.size().height));
    line(hist, Point(0, 0), Point(0, hist.size().height), Scalar(0)); // Draw separator
    hist.copyTo(farRight);

    imshow("Window", window);
    Mat windowBorder;
    copyMakeBorder(window, windowBorder, 1, 1, 1, 1, BORDER_CONSTANT); // Add a border before writing
    sprintf(buf, "img/%s%s", fileName, "_window.png");
    imwrite(buf, windowBorder);
#endif

    char key = 0;
    thresholdChanged = false;
    while (!thresholdChanged) {
        if ((key = cvWaitKey(1)) > 0) // Wait until a key is pressed or threshold changes
            break;
    }
    if (key != 27) // ESC
        goto restart;

    return 0;
}
