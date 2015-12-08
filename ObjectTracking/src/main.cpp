#if __arm__
#include <wiringPi.h> // GPIO access library for the Raspberry Pi
#endif

#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include "contours.h"
#include "euler.h"
#include "filter.h"
#include "histogram.h"
#include "moments.h"
#include "ringbuffer.h"
#include "segmentation.h"

#define PRINT_TIMING 0
#define PRINT_FPS    0

#define FPS_MS (1.0/0.020) // 50 FPS

RingBuffer zombieBuffer;

using namespace std;
using namespace cv;

static bool valueChanged;

#if __arm__
static const uint8_t leftSolenoidPin = 7, rightSolenoidPin = 9; // These two pins control the two solenoids
#endif

void valueChangedCallBack(int pos) {
    valueChanged  = true;
}

#if !(__arm__)
#define digitalWrite(pin, val) (void(0)); // Just added so it compiles on non-arm platforms
#endif

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
            if (val > 0) { // Right side
                digitalWrite(rightSolenoidPin, LOW); // Kill zombie on the right side
            } else { // Left side
                digitalWrite(leftSolenoidPin, LOW); // Kill zombie on the left side
            }
            timer = (double)getTickCount();
            state = SOLENOID_LIFT;
            break;
        case SOLENOID_LIFT:
            if (((double)getTickCount() - timer) / getTickFrequency() * 1000.0 > 30) { // Wait 30 ms for solenoid to go all the way down
                if (val > 0) { // Right side
                    digitalWrite(rightSolenoidPin, HIGH);
                } else { // Left side
                    digitalWrite(leftSolenoidPin, HIGH);
                }
                timer = (double)getTickCount();
                state = SOLENOID_READ;
            }
            break;
    }
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
#if 0 // Green rubiks
    int iLowH = 60;
    int iHighH = 100;

    int iLowS = 100;
    int iHighS = 255;

    int iLowV = 10;
    int iHighV = 255;

    int size1 = 20;
    int size2 = 3;

    int windowSize = 3;
    int percentile = 20;

    int neighborSize = 25;

    int objectMin = 1600;
    int objectMax = 1750;

    int cropPadding = 30;
#elif 1 // ZomBuset
    int iLowH = 40;
    int iHighH = 80;

    int iLowS = 145;
    int iHighS = 255;

    int iLowV = 55;
    int iHighV = 255;

    int size1 = 20; // These are not used on ARM platforms
    int size2 = 3;

    int windowSize = 3;
    int percentile = 50;

    int neighborSize = 5;

    int objectMin = 2100;
    int objectMax = 3200;

    int cropPadding = 30;

    int areaMin = 50;
    int areaMax = 100;
#elif 0 // Red folder
    int iLowH = 170;
    int iHighH = 10;

    int iLowS = 130;
    int iHighS = 255;

    int iLowV = 135;
    int iHighV = 255;

    int size1 = 30;
    int size2 = 3;

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

    int size1 = 12;
    int size2 = 1;

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
    pinMode(leftSolenoidPin, OUTPUT); // Set both pins to output and set low
    pinMode(rightSolenoidPin, OUTPUT);
    digitalWrite(leftSolenoidPin, HIGH); // Input is inverted
    digitalWrite(rightSolenoidPin, HIGH);
    solenoidDone = true;
#endif

    static bool firstRun = true;
    while (1) {
        if (DEBUG && valueChanged) {
            valueChanged = false;
            printf("HSV: %u %u\t%u %u\t%u %u\t\tSize: %d %d\tFractile filter: %d %d\tNeighbor size: %d\tObject: %d %d\tPadding: %d\tArea: %d %d\n", iLowH, iHighH, iLowS, iHighS, iLowV, iHighV, size1, size2, windowSize, percentile, neighborSize, objectMin, objectMax, cropPadding, areaMin, areaMax);
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
        resize(image, image, image.size() / 2);
#if PRINT_TIMING
        printf("Capture = %f ms\t", ((double)getTickCount() - timer) / getTickFrequency() * 1000.0);
        timer = (double)getTickCount();
#endif

    #if 0
        printf("image size: %lu, width: %d, height: %d, %lu, color format: %d\n",
                image.total() * image.channels(), image.size().width, image.size().height, image.total(), image.channels());
    #endif
        //imshow("Image", image);

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
#if PRINT_TIMING
        printf("Threshold = %f ms\t", ((double)getTickCount() - timer) / getTickFrequency() * 1000.0);
        timer = (double)getTickCount();
#endif
        //imshow("Thresholded image", imgThresholded);

        // Apply fractile filter to remove salt- and pepper noise
        Mat fractileFilterImg = fractileFilter(&imgThresholded, windowSize, percentile, true);
#if PRINT_TIMING
        printf("Fractile filter = %f ms\t", ((double)getTickCount() - timer) / getTickFrequency() * 1000.0);
        timer = (double)getTickCount();
#endif
        //imshow("Fractile filter", fractileFilterImg);

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
            continue;

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

#if PRINT_TIMING
        printf("Crop = %f ms\t", ((double)getTickCount() - timer) / getTickFrequency() * 1000.0);
        timer = (double)getTickCount();
#endif

        // Apply morphological closing and opening
        Mat morphologicalFilter = fractileFilterImg.clone();
#if !(__arm__) // Skip morphological filter on ARM, as it runs very very slow!
        // Morphological closing (fill small holes in the foreground)
        dilate(morphologicalFilter, morphologicalFilter, getStructuringElement(MORPH_ELLIPSE, Size(size1, size1)));
        erode(morphologicalFilter, morphologicalFilter, getStructuringElement(MORPH_ELLIPSE, Size(size1, size1)));

        // Morphological opening (remove small objects from the foreground)
        erode(morphologicalFilter, morphologicalFilter, getStructuringElement(MORPH_ELLIPSE, Size(size2, size2)));
        dilate(morphologicalFilter, morphologicalFilter, getStructuringElement(MORPH_ELLIPSE, Size(size2, size2)));
        //imshow("Morphological", morphologicalFilter);
#endif

#if PRINT_TIMING
        printf("Morph = %f ms\t", ((double)getTickCount() - timer) / getTickFrequency() * 1000.0);
        timer = (double)getTickCount();
#endif

        // Create a image for each segment
        uint8_t nSegments;
        Mat *segments = getSegments(&morphologicalFilter, &nSegments, neighborSize, true);
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
                static LinearFilter lowpassLaplacianFilter = LaplacianFilter() + 9 * LowpassFilter();
                Mat contour = lowpassLaplacianFilter.apply(&segments[i]); // Calculate contour
    #elif 0 // Use Laplacian filter to draw the contour
                static LaplacianFilter laplacianFilter;
                Mat contour = laplacianFilter.apply(&segments[i]); // Calculate contour
    #else // Use contour search method
                Mat contour;
                if (contoursSearch(&segments[i], &contour, CONNECTED_8, true)) // When there is only one object in each segment it is faster to use the contours search
    #endif
                {
                    //imshow("LaplacianFilter", contour);

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

        if (DEBUG) {
            // Create windows
            Mat window1(2 * image.size().height, image.size().width, CV_8UC3);
            Mat top1(window1, Rect(0, 0, image.size().width, image.size().height));
            Mat bot1(window1, Rect(0, image.size().height, imgThresholded.size().width, imgThresholded.size().height));

            Mat window2(2 * fractileFilterImg.size().height, fractileFilterImg.size().width, CV_8UC3);
            Mat top2(window2, Rect(0, 0, fractileFilterImg.size().width, fractileFilterImg.size().height));
            Mat bot2(window2, Rect(0, fractileFilterImg.size().height, morphologicalFilter.size().width, morphologicalFilter.size().height));

            // Convert to color images, so it actually shows up
            cvtColor(imgThresholded, imgThresholded, COLOR_GRAY2BGR);
            cvtColor(fractileFilterImg, fractileFilterImg, COLOR_GRAY2BGR);
            cvtColor(morphologicalFilter, morphologicalFilter, COLOR_GRAY2BGR);

            // Copy images to windows
            image.copyTo(top1);
            imgThresholded.copyTo(bot1);
            fractileFilterImg.copyTo(top2);
            morphologicalFilter.copyTo(bot2);

            imshow("Window1", window1);
            imshow("Window2", window2);
        } else {
            resize(image, image, image.size() * 2); // Resize image
            imshow("Window", image);
        }

#if __arm__ || 1
        // Draw blue lines to indicate borders and middle
        static const uint8_t borderWidth = 15;
        line(image, Point(0, borderWidth), Point(image.size().width, borderWidth), Scalar(255, 0, 0)); // Top border
        line(image, Point(0, image.size().height / 2), Point(image.size().width, image.size().height / 2), Scalar(255, 0, 0)); // Middle
        line(image, Point(0, image.size().height - borderWidth), Point(image.size().width, image.size().height - borderWidth), Scalar(255, 0, 0)); // Bottom border

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

        static double zombieDeathTimer = 0;
        static const uint16_t waitTime = 250; // Wait x ms after solenoid has gone all the way up down, as the zombie need to vanish
        static bool waitForSolenoidDone = false;
        if (solenoidDone && waitForSolenoidDone) {
            waitForSolenoidDone = false;
            zombieDeathTimer = (double)getTickCount(); // Set timer value
        }

        // Draw blue line indicating where it is actually looking, as the zombies need to vanish
        /*static const float ignoreWidth = 5;
        static float lastCenterX = -ignoreWidth;
        if (lastCenterX > 0 && (!solenoidDone || ((double)getTickCount() - zombieDeathTimer) / getTickFrequency() * 1000.0 <= waitTime))
            line(image, Point(lastCenterX + ignoreWidth, 0), Point(lastCenterX + ignoreWidth, image.size().height), Scalar(255, 0, 0));*/

        imshow("Areas", image);

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

        for (uint8_t i = 0; i < nZombies; i++) {
            if (moments[i].centerY > borderWidth && moments[i].centerY < image.size().height - borderWidth) { // Ignore plants in the border
                if ((solenoidDone && (((double)getTickCount() - zombieDeathTimer) / getTickFrequency() * 1000.0 > waitTime))) { // If it has been more than x ms since last zombie was killed or the center x position is above
                    if (moments[i].centerY > image.size().height / 2) {
                        zombieCounter[i]++;
                        /*if (zombieCounter[i] < 0) // We want x times in a row, so reset counter if it is negative
                            zombieCounter[i] = 0;*/
                    } else {
                        zombieCounter[i]--;
                        /*if (zombieCounter[i] > 0) // We want x times in a row, so reset counter if it is positive
                            zombieCounter[i] = 0;*/
                    }
                    printf("zombieCounter[%u] = %d\n", i, zombieCounter[i]);
                }
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
                    printf("Right\n");
                    zombieBuffer.write(1); // Indicate to state machine that it should kill a zombie on the right side
                } else {
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

    #if 0
        histogram_t histogram = getHistogram(&image);
        Mat hist = drawHistogram(&histogram, &image, image.size());
        imshow("Histogram", hist);
    #endif

        if (firstRun) {
            firstRun = false;
            printf("\n\nPress any key to start\n");
            if (cvWaitKey() == 27)
                goto end; // Bail out if user press ESC
        } else {
            double dt =  ((double)getTickCount() - startTimer) / getTickFrequency() * 1000.0;
            int delay = FPS_MS - dt;
            if (delay <= 0) // If the loop has spent more than 30 ms we will just wait the minimum amount
                delay = 1; // Set delay to 1 ms, as 0 will wait infinitely
            if (cvWaitKey(delay) == 27) // Limit FPS to 50
                goto end; // Wait 1ms to see if ESC is pressed

#if PRINT_FPS
            printf("FPS = %.2f\n", 1.0/(((double)getTickCount() - startTimer) / getTickFrequency()));
#endif
        }

#if PRINT_TIMING
        printf("Total = %f ms\n", ((double)getTickCount() - startTimer) / getTickFrequency() * 1000.0);
#endif
    }

end:
#if __arm__
    digitalWrite(rightSolenoidPin, HIGH);
    digitalWrite(leftSolenoidPin, HIGH);
#endif

    return 0;
}
