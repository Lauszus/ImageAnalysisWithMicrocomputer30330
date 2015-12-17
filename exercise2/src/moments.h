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
