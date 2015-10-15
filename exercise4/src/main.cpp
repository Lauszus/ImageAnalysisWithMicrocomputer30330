#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

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
    int size1 = 5;
    int size2 = 5;

    cvCreateTrackbar("Threshold", controlWindow, &thresholdValue, 255, valueChangedCallBack);
    cvCreateTrackbar("Size1", controlWindow, &size1, 10, valueChangedCallBack);
    cvCreateTrackbar("Size2", controlWindow, &size2, 10, valueChangedCallBack);

    moveWindow(controlWindow, 0, 500); // Move window

    static Mat imageFull = imread("../files/PEN.pgm", IMREAD_GRAYSCALE);
    static Mat image = imageFull(Rect(2, 2, imageFull.size().width - 2 * 2, imageFull.size().height - 2 * 2)).clone(); // Crop two pixels from both sides
    imshow("Image", image);

restart:
    printf("Threshold: %d\tSize: %d,%d\n", thresholdValue, size1, size2);

    histogram_t histogram = getHistogram(&image);
    Mat hist = drawHistogram(&histogram, &image, Size(image.size().width * 2, image.size().height), thresholdValue);
    imshow("Hist", hist);

    Mat imageThreshold = image.clone();
    for (int i = 0; i < imageThreshold.total() * imageThreshold.channels(); i++)
        imageThreshold.data[i] = imageThreshold.data[i] >= thresholdValue ? 255 : 0; // Draw threshold image
    imshow("Threshold", imageThreshold);

    // Apply morphological closing and opening
    Mat morphologicalFilter = imageThreshold.clone();

    // Morphological closing (fill small holes in the foreground)
    dilate(morphologicalFilter, morphologicalFilter, getStructuringElement(MORPH_ELLIPSE, Size(size2, size2)));
    erode(morphologicalFilter, morphologicalFilter, getStructuringElement(MORPH_ELLIPSE, Size(size2, size2)));

    // Morphological opening (remove small objects from the foreground)
    erode(morphologicalFilter, morphologicalFilter, getStructuringElement(MORPH_ELLIPSE, Size(size1, size1)));
    dilate(morphologicalFilter, morphologicalFilter, getStructuringElement(MORPH_ELLIPSE, Size(size1, size1)));

    imshow("Morphological filter", morphologicalFilter);

    Mat contour;
    if (contoursSearch(&morphologicalFilter, &contour, CONNECTED_8, false))
        imshow("Contour", contour);

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
