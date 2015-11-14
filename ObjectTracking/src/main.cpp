#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include "contours.h"
#include "euler.h"
#include "filter.h"
#include "histogram.h"
#include "moments.h"
#include "segmentation.h"

using namespace std;
using namespace cv;

static bool valueChanged;

void valueChangedCallBack(int pos) {
    valueChanged  = true;
}

int main(int argc, char *argv[]) {
    /*for (int i = 0; i < argc; i++)
        printf("argv[%d] = %s\n", i, argv[i]);*/
    static const bool DEBUG = argc == 2 ? argv[1][0] == '1' : false;

    // Orange: HSV = 5,15; LowS = 160; LowV = 55
    // Yellow: 25,35
    // Green: 60,100
    // Blue: 90,130
    // Red: 170,179
#if 1 // Green rubiks
    int iLowH = 60;
    int iHighH = 100;

    int iLowS = 100;
    int iHighS = 255;

    int iLowV = 10;
    int iHighV = 255;

    int size1 = 3;
    int size2 = 30;

    int windowSize = 3;
    int percentile = 20;

    int neighborSize = 25;

    int objectMin = 1600;
    int objectMax = 1750;
#elif 0 // Red folder
    int iLowH = 170;
    int iHighH = 10;

    int iLowS = 130;
    int iHighS = 255;

    int iLowV = 135;
    int iHighV = 255;

    int size1 = 3;
    int size2 = 30;

    int windowSize = 3;
    int percentile = 20;

    int neighborSize = 25;

    int objectMin = 1600;
    int objectMax = 2000;
#else // Blue tape
    int iLowH = 90;
    int iHighH = 130;

    int iLowS = 55;
    int iHighS = 255;

    int iLowV = 30;
    int iHighV = 255;

    int size1 = 1;
    int size2 = 12;

    int windowSize = 3;
    int percentile = 20;

    int neighborSize = 25;

    int objectMin = 1600;
    int objectMax = 1950;
#endif

    if (DEBUG) {
        const char *controlWindow = "Control";
        cvNamedWindow(controlWindow, CV_WINDOW_AUTOSIZE); // Create a window called "Control"

        // Create trackbars in "Control" window
        cvCreateTrackbar("LowH", controlWindow, &iLowH, 179, valueChangedCallBack); // Hue (0 - 179)
        cvCreateTrackbar("HighH", controlWindow, &iHighH, 179, valueChangedCallBack);

        cvCreateTrackbar("LowS", controlWindow, &iLowS, 255, valueChangedCallBack); // Saturation (0 - 255)
        cvCreateTrackbar("HighS", controlWindow, &iHighS, 255, valueChangedCallBack);

        cvCreateTrackbar("LowV", controlWindow, &iLowV, 255, valueChangedCallBack); // Value (0 - 255)
        cvCreateTrackbar("HighV", controlWindow, &iHighV, 255, valueChangedCallBack);

        cvCreateTrackbar("Size1", controlWindow, &size1, 50, valueChangedCallBack);
        cvCreateTrackbar("Size2", controlWindow, &size2, 50, valueChangedCallBack);

        cvCreateTrackbar("Window size", controlWindow, &windowSize, 10, valueChangedCallBack);
        cvCreateTrackbar("Percentile", controlWindow, &percentile, 100, valueChangedCallBack);

        cvCreateTrackbar("Neighbor size", controlWindow, &neighborSize, 50, valueChangedCallBack);

        cvCreateTrackbar("Object min", controlWindow, &objectMin, 2000, valueChangedCallBack);
        cvCreateTrackbar("Object max", controlWindow, &objectMax, 2000, valueChangedCallBack);
    }

    VideoCapture capture(0); // Capture video from webcam
    if (!capture.isOpened()) {
        printf("Could not open Webcam\n");
        return 1;
    }

restart:
    if (DEBUG && valueChanged) {
        valueChanged = false;
        printf("HSV: %u %u\t%u %u\t%u %u\t\tSize: %d %d\tFractile filter: %d %d\tNeighbor size: %d \tObject: %d %d\n", iLowH, iHighH, iLowS, iHighS, iLowV, iHighV, size1, size2, windowSize, percentile, neighborSize, objectMin, objectMax);
    }

    Mat image;
    if (!capture.read(image)) {
        printf("Could not read Webcam\n");
        return 1;
    }
    flip(image, image, 1); // Flip image so it acts like a mirror
    resize(image, image, image.size() / 3); // Resize image

#if 0
    printf("image size: %lu, width: %d, height: %d, %lu, color format: %d\n",
            image.total() * image.channels(), image.size().width, image.size().height, image.total(), image.channels());
#endif
    //imshow("Image", image);

    Mat image_hsv;
    cvtColor(image, image_hsv, COLOR_BGR2HSV); // Convert image to HSV

    Mat imgThresholded(image.size(), CV_8UC1);
    Scalar low = Scalar(iLowH, iLowS, iLowV);
    Scalar high = Scalar(iHighH, iHighS, iHighV);

    // Threshold the image
#if 1
    size_t index = 0;
    for (size_t i = 0; i < image_hsv.total(); i++) {
        bool inRange = true;
        for (uint8_t j = 0; j < image_hsv.channels(); j++) {
            uchar value = image_hsv.data[index + j];
            if (j != 0 || low.val[j] < high.val[j]) {
                if (value < low.val[j] || value > high.val[j])
                    inRange = false;
            } else if (value < low.val[j] && value > high.val[j]) // Needed for red color where H value will wrap around [170;10]
                inRange = false;
        }
        imgThresholded.data[i] = inRange ? 255 : 0;
        index += image_hsv.channels();
    }
#else
    inRange(image_hsv, low, high, imgThresholded);
#endif
    //imshow("Thresholded image", imgThresholded);

    // Apply fractile filter to remove salt- and pepper noise
    //double timer = (double)getTickCount();
    Mat fractileFilterImg = fractileFilter(&imgThresholded, windowSize, percentile, true);
    //printf("ms = %f\n", ((double)getTickCount() - timer) / getTickFrequency() * 1000.0);
    //imshow("Fractile filter", fractileFilterImg);

    // Apply morphological closing and opening
    Mat morphologicalFilter = fractileFilterImg.clone();

    // Morphological closing (fill small holes in the foreground)
    dilate(morphologicalFilter, morphologicalFilter, getStructuringElement(MORPH_ELLIPSE, Size(size2, size2)));
    erode(morphologicalFilter, morphologicalFilter, getStructuringElement(MORPH_ELLIPSE, Size(size2, size2)));

    // Morphological opening (remove small objects from the foreground)
    erode(morphologicalFilter, morphologicalFilter, getStructuringElement(MORPH_ELLIPSE, Size(size1, size1)));
    dilate(morphologicalFilter, morphologicalFilter, getStructuringElement(MORPH_ELLIPSE, Size(size1, size1)));

    //imshow("Morphological", morphologicalFilter);

    // Create a image for each segment
    uint8_t nSegments;
    Mat *segments = getSegments(&morphologicalFilter, &nSegments, neighborSize, true);

    // Draw red contour if object is found
    for (uint8_t i = 0; i < nSegments; i++) {
        moments_t moments = calculateMoments(&segments[i], true);
        int16_t eulerNumber = calculateEulerNumber(&segments[i], CONNECTED_8, true);

        // Object detected if it is within the range of the invariant and Euler number is equal to 1
        if (moments.phi1 > (float)objectMin * 1e-4f && moments.phi1 < (float)objectMax * 1e-4f && eulerNumber == 1) {
            const float sideLength = sqrtf(moments.area); // Calculate side length from area
            const float hypotenuse = sqrtf(2 * sideLength * sideLength); // Calculate hypotenuse, assuming that it is square
            //printf("Area: %f %f %f\n", moments.area, sideLength, hypotenuse);
            image = drawMoments(&image, &moments, hypotenuse / 9.0f, 0); // Draw center of mass on image
#if 0 // Use Laplacian filter with lowpass filter to draw the contour
            static LinearFilter lowpassLaplacianFilter = LaplacianFilter() + 9 * LowpassFilter();
            Mat contour = lowpassLaplacianFilter.apply(&segments[i]); // Calculate contour
#elif 0 // Use Laplacian filter to draw the contour
            static LaplacianFilter laplacianFilter;
            Mat contour = laplacianFilter.apply(&segments[i]); // Calculate contour
#else // Use contour search method
            Mat contour;
            if (contoursSearch(&segments[i], &contour, CONNECTED_8, true)) // As there is only one segment it is faster to use the manual approuch
#endif
            {
                //imshow("LaplacianFilter", contour);

                for (size_t i = 0; i < contour.total(); i++) {
                    if (contour.data[i]) {
                        image.data[i * image.channels() + 0] = 0;
                        image.data[i * image.channels() + 1] = 0;
                        image.data[i * image.channels() + 2] = 255; // Draw red contour
                    }
                }
            }
        } /*else if (DEBUG)
            printf("Segment: %u\tPhi: %.4f,%.4f\tEuler number: %d\tSegments: %u\n", i, moments.phi1, moments.phi2, eulerNumber, nSegments);*/
    }

    if (DEBUG) {
        // Create window
        Mat window(2 * image.size().height, 2 * image.size().width, CV_8UC3);
        Mat topLeft(window, Rect(0, 0, image.size().width, image.size().height));
        Mat botLeft(window, Rect(0, imgThresholded.size().height, imgThresholded.size().width, imgThresholded.size().height));
        Mat topRight(window, Rect(image.size().width, 0, image.size().width, image.size().height));
        Mat botRight(window, Rect(image.size().width, image.size().height, image.size().width, image.size().height));

        // Convert to color image, so it actually shows up
        cvtColor(imgThresholded, imgThresholded, COLOR_GRAY2BGR);
        cvtColor(fractileFilterImg, fractileFilterImg, COLOR_GRAY2BGR);
        cvtColor(morphologicalFilter, morphologicalFilter, COLOR_GRAY2BGR);

        // Copy images to window
        image.copyTo(topLeft);
        imgThresholded.copyTo(botLeft);
        fractileFilterImg.copyTo(topRight);
        morphologicalFilter.copyTo(botRight);

        imshow("Window", window);
    } else {
        resize(image, image, image.size() * 2); // Resize image
        imshow("Window", image);
    }

#if 0
    histogram_t histogram = getHistogram(&image);
    Mat hist = drawHistogram(&histogram, &image, image.size());
    imshow("Histogram", hist);
#endif

    if (cvWaitKey(1) != 27) // Wait 1ms to see if ESC is pressed
        goto restart;

    return 0;
}
