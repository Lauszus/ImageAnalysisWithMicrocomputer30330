#ifndef __segmentation_h__
#define __segmentation_h__

using namespace cv;

Mat *getSegments(const Mat *image, uint8_t *nSegments, const int8_t neightborSize, bool whitePixels);
void releaseSegments(void);

#endif
