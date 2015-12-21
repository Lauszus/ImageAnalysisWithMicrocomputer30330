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
    int percentile = 40;
    int closingSize = 20;
    int openingSize = 10;

    cvCreateTrackbar("Window size", controlWindow, &windowSize, 10, thresholdCallBack);
    cvCreateTrackbar("Percentile", controlWindow, &percentile, 100, thresholdCallBack);
    cvCreateTrackbar("Closing size", controlWindow, &closingSize, 30, thresholdCallBack);
    cvCreateTrackbar("Opening size", controlWindow, &openingSize, 20, thresholdCallBack);
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
    printf("WindowSize %d\tPercentile: %d\tClosing and opening size: %d %d\n", windowSize, percentile, closingSize, openingSize);

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
    Mat lowpassImage = lowpassFilter(&fractileFiltedImage);
    lowpassImage.copyTo(right); // Lowpass filter fractile filter result

    imshow("Fractile filter", fractileWindow);
    imwrite("img/imageNoise.png", imageNoise);
    imwrite("img/fractileFiltedImage.png", fractileFiltedImage);
    imwrite("img/lowpassImage.png", lowpassImage);

    // Morphological filter demo
    Mat morphologicalImg = imread("morphological_example.png", IMREAD_GRAYSCALE);

    Mat morphologicalImgOutput[6]; // Array for output images

    // Morphological closing (Remove small bright spots (i.e. "salt") and connect small dark cracks)
    morphologicalImgOutput[0] = morphologicalFilter(&morphologicalImg, DILATION, closingSize, false);
    morphologicalImgOutput[1] = morphologicalFilter(&morphologicalImgOutput[0], EROSION, closingSize, false);

    // Morphological opening (Remove small dark spots (i.e. "pepper") and connect small bright cracks)
    morphologicalImgOutput[2] = morphologicalFilter(&morphologicalImg, EROSION, openingSize, false);
    morphologicalImgOutput[3] = morphologicalFilter(&morphologicalImgOutput[2], DILATION, openingSize, false);

    // Morphological opening on already closed image
    morphologicalImgOutput[4] = morphologicalFilter(&morphologicalImgOutput[1], EROSION, openingSize, false);
    morphologicalImgOutput[5] = morphologicalFilter(&morphologicalImgOutput[4], DILATION, openingSize, false);

    copyMakeBorder(morphologicalImg, morphologicalImg, 1, 1, 1, 1, BORDER_CONSTANT); // Add a border before writing
    for (uint8_t i = 0; i < 6; i++)
        copyMakeBorder(morphologicalImgOutput[i], morphologicalImgOutput[i], 1, 1, 1, 1, BORDER_CONSTANT); // Add a border before writing

    imwrite("img/morphologicalImg.png", morphologicalImg);
    imwrite("img/morphologicalImgDialate1.png", morphologicalImgOutput[0]);
    imwrite("img/morphologicalImgErode1.png", morphologicalImgOutput[1]);
    imwrite("img/morphologicalImgErode2.png", morphologicalImgOutput[2]);
    imwrite("img/morphologicalImgDialate2.png", morphologicalImgOutput[3]);
    imwrite("img/morphologicalImgErode3.png", morphologicalImgOutput[4]);
    imwrite("img/morphologicalImgDialate3.png", morphologicalImgOutput[5]);

    resize(morphologicalImg, morphologicalImg, morphologicalImg.size() / 3); // Resize image
    Mat window(morphologicalImg.size().height, 7 * morphologicalImg.size().width, morphologicalImg.type());
    morphologicalImg.copyTo(Mat(window,  Rect(0, 0, morphologicalImg.size().width, morphologicalImg.size().height)));

    for (uint8_t i = 0; i < 6; i++) {
        resize(morphologicalImgOutput[i], morphologicalImgOutput[i], morphologicalImgOutput[i].size() / 3); // Resize image
        morphologicalImgOutput[i].copyTo(Mat(window, Rect((i + 1) * morphologicalImg.size().width, 0, morphologicalImg.size().width, morphologicalImg.size().height)));
    }

    imshow("Morphological filter", window);

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
    char filterName[50];
    static int8_t imageN = 0;
    if (imageN < 0)
        imageN = 5;
    else if (imageN > 5)
        imageN = 0;
    if (imageN == 0) {
        strcpy(filterName, "LowpassFilter");
        static LowpassFilter lowpassFilter;
        filter = &lowpassFilter;
    } else if (imageN == 1) {
        strcpy(filterName, "HighpassFilter");
        static HighpassFilter highpassFilter;
        filter = &highpassFilter;
    } else if (imageN == 2) {
        strcpy(filterName, "LaplacianFilter");
        static LaplacianFilter laplacianFilter;
        filter = &laplacianFilter;
    } else if (imageN == 3) {
        strcpy(filterName, "LaplacianLowpassFilter");
        static LinearFilter linearFilter = LaplacianFilter() + LowpassFilter();
        filter = &linearFilter;
    } else if (imageN == 4) {
        strcpy(filterName, "LaplacianTriangularFilter");
        static LaplacianTriangularFilter laplacianTriangularFilter;
        filter = &laplacianTriangularFilter;
    } else {
        strcpy(filterName, "LaplaceGaussianFilter");
        static LaplaceGaussianFilter laplaceGaussianFilter;
        filter = &laplaceGaussianFilter;
    }
    printf("%s\n", filterName);

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
        cvtColor(image, imageColor, COLOR_GRAY2BGR); // Convert image into 3-channel image, so we can draw in color
    else
        imageColor = image.clone(); // Just copy image
    window = Mat(imageColor.size().height, imageColor.size().width + filteredImage.size().width + filter2DImage.size().width, CV_8UC3);

    left = Mat(window, Rect(0, 0, imageColor.size().width, imageColor.size().height));
    imageColor.copyTo(left);
    imwrite("img/imageColor.png", imageColor);

    center = Mat(window, Rect(imageColor.size().width, 0, filteredImage.size().width, filteredImage.size().height));
    if (filteredImage.channels() == 1)
        cvtColor(filteredImage, filteredImage, COLOR_GRAY2BGR); // Convert image to BGR, so it actually shows up
    line(filteredImage, Point(0, 0), Point(0, filteredImage.size().height), Scalar(255, 255, 255)); // Draw separator
    filteredImage.copyTo(center);
    char buf[100];
    sprintf(buf, "img/%s.png", filterName);
    imwrite(buf, filteredImage);

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
