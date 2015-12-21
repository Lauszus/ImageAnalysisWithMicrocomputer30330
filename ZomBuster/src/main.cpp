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

#include "contours.h"
#include "euler.h"
#include "filter.h"
#include "histogram.h"
#include "moments.h"
#include "segmentation.h"

#define PRINT_TIMING 0
#define PRINT_FPS    0

#define FPS_MS (1.0/0.020) // 50 FPS

using namespace cv;

static bool valueChanged;
static void valueChangedCallBack(int pos) {
    valueChanged  = true;
}

#if __arm__
#include <wiringPi.h> // GPIO access library for the Raspberry Pi
#include "ringbuffer.h" // Ring buffer class

RingBuffer zombieBuffer;

static const uint8_t leftSolenoidPin = 9, rightSolenoidPin = 7, buttonPin = 8; // These first two pins control the two solenoids the last is a button input

enum Solenoid_e {
    SOLENOID_READ,
    SOLENOID_KILL,
    SOLENOID_LIFT,
};

static bool solenoidDone;

static void runSolenoidStateMachine(void) {
    static Solenoid_e state = SOLENOID_READ;
    static int val;
    static double timer;

    switch (state) {
        case SOLENOID_READ:
            if (((double)getTickCount() - timer) / getTickFrequency() * 1000.0 > 30) { // Wait 30 ms for solenoid to go up again
                solenoidDone = true;
                if (zombieBuffer.available()) {
                    val = zombieBuffer.read();
                    state = SOLENOID_KILL;
                    solenoidDone = false;
                }
            }
            break;
        case SOLENOID_KILL:
            if (val > 0) // Right side
                digitalWrite(rightSolenoidPin, LOW); // Kill zombie on the right side
            else // Left side
                digitalWrite(leftSolenoidPin, LOW); // Kill zombie on the left side
            timer = (double)getTickCount();
            state = SOLENOID_LIFT;
            break;
        case SOLENOID_LIFT:
            if (((double)getTickCount() - timer) / getTickFrequency() * 1000.0 > 30) { // Wait 30 ms for solenoid to go all the way down
                if (val > 0) // Right side
                    digitalWrite(rightSolenoidPin, HIGH);
                else // Left side
                    digitalWrite(leftSolenoidPin, HIGH);
                timer = (double)getTickCount();
                state = SOLENOID_READ;
            }
            break;
    }
}
#endif

int main(int argc, char *argv[]) {
    static const bool DEBUG = argc == 2 ? argv[1][0] == '1' : false; // Check if DEBUG flag is set

#if 0 // Green rubiks
    int iLowH = 60;
    int iHighH = 100;

    int iLowS = 100;
    int iHighS = 255;

    int iLowV = 10;
    int iHighV = 255;

    int closingSize = 20;
    int openingSize = 3;

    int windowSize = 3;
    int percentile = 20;

    int neighborSize = 25;

    int objectMin = 1600;
    int objectMax = 1750;

    int cropPadding = 30;

    int areaMin = 0; // Can be any size
    int areaMax = (uint16_t)~0;
#else // ZomBuset
    int iLowH = 40;
    int iHighH = 80;

    int iLowS = 145;
    int iHighS = 255;

    int iLowV = 55;
    int iHighV = 255;

    int closingSize = 20;
    int openingSize = 3;

    int windowSize = 3;
    int percentile = 50;

    int neighborSize = 5;

    int objectMin = 2100;
    int objectMax = 3200;

    int cropPadding = 30;

    int areaMin = 50;
    int areaMax = 100;
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

        cvCreateTrackbar("Closing size", controlWindow, &closingSize, 50, valueChangedCallBack);
        cvCreateTrackbar("Opening size", controlWindow, &openingSize, 50, valueChangedCallBack);

        cvCreateTrackbar("Window size", controlWindow, &windowSize, 10, valueChangedCallBack);
        cvCreateTrackbar("Percentile", controlWindow, &percentile, 100, valueChangedCallBack);

        cvCreateTrackbar("Neighbor size", controlWindow, &neighborSize, 50, valueChangedCallBack);

        cvCreateTrackbar("Object min", controlWindow, &objectMin, 4000, valueChangedCallBack);
        cvCreateTrackbar("Object max", controlWindow, &objectMax, 4000, valueChangedCallBack);

        cvCreateTrackbar("Crop padding", controlWindow, &cropPadding, 100, valueChangedCallBack);

        cvCreateTrackbar("Area min", controlWindow, &areaMin, 200, valueChangedCallBack);
        cvCreateTrackbar("Area max", controlWindow, &areaMax, 200, valueChangedCallBack);
    }

    VideoCapture capture(0); // Capture video from webcam
    if (!capture.isOpened()) {
        printf("Could not open Webcam\n");
        return 1;
    }
    capture.set(CV_CAP_PROP_FRAME_WIDTH, 320);
    capture.set(CV_CAP_PROP_FRAME_HEIGHT, 240);

#if __arm__
    if (wiringPiSetup() == -1) { // Setup WiringPi
        printf("wiringPiSetup failed!\n");
        return 1;
    }
    pinMode(leftSolenoidPin, OUTPUT); // Set both pins to output
    pinMode(rightSolenoidPin, OUTPUT);
    digitalWrite(leftSolenoidPin, HIGH); // Input is inverted, so HIGH disables the solenoid
    digitalWrite(rightSolenoidPin, HIGH);
    pinMode(buttonPin, INPUT);
    pullUpDnControl(buttonPin, PUD_UP); // Enable pull-up resistor
    solenoidDone = true;

    printf("Ready to kill some zombies!\n");
    while (digitalRead(buttonPin)) { // Quit if button is pressed
#else
    while (cvWaitKey(1) != 27) { // End if ESC is pressed
#endif
        if (DEBUG && valueChanged) {
            valueChanged = false;
            printf("HSV: %u %u\t%u %u\t%u %u\t\tSize: %d %d\tFractile filter: %d %d\tNeighbor size: %d\tObject: %d %d\tPadding: %d\tArea: %d %d\n", iLowH, iHighH, iLowS, iHighS, iLowV, iHighV, closingSize, openingSize, windowSize, percentile, neighborSize, objectMin, objectMax, cropPadding, areaMin, areaMax);
        }

        double startTimer = (double)getTickCount();
#if PRINT_TIMING
        double timer = startTimer;
#endif
        Mat image;
        if (!capture.read(image)) {
            printf("Could not read Webcam\n");
            return 1;
        }
        //flip(image, image, 1); // Flip image so it acts like a mirror
#if __arm__
        resize(image, image, image.size() / 2); // Make image even smaller on ARM platforms
#endif
#if PRINT_TIMING
        printf("Capture = %f ms\t", ((double)getTickCount() - timer) / getTickFrequency() * 1000.0);
        timer = (double)getTickCount();
#endif

    #if 0
        printf("image size: %lu, width: %d, height: %d, %lu, color format: %d\n",
                image.total() * image.channels(), image.size().width, image.size().height, image.total(), image.channels());
    #endif
        //imshow("Image", image);
        //imwrite("img/image.png", image);

        Mat image_hsv;
        cvtColor(image, image_hsv, COLOR_BGR2HSV); // Convert image to HSV
#if PRINT_TIMING
        printf("HSV = %f ms\t", ((double)getTickCount() - timer) / getTickFrequency() * 1000.0);
        timer = (double)getTickCount();
#endif

        Mat imgThresholded(image.size(), CV_8UC1);
        Scalar low = Scalar(iLowH, iLowS, iLowV);
        Scalar high = Scalar(iHighH, iHighS, iHighV);

        // Threshold the image
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
            imgThresholded.data[i] = inRange ? 255 : 0; // Draw thresholded object white
            index += image_hsv.channels();
        }

#if PRINT_TIMING
        printf("Threshold = %f ms\t", ((double)getTickCount() - timer) / getTickFrequency() * 1000.0);
        timer = (double)getTickCount();
#endif
        //imshow("Thresholded image", imgThresholded);
        //imwrite("img/imgThresholded.png", imgThresholded);

        // Apply fractile filter to remove salt- and pepper noise
        Mat fractileFilterImg = fractileFilter(&imgThresholded, windowSize, percentile, false);
#if PRINT_TIMING
        printf("Fractile filter = %f ms\t", ((double)getTickCount() - timer) / getTickFrequency() * 1000.0);
        timer = (double)getTickCount();
#endif
        //imshow("Fractile filter", fractileFilterImg);
        //imwrite("img/fractileFilterImg.png", fractileFilterImg);

#if 1
        // Crop image, so we are only looking at the actual data
        index = 0;
        int minX, maxX, minY, maxY;
        minX = fractileFilterImg.size().width - 1;
        minY = fractileFilterImg.size().height - 1;
        maxX = maxY = 0;
        for (size_t y = 0; y < fractileFilterImg.size().height; y++) {
            for (size_t x = 0; x < fractileFilterImg.size().width; x++) {
                if (fractileFilterImg.data[index]) { // Look for all white pixels
                    if (x < minX)
                        minX = x;
                    else if (x > maxX)
                        maxX = x;
                    if (y < minY)
                        minY = y;
                    else if (y > maxY)
                        maxY = y;
                }
                index += fractileFilterImg.channels();
            }
        }
        //printf("%d,%d,%d,%d\n", minX, maxX, minY, maxY);

        if (maxX == 0 || maxY == 0)
            continue; // Skip, as there was no object detected

        minX -= cropPadding;
        minY -= cropPadding;
        if (minX < 0)
            minX = 0;
        if (minY < 0)
            minY = 0;

        int width = maxX - minX;
        int height = maxY - minY;
        width += cropPadding;
        height += cropPadding;
        if (minX + width >= fractileFilterImg.size().width)
            width = fractileFilterImg.size().width - 1 - minX;
        if (minY + height >= fractileFilterImg.size().height)
            height = fractileFilterImg.size().height - 1 - minY;

        fractileFilterImg = Mat(fractileFilterImg, Rect(minX, minY, width, height)).clone(); // Do the actual cropping
#else
        int minX = 0, minY = 0;
#endif

#if PRINT_TIMING
        printf("Crop = %f ms\t", ((double)getTickCount() - timer) / getTickFrequency() * 1000.0);
        timer = (double)getTickCount();
#endif

        // Apply morphological closing and opening
        Mat morphologicalFilterImg = fractileFilterImg.clone();
        // Morphological closing (Remove small dark spots (i.e. "pepper") and connect small bright cracks)
        morphologicalFilterImg = morphologicalFilter(&morphologicalFilterImg, DILATION, closingSize, true);
        morphologicalFilterImg = morphologicalFilter(&morphologicalFilterImg, EROSION, closingSize, true);

        // Morphological opening (Remove small bright spots (i.e. "salt") and connect small dark cracks)
        morphologicalFilterImg = morphologicalFilter(&morphologicalFilterImg, EROSION, openingSize, true);
        morphologicalFilterImg = morphologicalFilter(&morphologicalFilterImg, DILATION, openingSize, true);
        //imshow("Morphological", morphologicalFilterImg);
        //imwrite("img/morphologicalFilterImg.png", morphologicalFilterImg);

#if PRINT_TIMING
        printf("Morph = %f ms\t", ((double)getTickCount() - timer) / getTickFrequency() * 1000.0);
        timer = (double)getTickCount();
#endif

        // Create a image for each segment
        uint8_t nSegments;
        Mat *segments = getSegments(&morphologicalFilterImg, &nSegments, neighborSize, true);
#if PRINT_TIMING
        printf("Segments = %f ms\t", ((double)getTickCount() - timer) / getTickFrequency() * 1000.0);
        timer = (double)getTickCount();
#endif

        // Draw red contour if object is found
        moments_t moments[nSegments];
        uint8_t objectsDetected = 0;
        for (uint8_t i = 0; i < nSegments; i++) {
            moments_t momentsTmp = calculateMoments(&segments[i], true);
            int16_t eulerNumber = calculateEulerNumber(&segments[i], CONNECTED_8, true);

            // Object detected if it is within the range of the invariant, has the right area and Euler number is equal to 1
            if (momentsTmp.phi1 > (float)objectMin * 1e-4f && momentsTmp.phi1 < (float)objectMax * 1e-4f && momentsTmp.area > areaMin && momentsTmp.area < areaMax && eulerNumber == 1) {
                const float sideLength = sqrtf(momentsTmp.area); // Calculate side length from area
                const float hypotenuse = sqrtf(2 * sideLength * sideLength); // Calculate hypotenuse, assuming that it is square
                //printf("Area: %f %f %f\n", moments.area, sideLength, hypotenuse);
                momentsTmp.centerX += minX; // Convert to x,y coordinates in original image
                momentsTmp.centerY += minY;
                image = drawMoments(&image, &momentsTmp, hypotenuse / 9.0f, 0); // Draw center of mass on original image
                moments[objectsDetected++] = momentsTmp; // Save the moments of detected objects
    #if 0 // Use Laplacian filter with lowpass filter to draw the contour
                static LinearFilter lowpassLaplacianFilter = LaplacianFilter() + LowpassFilter();
                Mat contour = lowpassLaplacianFilter.apply(&segments[i]); // Calculate contour
    #elif 0 // Use Laplacian filter to draw the contour
                static LaplacianFilter laplacianFilter;
                Mat contour = laplacianFilter.apply(&segments[i]); // Calculate contour
    #else // Use contour search method
                Mat contour;
                if (contoursSearch(&segments[i], &contour, CONNECTED_8, true)) // When there is only one object in each segment it is faster to use the contours search
    #endif
                {
                    //imshow("contour", contour);
                    //imwrite("img/contour.png", contour);

                    index = 0;
                    for (size_t y = 0; y < contour.size().height; y++) {
                        for (size_t x = 0; x < contour.size().width; x++) {
                            if (contour.data[index]) {
                                size_t subIndex = ((x + minX) + (y + minY) * image.size().width) * image.channels(); // Convert to x,y coordinates in original image
                                image.data[subIndex + 0] = 0;
                                image.data[subIndex + 1] = 0;
                                image.data[subIndex + 2] = 255; // Draw red contour
                            }
                            index++;
                        }
                    }
                }
            } /*else if (DEBUG)
                printf("Segment: %u\tPhi: %.4f,%.4f\tEuler number: %d\tSegments: %u\n", i, moments.phi1, moments.phi2, eulerNumber, nSegments);*/
        }
#if PRINT_TIMING
        printf("Contour = %f ms\t", ((double)getTickCount() - timer) / getTickFrequency() * 1000.0);
#endif
        //imwrite("img/image_contour.png", image);

        if (DEBUG) {
            // Create windows
            Mat window1(2 * image.size().height, image.size().width, CV_8UC3);
            Mat top1(window1, Rect(0, 0, image.size().width, image.size().height));
            Mat bot1(window1, Rect(0, image.size().height, imgThresholded.size().width, imgThresholded.size().height));

            Mat window2(2 * fractileFilterImg.size().height, fractileFilterImg.size().width, CV_8UC3);
            Mat top2(window2, Rect(0, 0, fractileFilterImg.size().width, fractileFilterImg.size().height));
            Mat bot2(window2, Rect(0, fractileFilterImg.size().height, morphologicalFilterImg.size().width, morphologicalFilterImg.size().height));

            // Convert to color images, so it actually shows up
            cvtColor(imgThresholded, imgThresholded, COLOR_GRAY2BGR);
            cvtColor(fractileFilterImg, fractileFilterImg, COLOR_GRAY2BGR);
            cvtColor(morphologicalFilterImg, morphologicalFilterImg, COLOR_GRAY2BGR);

            // Copy images to windows
            image.copyTo(top1);
            imgThresholded.copyTo(bot1);
            fractileFilterImg.copyTo(top2);
            morphologicalFilterImg.copyTo(bot2);

            imshow("Window1", window1);
            imshow("Window2", window2);
        }

#if __arm__
        static double zombieDeathTimer = 0;
        static const uint16_t waitTime = 250; // Wait x ms after solenoid has gone all the way up down, as the zombie need to vanish
        static bool waitForSolenoidDone = false;
        if (solenoidDone && waitForSolenoidDone) {
            waitForSolenoidDone = false;
            zombieDeathTimer = (double)getTickCount(); // Set timer value
        }

        static const int8_t topBorder = 5, bottomBorder = 20, middleOffset = -10;
        if (DEBUG) {
                // Draw blue lines to indicate borders and middle
                line(image, Point(0, topBorder), Point(image.size().width, topBorder), Scalar(255, 0, 0)); // Top border
                line(image, Point(0, image.size().height / 2 + middleOffset), Point(image.size().width, image.size().height / 2 + middleOffset), Scalar(255, 0, 0)); // Middle
                line(image, Point(0, image.size().height - bottomBorder), Point(image.size().width, image.size().height - bottomBorder), Scalar(255, 0, 0)); // Bottom border

                // Draw blue line indicating where it is actually looking, as the zombies need to vanish
                /*static const float ignoreWidth = 5;
                static float lastCenterX = -ignoreWidth;
                if (lastCenterX > 0 && (!solenoidDone || ((double)getTickCount() - zombieDeathTimer) / getTickFrequency() * 1000.0 <= waitTime))
                    line(image, Point(lastCenterX + ignoreWidth, 0), Point(lastCenterX + ignoreWidth, image.size().height), Scalar(255, 0, 0));*/

                imshow("Areas", image);
        }

        // Sort moments in ascending order according to the centerX position
        // Inspired by: http://www.tenouk.com/cpluscodesnippet/sortarrayelementasc.html
        for (uint8_t i = 1; i < objectsDetected; i++) {
            for (uint8_t j = 0; j < objectsDetected - 1; j++) {
                if (moments[j].centerX > moments[j + 1].centerX) {
                    moments_t temp = moments[j];
                    moments[j] = moments[j + 1];
                    moments[j + 1] = temp;
                }
            }
        }

#if 0 // Used to analyze the images
        static uint16_t counter = 0;
        char buf[30];
        sprintf(buf, "img/image%u.jpg", counter++);
        imwrite(buf, image);
#endif

        static uint8_t rounds = 0;
        static uint8_t nZombies; // Track up to this number of zombies
        if (rounds < 10) {
            rounds++;
            nZombies = 2; // Only track two in the beginning because of the text
        } else
            nZombies = 4;
        static int8_t zombieCounter[4];

        //static double timer = 0;
        for (uint8_t i = 0; i < nZombies; i++) {
            if (moments[i].centerY > topBorder && moments[i].centerY < image.size().height - bottomBorder) { // Ignore plants in the border
                if ((solenoidDone && (((double)getTickCount() - zombieDeathTimer) / getTickFrequency() * 1000.0 > waitTime))) { // If it has been more than x ms since last zombie was killed or the center x position is above
                    //timer = (double)getTickCount(); // Set timer value
                    if (moments[i].centerY > image.size().height / 2 + middleOffset) {
                        zombieCounter[i]++;
                        /*if (zombieCounter[i] < 0) // We want x times in a row, so reset counter if it is negative
                            zombieCounter[i] = 0;*/
                    } else {
                        zombieCounter[i]--;
                        /*if (zombieCounter[i] > 0) // We want x times in a row, so reset counter if it is positive
                            zombieCounter[i] = 0;*/
                    }
                    if (DEBUG)
                        printf("zombieCounter[%u] = %d\n", i, zombieCounter[i]);
                } /*else if (i < 2 && moments[i].centerX >= lastCenterX + ignoreWidth && ((double)getTickCount() - timer) / getTickFrequency() * 1000.0 > 300) {
                    timer = (double)getTickCount(); // Set timer value
                    if (moments[i].centerY > image.size().height / 2 + middleOffset)
                        zombieCounter[0]++;
                    else
                        zombieCounter[0]--;
                    break;
                }*/
            }
        }

        uint8_t zombieDeaths = 0;
        for (uint8_t i = 0; i < nZombies; i++) {
            if (abs(zombieCounter[0]) >= 1) { // Check how many times in a row we have seen that zombie
                //lastCenterX = moments[0].centerX; // Store center x value
                solenoidDone = false;
                waitForSolenoidDone = true;
                zombieDeaths++;
                if (zombieCounter[0] > 0) {
                    if (DEBUG)
                        printf("Right\n");
                    zombieBuffer.write(1); // Indicate to state machine that it should kill a zombie on the right side
                } else {
                    if (DEBUG)
                        printf("Left\n");
                    zombieBuffer.write(-1); // Indicate to state machine that it should kill a zombie on the left side
                }
                for (uint8_t i = 0; i < nZombies - 1; i++)
                    zombieCounter[i] = zombieCounter[i + 1]; // Move all one down
            } else
                break;
        }
        for (int8_t i = nZombies; i > nZombies - zombieDeaths; i--)
            zombieCounter[i - 1] = 0; // Reset to initial value, as they are moved down

        runSolenoidStateMachine(); // This state machine control the solenoids without blocking the code
#endif

        double dt =  ((double)getTickCount() - startTimer) / getTickFrequency() * 1000.0;
        int delay = FPS_MS - dt; // Limit to 50 FPS
        if (delay <= 0) // If the loop has spent more than 30 ms we will just wait the minimum amount
            delay = 1; // Set delay to 1 ms, as 0 will wait infinitely
#if __arm__
        if (cvWaitKey(delay) == 27 || !digitalRead(buttonPin)) // End if either ESC or button is pressed
#else
        if (cvWaitKey(delay) == 27) // End if ESC is pressed
#endif
            break; // TODO: Run solenoids while waiting

#if PRINT_FPS
            printf("FPS = %.2f\n", 1.0/(((double)getTickCount() - startTimer) / getTickFrequency()));
#endif

#if PRINT_TIMING
        printf("Total = %f ms\n", ((double)getTickCount() - startTimer) / getTickFrequency() * 1000.0);
#endif
    }

    releaseSegments(); // Release all segments inside segmentation.cpp
#if __arm__
    digitalWrite(rightSolenoidPin, HIGH); // Turn both solenoids off
    digitalWrite(leftSolenoidPin, HIGH);
#endif

    return 0;
}
