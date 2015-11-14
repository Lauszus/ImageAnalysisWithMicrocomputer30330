#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include "moments.h"

using namespace std;
using namespace cv;

moments_t calculateMoments(const Mat *image, bool whitePixels) {
    assert(image->channels() == 1); // Picture must be black and white image

    moments_t moments;
    // Calculate moments
    moments.M00 = moments.M10 = moments.M01 = moments.M11 = moments.M20 = moments.M02 = 0;
    size_t index = 0;
    for (int y = 0; y < image->size().height; y++) {
        for (int x  = 0; x < image->size().width; x++) {
            if ((bool)image->data[index++] == whitePixels) {
                moments.M00++;
                // First moments
                moments.M10 += x;
                moments.M01 += y;
                // Second moments
                moments.M11 += x * y;
                moments.M20 += x * x;
                moments.M02 += y * y;
            }
        }
    }

    // Calculate the center of mass
    moments.centerX = moments.M10 / moments.M00;
    moments.centerY = moments.M01 / moments.M00;

    // Calculate reduced central moments
    moments.u00 = moments.M00;
#if 1
    moments.u11 = moments.M11 - moments.centerY * moments.M10;
#if 0
    const float u11_x = moments.M11 - moments.centerX * moments.M01;
    //printf("%.2f == %.2f\n", moments.u11, u11_x); // Should be the same
    assert(moments.u11 == u11_x);
#endif
    moments.u20 = moments.M20 - moments.centerX * moments.M10;
    moments.u02 = moments.M02 - moments.centerY * moments.M01;
#else
    moments.u11 = moments.u20 = moments.u02 = 0;
    index = 0;
    for (int y = 0; y < image->size().height; y++) {
            for (int x  = 0; x < image->size().width; x++) {
                if ((bool)image->data[index++] == whitePixels) {
                    moments.u11 += (x - moments.centerX) * (y - moments.centerY);
                    moments.u20 += (x - moments.centerX) * (x - moments.centerX);
                    moments.u02 += (y - moments.centerY) * (y - moments.centerY);
                }
            }
    }
#endif

#if 0 // Set to 1 in order to normalize data
    moments.u11 /= moments.u00;
    moments.u20 /= moments.u00;
    moments.u02 /= moments.u00;
#endif

    // Calculate angle
    moments.angle = 0.5f * atan2f(2.0f * moments.u11, moments.u20 - moments.u02);
    //printf("Center: %.2f,%.2f\tAngle: %.2f\n", moments.centerX, moments.centerY, moments.angle * 180.0f / M_PI);

    // Normalized central moments
    moments.n11 = moments.u11 / (moments.u00 * moments.u00); // Gamma = (1 + 1) / 2 + 1 = 2
    moments.n20 = moments.u20 / (moments.u00 * moments.u00); // Gamma = (2 + 0) / 2 + 1 = 2
    moments.n02 = moments.u02 / (moments.u00 * moments.u00); // Gamma = (2 + 0) / 2 + 1 = 2

    // Invariant moments
    moments.phi1 = moments.n20 + moments.n02;
    moments.phi2 = (moments.n20 + moments.n02) * (moments.n20 + moments.n02) + 4 * (moments.n11 * moments.n11);
    //printf("n: %.2f,%.2f\tphi: %.2f,%.2f\n", moments.n20, moments.n02, moments.phi1, moments.phi2);

    return moments;
}

Mat drawMoments(const Mat *image, moments_t *moments, const float centerLength, const float angleLineLength) {
    Mat out;
    if (image->channels() == 1)
        cvtColor(*image, out, COLOR_GRAY2BGR); // Convert image into 3-channel image, so we can draw in color
    else
        out = image->clone();

    // Draw center of mass if length is larger than 0
    if (centerLength > 0) {
        line(out, Point(moments->centerX - centerLength, moments->centerY - centerLength), Point(moments->centerX + centerLength, moments->centerY + centerLength), Scalar(0, 0, 255));
        line(out, Point(moments->centerX - centerLength, moments->centerY + centerLength), Point(moments->centerX + centerLength, moments->centerY - centerLength), Scalar(0, 0, 255));
    }

    // Draw angle line if length is larger than 0
    if (angleLineLength > 0)
        line(out, Point(moments->centerX + angleLineLength * cosf(moments->angle), moments->centerY + angleLineLength * sinf(moments->angle)), Point(moments->centerX - angleLineLength * cosf(moments->angle), moments->centerY - angleLineLength * sinf(moments->angle)), Scalar(0, 0, 255));

    return out;
}
