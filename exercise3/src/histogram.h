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

#ifndef __histogram_h__
#define __histogram_h__

using namespace cv;

struct histogram_t {
    histogram_t(void) {
        memset(data, 0, sizeof(data)); // Make sure all elements are zero
    }
    static const uint16_t nSize = 256;
    uint32_t data[nSize][3]; // Data for three channels
};

void printHistogram(const histogram_t *histogram, uint8_t channels);
histogram_t getHistogram(const Mat *image);
Mat drawHistogram(const histogram_t *histogram, const Mat *image, const Size imageSize, int thresholdValue = -1);

#endif
