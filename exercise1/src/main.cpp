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

using namespace cv;

typedef struct histogram_t {
    static const int nSize = 256;
    int data[nSize][3]; // Data for three channels
} histogram_t;

#define GREYSCALE 0

int main(int argc, char *argv[]) {
    for (int i = 0; i < argc; i++)
        printf("argv[%d] = %s\n", i, argv[i]);

restart:
    const char *filePath;
#if 0
    argc = 2;
    filePath = "../files/ariane5_1b.jpg";
#else
    if (argc > 1)
        filePath = argv[1];
#endif

    CvCapture *capture;
    IplImage *image;
    if (argc < 2) {
        // Use Webcam if no argument is given
        capture = cvCaptureFromCAM(0);
        if (!capture)
            return 1; // Return error in case capture is NULL
        if (!cvGrabFrame(capture))
            return 1; // Return error in case picture can not be captured
        IplImage *frame = cvRetrieveFrame(capture);
        if (!frame)
            return 1; // Return error in case frame is NULL

        IplImage *frame_copy = cvCreateImage(cvSize(frame->width, frame->height), IPL_DEPTH_8U, frame->nChannels);
        if (frame->origin == IPL_ORIGIN_TL)
            cvCopy(frame, frame_copy);
        else
            cvFlip(frame, frame_copy);
        cvFlip(frame, frame_copy, 1);
#if GREYSCALE
        // Convert to greyscale
        image = cvCreateImage(cvSize(frame->width, frame->height), IPL_DEPTH_8U, 1);
        cvCvtColor(frame_copy, image, CV_BGR2GRAY); // OpenCV uses BGR order by default
#else
        // Just copy image
        image = cvCreateImage(cvSize(frame->width, frame->height), IPL_DEPTH_8U, frame->nChannels);
        cvCopy(frame_copy, image);
#endif
        cvReleaseImage(&frame_copy);
    } else {
        image = cvLoadImage(filePath, GREYSCALE ? CV_LOAD_IMAGE_GRAYSCALE : CV_LOAD_IMAGE_COLOR);
        const char *fileName = strrchr(filePath, '/') + 1;
        const char *format = strrchr(filePath, '.') + 1;
        printf("File name: %s, format: %s, ", fileName, format);
    }
    if (image->dataOrder != IPL_DATA_ORDER_PIXEL) {
        printf("Data order has to be interleaved: %u\n", image->dataOrder);
        return 1;
    }

    printf("image size: %d, width: %d, height: %d, color format: %d\n",
               image->imageSize, image->width, image->height, image->nChannels);

    const char *imageWindow = "Image";
    cvNamedWindow(imageWindow);
    cvResizeWindow(imageWindow, image->width, image->height);
    cvShowImage(imageWindow, image);

    histogram_t histogram;
    memset(histogram.data, 0, sizeof(histogram.data));
    for (int i = 0; i < image->width * image->height; i++) {
        for (int j = 0; j < image->nChannels; j++) {
            unsigned char value = image->imageData[image->nChannels * i + j];
            histogram.data[value][j]++;
        }
    }

    unsigned int total = 0;
    for (int i = 0; i < histogram.nSize; i++) {
        for (int j = 0; j < image->nChannels; j++) {
            //printf("%d %d\n", i, histogram.data[i][j]);
            total += histogram.data[i][j];
        }
    }
    printf("%d\n", total); // Should be equal to image->imageSize

    const char *histWindow = "Histogram";
    IplImage *hist = cvCreateImage(cvSize(image->width, image->height), IPL_DEPTH_8U, image->nChannels);
    cvNamedWindow(histWindow);
    cvRectangle(hist, cvPoint(0, 0), cvPoint(hist->width, hist->height), cvScalar(255, 255, 255, 0), CV_FILLED); // White background

    const int startX = 50;
    const int endX = hist->width - 50;
    const int startY = 50;
    const int endY = hist->height - 50;

    cvLine(hist, cvPoint(startX, endY), cvPoint(endX, endY), cvScalar(0)); // x-axis
    cvLine(hist, cvPoint(startX, startY), cvPoint(startX, endY), cvScalar(0)); // y-axis

    for (int i = 0; i < histogram.nSize; i++) {
        int posX  = startX + 5 + 2*i;
        for (int j = 0; j < image->nChannels; j++)
            cvLine(hist, cvPoint(posX, endY - histogram.data[i][j] / 300), cvPoint(posX, endY),
                                            /* Draw in colors if color image, else draw black if greyscale */
                                            cvScalar(j == 0 && image->nChannels > 1? 255 : 0,
                                                     j == 1 ? 255 : 0,
                                                     j == 2 ? 255 : 0, 0));
    }
    cvShowImage(histWindow, hist);

    char key = cvWaitKey(); // Wait until a key is pressed
    if (key != 'a' && argc < 2)
        goto restart;

#if 1
    cvSaveImage("image.png", image);
#else
    FILE *fp = fopen("image.pgm", "w");
    fprintf(fp, "P5 %d %d 255\n", image->width, image->height);
    fprintf(fp, "# Some comment");
    fwrite(image->imageData, image->width, image->height, fp);
    fclose(fp);
#endif

    if (capture)
        cvReleaseCapture(&capture);
    cvReleaseImage(&image);
    cvReleaseImage(&hist);
    cvDestroyWindow(imageWindow);
    cvDestroyWindow(histWindow);

    return 0;
}
