#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "segmentation.h"

using namespace cv;

static const uint8_t MAX_SEGMENTS = 10;
static Mat segmentImg[MAX_SEGMENTS];

static uchar checkNeighbors(const Mat *image, const Mat *segments, const size_t index, const int8_t neightborSize, bool whitePixels) {
    const Size size = image->size();
    const int width = size.width;
    const int height = size.height;

    for (int8_t i = -neightborSize; i <= neightborSize; i++) {
        for (int8_t j = -neightborSize; j <= neightborSize; j++) {
            int32_t subIndex = index + i * width + j;
            if (!(i == 0 && j == 0) && subIndex >= 0 && subIndex < width * height) { // Do not check x,y = (0,0) and prevent overflow in the bottom of the image
                if ((bool)image->data[subIndex] == whitePixels) {
                    if (segments->data[subIndex] > 0) // Part of existing segment
                        return segments->data[subIndex]; // Return id
                    else
                        return 255; // Part of a new segments
                }
            }
        }
    }
    return 0;
}

Mat *getSegments(const Mat *image, uint8_t *nSegments, const int8_t neightborSize, bool whitePixels) {
    assert(image->channels() == 1); // Picture must be black and white image

    const Size size = image->size();
    const int width = size.width;
    const int height = size.height;

    Mat segments(size, image->type());
    memset(segments.data, 0, image->total());

    size_t index = 0;
    *nSegments = 0;
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            if ((bool)image->data[index] == whitePixels) { // Check if we have found a pixel
                uchar id = checkNeighbors(image, &segments, index, neightborSize, whitePixels);
                if (id > 0) {
                    if (id == 255) // New segment
                        id = ++(*nSegments);
                    segments.data[index] = id; // Set same id as neighbor
                } else
                    segments.data[index] = 0; // No neighbors, just assume it is noise
            }
            index++; // Increment index
        }
    }

    if (*nSegments > 0) {
        if (*nSegments > MAX_SEGMENTS) {
            *nSegments = MAX_SEGMENTS; // Limit number of segments
            printf("Segment saturation!\n");
        }

        for (uint8_t i=0; i < *nSegments; i++) {
            if (segmentImg[i].total() == 0)
                segmentImg[i].create(size, segments.type()); // Recreate image
            memset(segmentImg[i].data, 0, segmentImg[i].total()); // Reset all data
        }

        for (uint8_t id = 1; id <= *nSegments; id++) {
            for (size_t j = 0; j < width * height; j++) {
                if (segments.data[j] == id)
                    segmentImg[id - 1].data[j] = 255;
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
