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

#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

using namespace cv;

int main(int argc, char *argv[]) {
    const char *wName = "Hello world!";
    cvNamedWindow(wName, 0);

    CvCapture *capture = 0;

    IplImage *frame, *frame_copy = 0;
    capture = cvCaptureFromCAM(0);

    if (capture) {
        for (;;) {
            if (!cvGrabFrame(capture))
                break;
            frame = cvRetrieveFrame(capture);

            if (!frame)
                break;
            if (!frame_copy) {
                printf("Frame settings:\n Width: %d\n Height: %d\n", frame->width, frame->height);
                frame_copy = cvCreateImage(cvSize(frame->width, frame->height), IPL_DEPTH_8U, frame->nChannels);
                cvResizeWindow(wName, frame->width, frame->height);
            }
            if (frame->origin == IPL_ORIGIN_TL)
                cvCopy(frame, frame_copy, 0);
            else
                cvFlip(frame, frame_copy, 0);
            cvFlip(frame, frame_copy, 1);
            cvShowImage(wName, frame_copy);

            if (cvWaitKey(5) > 0)
                break;
        }
    }

    if (capture)
        cvReleaseCapture(&capture);
    cvReleaseImage(&frame_copy);
    cvDestroyWindow(wName);

    return 0;
}
