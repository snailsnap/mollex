#pragma once

#include <string>
#include <vector>
#include <fstream>

#include <opencv/cv.h>

using contour = std::vector<cv::Point2i>;

std::vector<cv::Mat> process(std::string imageName, std::string inDir);
std::string getColor(cv::Mat image);

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

class moldec {
	const cv::Mat origin;
	std::vector<contour> contours;
	cv::Mat thresholded;
	std::vector<molluscoid> molluscoids;

	void find_contours();
	void extract_molluscoids();
	static bool decide(const contour& cont);
	double determine_threshold(const cv::Mat&) const;
	void threshold(const cv::Mat&);

	std::string get_color(const cv::Mat& img) const;
public:
	moldec(const cv::Mat& img);
	~moldec();

	std::vector<contour> get_contours() const;

	void write_images(std::ostream& os, const std::vector<std::string>& data, const std::string& imageName) const;
};
