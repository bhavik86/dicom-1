#pragma once

#include "opencv2/opencv.hpp"
#include "GaussDeriv.h"

using namespace cv;

void stegerEdges(const Mat & img, Mat & edges, double sigma, double low, double high);//, int bright);
