#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "filter.h"
#include "histogram.h"

using namespace std;
using namespace cv;

#define WEBCAM 0
#define PRINT_SPEED 0

static bool windowSizeChanged;

void thresholdCallBack(int pos) {
    windowSizeChanged  = true;
}

int main(int argc, char *argv[]) {
    for (int i = 0; i < argc; i++)
        printf("argv[%d] = %s\n", i, argv[i]);

    const char *controlWindow = "Control";
    cvNamedWindow(controlWindow, CV_WINDOW_AUTOSIZE); // Create a window called "Control"
    int windowSize = 3;
    cvCreateTrackbar("Window size", controlWindow, &windowSize, 10, thresholdCallBack);
    int percentile = 40;
    cvCreateTrackbar("Percentile", controlWindow, &percentile, 100, thresholdCallBack);
    //moveWindow(controlWindow, 0, 500); // Move window

#if WEBCAM
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
    cvtColor(image, image, COLOR_BGR2GRAY); // Convert to greyscale
#else
restart:
    printf("WindowSize %d\tPercentile: %d\n", windowSize, percentile);

    // Fractile filter demo
    static Mat imageNoise = imread("Medianfilterp_left.png", IMREAD_COLOR);
    Mat fractileFiltedImage = fractileFilter(&imageNoise, windowSize, percentile, false);

    Mat fractileWindow(fractileFiltedImage.size().height, 3 * imageNoise.size().width, CV_8UC3);
    Mat left(fractileWindow, Rect(0, 0, imageNoise.size().width, imageNoise.size().height));
    Mat center(fractileWindow, Rect(imageNoise.size().width, 0, imageNoise.size().width, imageNoise.size().height));
    Mat right(fractileWindow, Rect(2 * imageNoise.size().width, 0, imageNoise.size().width, imageNoise.size().height));

    imageNoise.copyTo(left);
    fractileFiltedImage.copyTo(center);
    static LowpassFilter lowpassFilter;
    lowpassFilter(&fractileFiltedImage).copyTo(right); // Lowpass filter fractile filter result

    imshow("Fractile filter", fractileWindow);

    // Show some of the other filters
    static Mat image = imread("../files/shuttle_640x480.jpg", IMREAD_COLOR);
    //static Mat imageFull = imread("../files/PEN.pgm", IMREAD_GRAYSCALE);
    //static Mat image = imageFull(Rect(2, 2, imageFull.size().width - 2 * 2, imageFull.size().height - 2 * 2)).clone(); // Crop two pixels from both sides
    //imshow("Image", image);
#endif
#if 0
    printf("Image size: %lu, width: %d, height: %d, total: %lu, channels: %d\n",
            image.total() * image.channels(), image.size().width, image.size().height, image.total(), image.channels());
#endif

    LinearFilter *filter;
    static int8_t imageN = 0;
    if (imageN < 0)
        imageN = 3;
    else if (imageN > 3)
        imageN = 0;
    if (imageN == 0) {
        printf("HighpassFilter\n");
        static HighpassFilter highpassFilter;
        filter = &highpassFilter;
    } else if (imageN == 1) {
        printf("LaplacianFilter\n");
        static LaplacianFilter laplacianFilter;
        filter = &laplacianFilter;
    } else if (imageN == 2) {
        printf("LaplacianTriangularFilter\n");
        static LaplacianTriangularFilter laplacianTriangularFilter;
        filter = &laplacianTriangularFilter;
    } else {
        printf("LaplaceGaussianFilter\n");
        static LaplaceGaussianFilter laplaceGaussianFilter;
        filter = &laplaceGaussianFilter;
    }

#if PRINT_SPEED
    int64_t timer = getTickCount();
#endif
    Mat filteredImage = filter->apply(&image);
#if PRINT_SPEED
    uint32_t count1 = getTickCount() - timer;
#endif

    Mat filter2DImage;
#if PRINT_SPEED
    timer = getTickCount();
#endif
    filter2D(image, filter2DImage, -1 /* Same depth as the source */, Mat(filter->rows, filter->columns, CV_32F, filter->c));

#if PRINT_SPEED
    uint32_t count2 = getTickCount() - timer;
    printf("Count: %u VS %u\t%.2f\n", count1, count2, (float)count2 / (float)count1);
#endif

    static histogram_t histogram = getHistogram(&image);
    static Mat histImage = drawHistogram(&histogram, &image, image.size());

    // Draw window
    Mat imageColor;
    if (image.channels() == 1)
        cvtColor(image, imageColor, COLOR_GRAY2BGR);
    else
        imageColor = image.clone(); // Convert image into 3-channel image, so we can draw in color
    Mat window(imageColor.size().height, imageColor.size().width + filteredImage.size().width + filter2DImage.size().width, CV_8UC3);

    left = Mat(window, Rect(0, 0, imageColor.size().width, imageColor.size().height));
    imageColor.copyTo(left);

    center = Mat(window, Rect(imageColor.size().width, 0, filteredImage.size().width, filteredImage.size().height));
    if (filteredImage.channels() == 1)
        cvtColor(filteredImage, filteredImage, COLOR_GRAY2BGR); // Convert image to BGR, so it actually shows up
    line(filteredImage, Point(0, 0), Point(0, filteredImage.size().height), Scalar(255, 255, 255)); // Draw separator
    filteredImage.copyTo(center);

    right = Mat(window, Rect(imageColor.size().width + filteredImage.size().width, 0, filter2DImage.size().width, filter2DImage.size().height));
    if (filter2DImage.channels() == 1)
        cvtColor(filter2DImage, filter2DImage, COLOR_GRAY2BGR); // Convert image to BGR, so it actually shows up
    line(filter2DImage, Point(0, 0), Point(0, filter2DImage.size().height), Scalar(255, 255, 255)); // Draw separator
    filter2DImage.copyTo(right);

    imshow("Window", window);
    imshow("Histogram", histImage);
    //moveWindow("Histogram", 50, imageColor.size().height * 1.5);

    windowSizeChanged = false;
    char key = 0;
    while (!windowSizeChanged) {
        if ((key = cvWaitKey(1)) > 0) // Wait until a key is pressed or threshold changes
            break;
    }
    if (key != 27) { // ESC
        if (key == 2) // Left
            imageN--;
        else if (key == 3) // Right
            imageN++;
        goto restart;
    }

    return 0;
}
