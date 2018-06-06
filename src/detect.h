#pragma once

#include <string>
#include <vector>
#include <fstream>

#include <opencv/cv.h>

std::vector<cv::Mat> process(std::string imageName, std::string inDir);
std::string getColor(cv::Mat image);

