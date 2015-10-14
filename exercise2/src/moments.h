#ifndef __moments_h__
#define __moments_h__

using namespace cv;

typedef struct moments_t {
    union {
        float M00;
        float area;
    };
    float M10, M01, M11, M20, M02; // Moments
    float centerX, centerY; // Center of mass
    float u00, u11, u20, u02; // Reduced central moments
    float angle; // Angle of the object
    float n11, n20, n02; // Normalized central moments
    float phi1, phi2; // Invariant moments
} moments_t;

moments_t calculateMoments(const Mat *image, bool whitePixels);
Mat drawMoments(const Mat *image, moments_t *moments, const float centerLength, const float angleLineLength);

#endif
