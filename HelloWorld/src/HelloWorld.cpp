#include <stdio.h>
#include "cv.h"
#include "cxcore.h"
#include "highgui.h"

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
