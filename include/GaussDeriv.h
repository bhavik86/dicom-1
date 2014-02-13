#pragma once

#include "opencv2/opencv.hpp"
#include "normal.h"

using namespace cv;

#define MAX_SIZE_MASK_0  3.09023230616781    /* Size for Gaussian mask */
#define MAX_SIZE_MASK_1  3.46087178201605    /* Size for 1st derivative mask */
#define MAX_SIZE_MASK_2  3.82922419517181    /* Size for 2nd derivative mask */
#define MASK_SIZE(MAX, sigma) ceil(MAX*sigma) /* Maximum mask index */

// 1/sqrt(2*PI)
#define SQRT_2_PI_INV 0.398942280401432677939946059935

// derivatives of gaussian
Mat gaussDeriv0(double sigma);
Mat gaussDeriv1(double sigma);
Mat gaussDeriv2(double sigma);
void gaussDerivs(Mat * G, double sigma);