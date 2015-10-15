#ifndef __contours_h__
#define __contours_h__

#include "misc.h"

using namespace cv;

bool contoursSearch(const Mat *image, Mat *out, Connected connected, bool whitePixels);

#endif
