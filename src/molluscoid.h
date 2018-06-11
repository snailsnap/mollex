#pragma once

#include <string>
#include <vector>

#include <opencv/cv.h>

#include "common.h"

struct molluscoid {
	const contour cont;
	const cv::Mat image;
	const cv::RotatedRect bounding_rect;

	std::string get_color() const;
	double ratio() const;
	double angle() const;
	
	molluscoid(const contour& _cont, const cv::Mat& _image);
	~molluscoid();
};

