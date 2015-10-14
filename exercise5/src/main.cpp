#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace std;
using namespace cv;

int main(int argc, char *argv[]) {
    static Mat leftImage = imread("../files/PIC1_L.png", IMREAD_GRAYSCALE);
    static Mat rightImage = imread("../files/PIC1_R.png", IMREAD_GRAYSCALE);
    //imshow("Left", leftImage);
    //imshow("Right", rightImage);

    const uint8_t windowSize = 20;
    const uint8_t pairOfPoints = 3;

    int left_xy[pairOfPoints][2];
    uint32_t min_m1[pairOfPoints];
    uint32_t min_m2[pairOfPoints];
    uint32_t min_m1_xy[pairOfPoints][2];
    uint32_t min_m2_xy[pairOfPoints][2];

    double timer = (double)getTickCount();

    static const uint16_t x0 = 320;
    static const uint16_t y0 = 20;

    for (uint8_t i = 0; i < pairOfPoints; i++) {
        min_m1[i] = min_m2[i] = ~0; // Set to maximum value

        left_xy[i][0] = x0 + i * rand() % 10; // Use a random point, so it actually works
        left_xy[i][1] = y0 + i * rand() % 10;
        Mat leftWindow = leftImage(Rect(left_xy[i][0], left_xy[i][1], windowSize, windowSize)).clone();

        for (size_t y = 0; y < rightImage.size().height - windowSize; y++) {
            for (size_t x = 0; x < rightImage.size().width - windowSize; x++) {
                Mat rightWindow = rightImage(Rect(x, y, windowSize, windowSize)).clone();

                uint32_t m1 = 0, m2 = 0;
                for (uint8_t windowY = 0; windowY < windowSize; windowY++) {
                    for (uint8_t windowX = 0; windowX < windowSize; windowX++) {
                        int16_t val = leftWindow.data[windowX + windowY * leftWindow.size().width] - rightWindow.data[windowX + windowY * rightWindow.size().width];
                        m1 += abs(val);
                        m2 += val * val;
                        if (m1 >= min_m1[i] && m2 >= min_m2[i])
                            goto break_out; // If both m1 and m2 are larger we can just break out of these loops
                    }
                }
                if (m1 < min_m1[i]) {
                    min_m1[i] = m1;
                    min_m1_xy[i][0] = x;
                    min_m1_xy[i][1] = y;
                }
                if (m2 < min_m2[i]) {
                    min_m2[i] = m2;
                    min_m2_xy[i][0] = x;
                    min_m2_xy[i][1] = y;
                }
                if (min_m1[i] == 0 || min_m2[i] == 0)
                    goto found;
break_out:
                m1 = 0; // Just added so it compiles
            }
        }

found:
        static Mat leftImageColor, rightImageColorM1, rightImageColorM2;
        if (leftImageColor.empty()) { // Convert images into 3-channel images, so we can draw in color
            cvtColor(leftImage, leftImageColor, COLOR_GRAY2BGR);
            cvtColor(rightImage, rightImageColorM1, COLOR_GRAY2BGR);
            cvtColor(rightImage, rightImageColorM2, COLOR_GRAY2BGR);
        }
        rectangle(leftImageColor, Rect(left_xy[i][0], left_xy[i][1], windowSize, windowSize), Scalar(0, 0, 255));
        imshow("Left color", leftImageColor);

        rectangle(rightImageColorM1, Rect(min_m1_xy[i][0], min_m1_xy[i][1], windowSize, windowSize), Scalar(0, 0, 255)); // Draw m1 in red
        rectangle(rightImageColorM2, Rect(min_m2_xy[i][0], min_m2_xy[i][1], windowSize, windowSize), Scalar(255, 0, 0)); // Draw m2 in blue
        imshow("Right color m1", rightImageColorM1);
        imshow("Right color m2", rightImageColorM2);

        /*printf("%d,%d\tPoint[%u] = m1: %u x,y: %u,%u\tm2: %u x,y: %u,%u\n", left_xy[i][0], left_xy[i][1], i,
                                                                            min_m1[i], min_m1_xy[i][0], min_m1_xy[i][1],
                                                                            min_m2[i], min_m2_xy[i][0], min_m2_xy[i][1]);*/
    }

    float x[3], y[3], u[3], v[3];
    for (uint8_t i = 0; i < pairOfPoints; i++) {
        // Convert to center and print equations
        x[i] = left_xy[i][0] - windowSize/2;
        y[i] = left_xy[i][1] - windowSize/2;
        u[i] = min_m2_xy[i][0] - windowSize/2;
        v[i] = min_m2_xy[i][1] - windowSize/2;
#if 0 // Used to check in Maple
        printf("x%u:=%.2f;\n", i, x[i]);
        printf("y%u:=%.2f;\n", i, y[i]);
        printf("u%u:=%.2f;\n", i, u[i]);
        printf("v%u:=%.2f;\n", i, v[i]);
#else
        printf("%.2f = p0 + p1 * %.2f + p2 * %.2f;\n", u[i], x[i], y[i]);
        printf("%.2f = q0 + q1 * %.2f + q2 * %.2f;\n", v[i], x[i], y[i]);
#endif
    }

    // Found using Maple
    const float p0 = (u[0]*x[1]*y[2]-u[0]*x[2]*y[1]-u[1]*x[0]*y[2]+u[1]*x[2]*y[0]+u[2]*x[0]*y[1]-u[2]*x[1]*y[0])/(x[0]*y[1]-x[0]*y[2]-x[1]*y[0]+x[1]*y[2]+x[2]*y[0]-x[2]*y[1]);
    const float p1 = (u[0]*y[1]-u[0]*y[2]-u[1]*y[0]+u[1]*y[2]+u[2]*y[0]-u[2]*y[1])/(x[0]*y[1]-x[0]*y[2]-x[1]*y[0]+x[1]*y[2]+x[2]*y[0]-x[2]*y[1]);
    const float p2 = -(u[0]*x[1]-u[0]*x[2]-u[1]*x[0]+u[1]*x[2]+u[2]*x[0]-u[2]*x[1])/(x[0]*y[1]-x[0]*y[2]-x[1]*y[0]+x[1]*y[2]+x[2]*y[0]-x[2]*y[1]);
    const float q0 = (v[0]*x[1]*y[2]-v[0]*x[2]*y[1]-v[1]*x[0]*y[2]+v[1]*x[2]*y[0]+v[2]*x[0]*y[1]-v[2]*x[1]*y[0])/(x[0]*y[1]-x[0]*y[2]-x[1]*y[0]+x[1]*y[2]+x[2]*y[0]-x[2]*y[1]);
    const float q1 = (v[0]*y[1]-v[0]*y[2]-v[1]*y[0]+v[1]*y[2]+v[2]*y[0]-v[2]*y[1])/(x[0]*y[1]-x[0]*y[2]-x[1]*y[0]+x[1]*y[2]+x[2]*y[0]-x[2]*y[1]);
    const float q2 = -(v[0]*x[1]-v[0]*x[2]-v[1]*x[0]+v[1]*x[2]+v[2]*x[0]-v[2]*x[1])/(x[0]*y[1]-x[0]*y[2]-x[1]*y[0]+x[1]*y[2]+x[2]*y[0]-x[2]*y[1]);

    printf("p = %.2f %.2f %.2f\n", p0, p1, p2);
    printf("q = %.2f %.2f %.2f\n", q0, q1, q2);

#if 1 // Check if they are calculated correctly
    for (uint8_t i = 0; i < pairOfPoints; i++) {
        assert((int)u[i] == (int)(p0 + p1 * x[i] + p2 * y[i]));
        assert((int)v[i] == (int)(q0 + q1 * x[i] + q2 * y[i]));
    }
#endif

    Mat transformedImage = Mat(leftImage.size(), leftImage.type());
    memset(transformedImage.data, 0, transformedImage.total()); // Set all pixels to black

    for (size_t y = 0; y < rightImage.size().height; y++) {
        for (size_t x = 0; x < rightImage.size().width; x++) {
            const int u = p0 + p1 * (float)x + p2 * (float)y;
            const int v = q0 + q1 * (float)x + q2 * (float)y;

            if (u >= 0 && u < transformedImage.size().width && v >= 0 && v < transformedImage.size().height)
                transformedImage.data[u + v * transformedImage.size().width] = leftImage.data[x + y * leftImage.size().width];
        }
    }

    cvtColor(transformedImage, transformedImage, COLOR_GRAY2BGR);
    for (uint8_t i = 0; i < pairOfPoints; i++)
        rectangle(transformedImage, Rect(min_m2_xy[i][0], min_m2_xy[i][1], windowSize, windowSize), Scalar(255, 0, 0)); // Draw m2 in blue on transformed image
    imshow("Transformed Image", transformedImage);

    printf("ms = %f\n", ((double)getTickCount() - timer) / getTickFrequency() * 1000.0);

    cvWaitKey(); // Wait until a key is pressed

    return 0;
}
