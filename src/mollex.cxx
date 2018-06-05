#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>

#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>

#include <opencv/cv.hpp>

#include "tunables.h"

using contour = std::vector<cv::Point2i>;

const int32_t downsampling_width = 256;

#if !(__cplusplus >= 201703L)
namespace std {
template<typename T>
constexpr const T& clamp(const T& val, const T& min, const T& max) {
    return std::min(max, std::max(min, val));
}
}
#endif

bool decide(const contour& cont) {
    const double area = cv::contourArea(cont);
    const cv::RotatedRect brect = cv::minAreaRect(cont);
    const double brect_area = brect.size.area();
    const double quotient = brect_area/area;
    if (area < MINIMUM_AREA) {
        return true;
    }
#ifdef DEBUG
    std::cout << area << " b:" << brect_area << " " << quotient << std::endl;
#endif

    if (quotient < BOXINESS_CUTOFF_LO ||
        quotient > BOXINESS_CUTOFF_HI) {
        return true;
    }
    {
        contour convex;
        cv::convexHull(cont, convex);
        const double convex_area = cv::contourArea(convex);
#ifdef DEBUG
        std::cout << "convex: " << convex_area << " q:"
                  << convex_area/area << std::endl;
#endif
        if (convex_area/area > CONTOUR_SMOOTHNESS_CUTOFF) {
            return true;
        }
    }

    return false;
}

std::vector<contour> find_contours(const cv::Mat& in) {
    std::vector<contour> contours;
    cv::findContours(in, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
    contours.erase(
        std::remove_if(contours.begin(), contours.end(), decide),
        contours.end()
    );

#ifdef DEBUG
    std::cout << "Found " << contours.size()
              << " potential molluscs" << std::endl;
#endif

    return contours;
}

double determine_threshold(const cv::Mat& img) {
    cv::Mat hist, blurred;

    assert(HIST_BUCKETS <= 8);
    const int buckets = 1 << HIST_BUCKETS,
              half_buckets = (1 << (HIST_BUCKETS - 1));
    cv::GaussianBlur(img, blurred, cv::Size(5, 5), 20.0);
    cv::pyrDown(blurred, blurred);
    cv::calcHist(std::vector<cv::Mat>{ blurred }, std::vector<int>{ 0 },
        cv::Mat(), hist, std::vector<int>{ buckets },
        std::vector<float>{ 0.0, 1.0 }
    );  

    int max_idx = 0, max = 0;
    for (int i = 0; i < half_buckets; i++) {
        if (hist.at<float>(i) > max) {
            max_idx = i;
            max = hist.at<float>(i);
        }
    }

    return std::clamp((max_idx / (double)buckets) + THRESHOLD_BIAS,
                      0.0, 1.0);
}

cv::Mat threshold(const cv::Mat& in) {
    cv::Mat out, tmp = in.clone();

#ifdef THRESHOLD
    const double threshold = determine_threshold(in);
#ifdef DEBUG
    std::cout << "threshold: " << threshold << std::endl;
#endif
    cv::threshold(tmp, tmp, threshold, 1.0, cv::THRESH_TOZERO);
#endif
    tmp.convertTo(out, CV_8UC1, 255.0);
    cv::compare(out, 0, out, cv::CMP_GT);

    return out;
}

cv::Mat get_structuring_element(const int order) {
    const int ksize = (order << 1) + 1;
    return cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(ksize, ksize));
}

void morphological_filtering(cv::Mat& img) {
    const cv::Point2i anchor { -1, -1 };
#ifdef MOPRH_MULTIPLE
    for (int i = 0; i < 10; i++) {
        const cv::Mat se = get_structuring_element(i);
        cv::morphologyEx(img, img, cv::MORPH_OPEN, se, anchor, 1);
    }

    for (int i = 0; i < 10; i++) {
        const cv::Mat se = get_structuring_element(i);
        cv::morphologyEx(img, img, cv::MORPH_CLOSE, se, anchor, 1);
    }
#endif

#ifdef MORPH_SINGLE
    {
        const cv::Mat se { get_structuring_element(5) };

        cv::morphologyEx(img, img, cv::MORPH_OPEN, se, anchor,
                         MORPH_SINGLE_OPEN_ITERATIONS);
        cv::morphologyEx(img, img, cv::MORPH_CLOSE, se, anchor,
                         MORPH_SINGLE_CLOSE_ITERATIONS);
    }
#endif
}

cv::Mat prefilter(cv::Mat in) {
    cv::Mat tmp, filtered;

    cv::bilateralFilter(in, filtered, 9, 100, 100);
#ifdef PREFILTER_GAUSSIAN
    cv::GaussianBlur(filtered, filtered, cv::Size { 5, 5 }, 10.0);
#endif
    cv::pyrMeanShiftFiltering(filtered, tmp, 3.0, 20.0, 5);

    tmp.convertTo(tmp, CV_32FC3, 1/255.0);
    cv::cvtColor(tmp, tmp, cv::COLOR_BGR2GRAY, 1);

	return tmp;
}

std::vector<contour> process_image(const cv::Mat& img) {
    const cv::Mat filtered = prefilter(img);
    cv::Mat thresholded = threshold(filtered);
   
    morphological_filtering(thresholded);
	return find_contours(thresholded);
}

const cv::Scalar black { 0, 0, 0 };
const cv::Scalar white { 255, 255, 255 };
const cv::Scalar red { 0, 0, 255 };

std::vector<cv::Mat> process(std::string imageName, std::string inDir) {
    std::vector<cv::Mat> images;

    const cv::Mat img = cv::imread(inDir + "/" + imageName + ".jpg");
    if (!img.data) return images;
    const std::vector<contour> contours { process_image(img) };
	std::vector<cv::Mat> channels(4);
	cv::split(img, channels);
	for (int i = 0; i < contours.size(); i++) {
		const auto boundingBox = cv::boundingRect(contours[i]);
		cv::Mat contour_alpha { img.size(), CV_8UC1, cv::Scalar { 0 } };
		cv::drawContours(contour_alpha, contours, i, white, cv::FILLED);
		channels.resize(3);
		channels.push_back(contour_alpha);
		cv::Mat outImage;
		cv::merge(channels, outImage);
		auto segment = cv::Mat(outImage, boundingBox).clone();
		const auto scale_factor = (boundingBox.size().width >= downsampling_width) ? downsampling_width/(double)boundingBox.size().width : 1.0;
		cv::resize(segment, segment, cv::Size{}, scale_factor, scale_factor, cv::INTER_AREA);
		images.push_back(segment);
	}
	return images;
}

std::string getColor(cv::Mat image) {
	cv::MatIterator_<cv::Vec4b> start, end;
	auto r = 0.0;
	auto g = 0.0;
	auto b = 0.0;
	auto a = 0.0;
	for (auto it = image.begin<cv::Vec4b>(), end = image.end<cv::Vec4b>(); it != end; ++it)
	{
		b += (*it)[3] * (*it)[0];
		g += (*it)[3] * (*it)[1];
		r += (*it)[3] * (*it)[2];
		a += (*it)[3];
	}
	int ri = r / a;
	int gi = g / a;
	int bi = b / a;
	auto ss = std::stringstream();
	ss << std::hex << (ri<=0xf?"0":"") << ri << (gi <= 0xf ? "0" : "") << gi << (bi <= 0xf ? "0" : "") << bi;
	return ss.str();
}

void write_image(const std::string& imageName, const std::vector<std::string>& data, std::ostream& newMetaFile) {
	const auto images = process(imageName, "images");
	int i = 0;
	for (auto image : images) {
		const std::string fname { imageName + "_" + std::to_string(i) + ".png" };
		cv::imwrite("data/" + fname, image);
		newMetaFile << fname << ";";
		newMetaFile << "#" << getColor(image) << ";";
		newMetaFile << "0.0;";
		newMetaFile << "1.0;";
		newMetaFile << imageName << ".jpg";
			
		for (auto d : data) {
			newMetaFile << ";" << d;
		}
		newMetaFile << std::endl;

		i++;
	}
}

void process_line(const std::string& line, std::ostream& newMetaFile) {
	std::vector<std::string> data;
	std::stringstream ss(line);
	std::string entry;
	for (auto i = (size_t)0; i < 13; ++i) {
		std::getline(ss, entry, ';');
		data.push_back(entry);
	}
	std::getline(ss, entry, ';');//Hackathon download Link

	//read image names
	while (std::getline(ss, entry, ';')) {
		if (entry != "") {
			auto ss1 = std::stringstream(entry);
			std::string imageName;
			std::getline(ss1, imageName, '.');
			write_image(imageName, data, newMetaFile);
		}
	}
}

int main(int argc, char **argv) {
	/*cv::namedWindow("in", cv::WINDOW_NORMAL | cv::WINDOW_GUI_NORMAL);
	cv::resizeWindow("in", 640, 480);/**/

	std::ifstream oldMetaFile("species_list.csv");	
	std::string line;
    if (!oldMetaFile.is_open()) {
        std::cerr << "Can not open species_list.csv" << std::endl;
        return EXIT_FAILURE;
    }

	std::ofstream newMetaFile("data/meta_file.csv");
    if (!newMetaFile.is_open()) {
        std::cerr << "Can not create data/meta_file.csv" << std::endl;
        return EXIT_FAILURE;
    }

    newMetaFile << "Image;Color;Rotation;Ratio;OriginalImage;InventarNr;Class;Family;Genus;Species;Scientific Name;Fundort;Datum;Gebiet;Provinz;Land;Teilkontinent;Kontinent" << std::endl;

	int lineNr = 1;
	std::getline(oldMetaFile, line);

	std::vector<std::string> lines;
	while (std::getline(oldMetaFile, line)) {
		lines.emplace_back(line);
	}

    std::atomic<int> lines_processed { 0 };

    const auto print_progress = [&] {
        while (lines_processed != lines.size()) {
            std::cout
                << lines_processed << "/" << lines.size() 
                << " " << lines_processed/(double)lines.size() * 100 << "%"
                << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds{ 2 });
        }
    };
    std::thread progress_thread{ print_progress };

    std::mutex mtx;
	#pragma omp parallel for
    for (int i = 0; i < lines.size(); i++) {   
        if (0) {
            std::lock_guard<std::mutex> guard{ mtx };
            std::cout << "line " << lineNr << std::endl;
            lineNr++;
        }
        process_line(lines[i], newMetaFile);
        lines_processed += 1;
	}

    progress_thread.join();
	oldMetaFile.close();
	newMetaFile.close();

    return EXIT_SUCCESS;
}
