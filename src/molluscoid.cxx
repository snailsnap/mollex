#include <cmath>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>

#include <opencv/cv.hpp>

#include "tunables.h"

#include "molluscoid.h"

cv::RotatedRect normalize_rrect(cv::RotatedRect rr) {
	if (rr.size.height > rr.size.width) {
		rr.angle += 90;
		std::swap(rr.size.height, rr.size.width);
	}
	return rr;
}

double molluscoid::ratio() const {
	return bounding_rect.size.height/bounding_rect.size.width;
}
double molluscoid::angle() const {
	return (bounding_rect.angle * M_PI) / 180;
}

molluscoid::molluscoid(const contour& _cont, const cv::Mat& _image) :
	cont{_cont},
	image{_image},
	bounding_rect{normalize_rrect(cv::minAreaRect(cont))} {
}

molluscoid::~molluscoid() {}

