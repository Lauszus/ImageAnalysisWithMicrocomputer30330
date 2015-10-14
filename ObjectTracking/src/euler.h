#ifndef __euler_h__
#define __euler_h__

#include "misc.h"

using namespace cv;

int16_t calculateEulerNumber(const Mat *image, const Connected connected, bool whitePixels);

#endif
