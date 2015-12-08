#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include "contours.h"
#include "histogram.h"
#include "misc.h"

using namespace std;
using namespace cv;

static void checkNeighbors(const Mat *image, int *pos, uint8_t *index, const int width, Connected connected, bool whitePixels) {
    const int contourArray[8] = {
            1,          // (x + 1, y)
            width + 1,  // (x + 1, y + 1)
            width,      // (x, y + 1)
            width - 1,  // (x - 1, y + 1)
            -1,         // (x - 1, y)
            -width - 1, // (x - 1, y - 1)
            -width,     // (x, y - 1)
            -width + 1, // (x + 1, y - 1)
    };
    *index = (*index + 6) % 8; // Select next search direction. Add 6, so we look just above the last pixel

    for (uint8_t i = *index; i < *index + 8;) { // Check 8 pixels around
        int32_t subIndex = *pos + contourArray[i % 8];
        if (subIndex >= 0 && subIndex < image->total()) {
            if ((bool)image->data[subIndex] == whitePixels) {
                *pos = subIndex;
                *index = i % 8;
                return;
            }
        }
        if (connected == CONNECTED_8)
            i++;
        else if (connected == CONNECTED_4)
            i += 2; // Skip all corners
        else { // When using 6-connected, I use the definition here: https://en.wikipedia.org/wiki/Pixel_connectivity#6-connected
            if (i % 8 != 2 && i % 8 != 6)
                i++;
            else
                i += 2; // Skip upper right and lower left corners
        }
    }
}

bool contoursSearch(const Mat *image, Mat *out, Connected connected, bool whitePixels) {
    assert(image->channels() == 1); // Image has to be one channel only

    *out = Mat(image->size(), image->type());
    memset(out->data, 0, out->total());

    const size_t total = out->total();
    const size_t MAX_RAND = total;
    const Size size = out->size();
    const int width = size.width;
    const int height = size.height;

    int pos = -1;
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            size_t index = x + y * width;
            if ((bool)image->data[index] == whitePixels) {
                pos = index;
                goto contourFound;
            }
        }
    }
    if (pos == -1) {
        printf("No contour detected!\n");
        return false;
    }

contourFound:
    size_t count = 0;
    int newpos = pos;
    uint8_t draw_type = 0;

    while (1) {
        out->data[newpos] = 255; // Draw contour
#if 1
        checkNeighbors(image, &newpos, &draw_type, width, connected, whitePixels);
#else
        draw_type = (draw_type + 6) % 8; // Select next search direction

        switch (draw_type) {
        case 0:
            if ((bool)image->data[newpos + 1] == whitePixels) {
                newpos += 1;
                draw_type = 0;
                break;
            }
            // Intentional fall through
        case 1:
            if ((bool)image->data[newpos + width + 1] == whitePixels) {
                newpos += width + 1;
                draw_type = 1;
                break;
            }
            // Intentional fall through
        case 2:
            if ((bool)image->data[newpos + width] == whitePixels) {
                newpos += width;
                draw_type = 2;
                break;
            }
            // Intentional fall through
        case 3:
            if ((bool)image->data[newpos + width - 1] == whitePixels) {
                newpos += width - 1;
                draw_type = 3;
                break;
            }
            // Intentional fall through
        case 4:
            if ((bool)image->data[newpos - 1] == whitePixels) {
                newpos -= 1;
                draw_type = 4;
                break;
            }
            // Intentional fall through
        case 5:
            if ((bool)image->data[newpos - width - 1] == whitePixels) {
                newpos -= width + 1;
                draw_type = 5;
                break;
            }
            // Intentional fall through
        case 6:
            if ((bool)image->data[newpos - width] == whitePixels) {
                newpos -= width;
                draw_type = 6;
                break;
            }
            // Intentional fall through
        case 7:
            if ((bool)image->data[newpos - width + 1] == whitePixels) {
                newpos -= width - 1;
                draw_type = 7;
                break;
            }
            // Intentional fall through
        case 8:
            if ((bool)image->data[newpos + 1] == whitePixels) {
                newpos += 1;
                draw_type = 0;
                break;
            }
            // Intentional fall through
        case 9:
            if ((bool)image->data[newpos + width + 1] == whitePixels) {
                newpos += width + 1;
                draw_type = 1;
                break;
            }
            // Intentional fall through
        case 10:
            if ((bool)image->data[newpos + width] == whitePixels) {
                newpos += width;
                draw_type = 2;
                break;
            }
            // Intentional fall through
        case 11:
            if ((bool)image->data[newpos + width - 1] == whitePixels) {
                newpos += width - 1;
                draw_type = 3;
                break;
            }
            // Intentional fall through
        case 12:
            if ((bool)image->data[newpos - 1] == whitePixels) {
                newpos -= 1;
                draw_type = 4;
                break;
            }
            // Intentional fall through
        case 13:
            if ((bool)image->data[newpos - width - 1] == whitePixels) {
                newpos -= width + 1;
                draw_type = 5;
                break;
            }
            // Intentional fall through
        case 14:
            if ((bool)image->data[newpos - width] == whitePixels) {
                newpos -= width;
                draw_type = 6;
                break;
            }
        }
#endif
        if (newpos == pos) { // If we are back at the beginning, we declare success
            //printf("Count: %lu\n", count);
            return true;
        } else if (++count >= MAX_RAND) { // Abort if the contour is too complex
            printf("Contour is too complex!\n");
            break;
        }
    }
    printf("Count: %lu\n", count);
    return false;
}
