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

#include "opencv2/imgproc.hpp"

#include "euler.h"

using namespace cv;

int16_t calculateEulerNumber(const Mat *image, const Connected connected, bool whitePixels) {
    assert(image->channels() == 1); // Image must be in black and white

    const Size size = image->size();
    const int width = size.width;
    const int height = size.height;

    int16_t eulerNumber;

    size_t nQ1 = 0, nQ3 = 0, nQD = 0;

    size_t index = 0;
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            uint8_t pixelCounter = 0;
            int8_t pixelLocation[2];
            for (uint8_t i = 0; i < 2; i++) { // Run through the 4x4 bit quad
                for (uint8_t j = 0; j < 2; j++) {
                    size_t subIndex = index + i * width + j;
                    if (subIndex < width * height) { // Prevent overflow in the bottom of the image
                        if ((bool)image->data[subIndex] == whitePixels) {
                            if (pixelCounter < 2)
                                pixelLocation[pixelCounter] = i + 2 * j; // 0,0 = 0; 1,0 = 1; 0,1 = 2; 1,1 = 3
                            pixelCounter++;
                        }
                    }
                }
            }
            if (pixelCounter == 1)
                nQ1++; // There is only 1 pixel in the bit quad
            else if (pixelCounter == 3)
                nQ3++; // There is 3 pixels in the bit quad
            else if (pixelCounter == 2) {
                if ((pixelLocation[0] == 0 && pixelLocation[1] == 3) || (pixelLocation[0] == 1 && pixelLocation[1] == 2))
                    nQD++; // The bit quad is diagonal
            }
            index++; // Increment index
        }
    }

    if (connected == CONNECTED_4) // 4-connected
        eulerNumber = (nQ1 - nQ3 + 2 * nQD) / 4;
    else // 8-connected
        eulerNumber = (nQ1 - nQ3 - 2 * nQD) / 4;

    return eulerNumber;
}
