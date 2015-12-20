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

#include "segmentation.h"

using namespace cv;

static const uint8_t MAX_SEGMENTS = 10;
static Mat segmentImg[MAX_SEGMENTS];

static uint8_t checkNeighbours(const Mat *image, const Mat *segments, const size_t index, const int8_t neighbourSize, bool whitePixels) {
    const Size size = image->size();
    const int width = size.width;
    const size_t total = image->total();

    uint8_t id = 0;

    // Check all neighbors using 8-connectedness
    for (int8_t i = -neighbourSize; i <= neighbourSize; i++) {
        for (int8_t j = -neighbourSize; j <= neighbourSize; j++) {
            int32_t subIndex = index + i * width + j;
            if (!(i == 0 && j == 0) && subIndex >= 0 && subIndex < total) { // Do not check x,y = (0,0) and prevent overflow
                if ((bool)image->data[subIndex] == whitePixels) {
                    if (segments->data[subIndex] > 0) // Part of an existing segment
                        return segments->data[subIndex]; // Return ID
                    else
                        id = 255; // Part of a new segments, but keep looping as there might be a neighbor with an existing ID
                }
            }
        }
    }
    return id;
}

Mat *getSegments(const Mat *image, uint8_t *nSegments, const int8_t neighbourSize, bool whitePixels) {
    assert(image->channels() == 1); // Picture must be a binary image

    const Size size = image->size();
    const int width = size.width;
    const int height = size.height;

    Mat segments(size, image->type()); // This Mat variable is used to store the ID of the different segments
    memset(segments.data, 0, image->total());

    // Look for segments
    size_t index = 0;
    *nSegments = 0;
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            if ((bool)image->data[index] == whitePixels) { // Check if we have found a pixel
                uint8_t id = checkNeighbours(image, &segments, index, neighbourSize, whitePixels);
                if (id > 0) {
                    if (id == 255) // New segment
                        id = ++(*nSegments); // Set ID equal to the current number of found segments
                    segments.data[index] = id; // Set same ID as neighbor
                } else
                    segments.data[index] = 0; // No neighbors, just assume it is noise
            }
            index++; // Increment index
        }
    }

    // Create an image for each segment
    if (*nSegments > 0) {
        if (*nSegments > MAX_SEGMENTS) {
            *nSegments = MAX_SEGMENTS; // Limit number of segments
            printf("Segment saturation!\n");
        }

        for (uint8_t i=0; i < *nSegments; i++) {
            segmentImg[i].create(size, segments.type()); // Recreate image
            memset(segmentImg[i].data, 0, segmentImg[i].total()); // Reset all data
        }

        for (uint8_t id = 1; id <= *nSegments; id++) {
            for (size_t j = 0; j < width * height; j++) {
                if (segments.data[j] == id)
                    segmentImg[id - 1].data[j] = 255; // Draw white on each segment
            }
#if 0
            char buf[20];
            sprintf(buf, "Segment %u", id);
            imshow(buf, segmentImg[id - 1]);
#endif
        }

        //printf("Segments: %u\n", *nSegments);
        return segmentImg;
    }
    return NULL;
}

void releaseSegments(void) {
    for (uint8_t i = 0; i < MAX_SEGMENTS; i++)
        segmentImg[i].release();
}
