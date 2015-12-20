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

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "filter.h"
#include "histogram.h"
#include "contours.h"
#include "misc.h"

using namespace std;
using namespace cv;

static bool valueChanged;

void valueChangedCallBack(int pos) {
    valueChanged  = true;
}

int main(int argc, char *argv[]) {
    const char *controlWindow = "Control";
    cvNamedWindow(controlWindow, CV_WINDOW_AUTOSIZE); // Create a window called "Control"

    int thresholdValue = 105;
    int closingSize = 3;
    int openingSize = 3;

    cvCreateTrackbar("Threshold", controlWindow, &thresholdValue, 255, valueChangedCallBack);
    cvCreateTrackbar("Closing size", controlWindow, &closingSize, 10, valueChangedCallBack);
    cvCreateTrackbar("Opening size", controlWindow, &openingSize, 10, valueChangedCallBack);

    moveWindow(controlWindow, 0, 500); // Move window

#if 1
    static Mat imageFull = imread("../files/PEN.pgm", IMREAD_GRAYSCALE);
    static Mat image = imageFull(Rect(2, 2, imageFull.size().width - 2 * 2, imageFull.size().height - 2 * 2)).clone(); // Crop two pixels from both sides
#else
    static Mat image = imread("../files/ariane5_1b.jpg", IMREAD_GRAYSCALE);
    imwrite("img/ariane5_1b_grey.jpg", image);
    thresholdValue = 190;
#endif
    imshow("Image", image);
    imwrite("img/image.png", image);

restart:
    printf("Threshold: %d\tSize: %d,%d\n", thresholdValue, closingSize, openingSize);

    histogram_t histogram = getHistogram(&image);
    Mat hist = drawHistogram(&histogram, &image, Size(image.size().width * 2, image.size().height), thresholdValue);
    imshow("Hist", hist);

    Mat imageThreshold = image.clone();
    for (int i = 0; i < imageThreshold.total() * imageThreshold.channels(); i++)
        imageThreshold.data[i] = imageThreshold.data[i] >= thresholdValue ? 255 : 0; // Draw threshold image
    imshow("Threshold", imageThreshold);
    Mat imageThresholdBorder;
    copyMakeBorder(imageThreshold, imageThresholdBorder, 1, 1, 1, 1, BORDER_CONSTANT); // Add a border before writing
    imwrite("img/imageThreshold.png", imageThresholdBorder);

    // Apply morphological closing and opening
    Mat morphologicalFilterImg = imageThreshold.clone();

    // Morphological closing (Remove small bright spots (i.e. "salt") and connect small dark cracks)
    morphologicalFilterImg = morphologicalFilter(&morphologicalFilterImg, DILATION, closingSize, false);
    morphologicalFilterImg = morphologicalFilter(&morphologicalFilterImg, EROSION, closingSize, false);

    // Morphological opening (Remove small dark spots (i.e. "pepper") and connect small bright cracks)
    morphologicalFilterImg = morphologicalFilter(&morphologicalFilterImg, EROSION, openingSize, false);
    morphologicalFilterImg = morphologicalFilter(&morphologicalFilterImg, DILATION, openingSize, false);

    imshow("Morphological filter", morphologicalFilterImg);
    Mat morphologicalFilterImgBorder;
    copyMakeBorder(morphologicalFilterImg, morphologicalFilterImgBorder, 1, 1, 1, 1, BORDER_CONSTANT); // Add a border before writing
    imwrite("img/morphologicalFilter.png", morphologicalFilterImgBorder);

    Mat contour;
    if (contoursSearch(&morphologicalFilterImg, &contour, CONNECTED_8, false)) {
        imshow("Contour", contour);
        imwrite("img/contour.png", contour);
    }

    char key = 0;
    valueChanged = false;
    while (!valueChanged) {
        if ((key = cvWaitKey(1)) > 0) // Wait until a key is pressed or threshold changes
            break;
    }
    if (key != 27) // ESC
        goto restart;

    return 0;
}
