#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>

#include <opencv/cv.hpp>
//#include <opencv/highgui.hpp>

// ### TUNABLES ###
/*
 * Boxiness is the area of the fitted rectangle divided through
 * the area of the actual contour.
 * A low boxiness value (near 1.0) indicates a very box-like shape.
 * A high boxiness value (greater than 2.0) indicates a very chaotic
 * shape that we are not interested in.
 */
#define BOXINESS_CUTOFF_LO 1.18
#define BOXINESS_CUTOFF_HI 1.8
/*
 * The minimum area occupied by a possible mollusc.
 */
#define MINIMUM_AREA 1e5

#define ENABLE_THRESHOLD 1
#define MORPH_SINGLE
#define MORPH_MULTIPLE

bool decide(std::vector<cv::Point2i> cont) {
    const double area = cv::contourArea(cont);
    const cv::RotatedRect brect = cv::minAreaRect(cont);
    const double brect_area = brect.size.area();
    const double quotient = brect_area/area;
    if (area < MINIMUM_AREA) {
        return true;
    }
    std::cout << area << " b:" << brect_area << " " << quotient;
#if 0
    if (cont.size() >= 5 && 0) {
        const cv::RotatedRect ell = cv::fitEllipse(cont);
        const double ell_area = ell.size.area();
        std::cout << " " << ell_area << " " << ell_area/area;
    }
#endif
    std::cout << std::endl;
    return quotient < BOXINESS_CUTOFF_LO ||
           quotient > BOXINESS_CUTOFF_HI;
}

std::vector<std::vector<cv::Point2i>> find_contours(const cv::Mat& in) {
    std::vector<std::vector<cv::Point2i>> contours;
    cv::findContours(in, contours, cv::RETR_LIST, cv::CHAIN_APPROX_TC89_L1);
    contours.erase(std::remove_if(contours.begin(), contours.end(), decide), contours.end());
    std::cout << "Found " << contours.size() << " potential molluscs" << std::endl;

    return contours;
}

double determine_threshold(const cv::Mat& in) {
    cv::Mat blurred, hist;
 
    cv::GaussianBlur(in, blurred, cv::Size(3, 3), 0);
    cv::pyrDown(blurred, blurred);
    cv::calcHist(std::vector<cv::Mat>{blurred}, std::vector<int>{0},
        cv::Mat(), hist, std::vector<int>{64}, std::vector<float>{0.0, 1.0});

    int max_idx = 0, max = 0;
    for (int i = 0; i < 32; i++) {
        if (hist.at<float>(i) > max) {
            max_idx = i;
            max = hist.at<float>(i);
        }
    }

    return std::max(0.0, (max_idx / 64.0) + 0.06);
}

void threshold(const cv::Mat& in, cv::Mat& out) {
    cv::Mat tmp = in.clone();
    const double threshold = determine_threshold(in);
    std::cout << "threshold: " << threshold << std::endl;
    cv::threshold(tmp, tmp, ENABLE_THRESHOLD ? threshold : 0, 1.0, cv::THRESH_TOZERO);
    tmp.convertTo(out, CV_8UC1, 255.0);
    cv::compare(out, 0, out, cv::CMP_GT);
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

        cv::morphologyEx(img, img, cv::MORPH_OPEN, se, anchor, 5);
        //cv::morphologyEx(img, img, cv::MORPH_CLOSE, se, anchor, 10);
        //cv::morphologyEx(img, img, cv::MORPH_GRADIENT, se, anchor, 10);
    }
#endif
}

void process(std::string img_fname, std::string inDir, std::string outDir) {
	auto ss = std::stringstream(img_fname);
	std::string imageName;
	std::getline(ss, imageName);
    const cv::Mat img = cv::imread(inDir + "/" + img_fname);
    cv::Mat tmp, tmp2, filtered;
    cv::Mat hsv[3];
    cv::bilateralFilter(img, filtered, 9, 100, 100);
    cv::pyrMeanShiftFiltering(filtered, tmp, 3.0, 20.0, 5);

    tmp.convertTo(tmp, CV_32FC3, 1/255.0);
    cv::cvtColor(tmp, tmp, cv::COLOR_BGR2GRAY, 1);

    threshold(tmp, tmp2);
    //cv::imshow("out", tmp2);
//  cv::Canny(tmp2, tmp2, 25, 40);
    morphological_filtering(tmp2);

    std::vector<std::vector<cv::Point2i>> contours { find_contours(tmp2) };
    const cv::Scalar red { 0, 0, 255 };

	std::vector<cv::Mat> channels(3);
	cv::split(img, channels);
	channels.push_back(tmp2);
	std::cout << "layer count: " << channels.size() << std::endl;

	cv::Mat outImage;
	cv::merge(channels, outImage);

	int i = 0;
	for (auto contour : contours) {
		auto boundingBox = cv::boundingRect(contour);
		std::cout << "rect: " << boundingBox << std::endl;
		auto segment = cv::Mat(outImage, boundingBox).clone();
		cv::imwrite(outDir + "/" + imageName + "_" + std::to_string(i++)  + ".png", segment);
	}

	//cv::imwrite(outDir + "/" + img_fname + ".png", outImage);
    cv::drawContours(img, contours, -1, red, 3);
    cv::imshow("in", img);
    //cv::imshow("eroded", tmp2);
    cv::waitKey(1000);
}

int main(int argc, char **argv) {
	cv::namedWindow("in", cv::WINDOW_NORMAL | cv::WINDOW_GUI_NORMAL);
	cv::resizeWindow("in", 640, 480);
	/*cv::namedWindow("out", cv::WINDOW_NORMAL | cv::WINDOW_GUI_NORMAL);
	cv::resizeWindow("out", 640, 480);
	cv::namedWindow("eroded", cv::WINDOW_NORMAL | cv::WINDOW_GUI_NORMAL);
	cv::resizeWindow("eroded", 640, 480);/**/

	std::ifstream oldMetaFile("species_list.csv");
	std::string line;
	if (oldMetaFile.is_open()) {
		int lineNr = 1;
		std::getline(oldMetaFile, line);
		while (std::getline(oldMetaFile, line) && lineNr == 1) {
			std::cout << "line " << lineNr << std::endl;
			std::vector<std::string> data;
			std::stringstream ss(line);
			std::string entry;
			for (auto i = (size_t)0; i < 13; ++i) {
				std::getline(ss, entry, ';');
				data.push_back(entry);
				//std::cout << entry << "|";
			}
			std::getline(ss, entry, ';');//Hackathon download Link

			//read image names
			while (std::getline(ss, entry, ';')) {
				if (entry != "") {
					//std::cout << "image path: <images/" << entry << ">" << std::endl;
					process(entry, "images", "outImages");
				}
			}
			lineNr++;
		}
	}




    /*if (argc != 2) {
        fprintf(stderr, "usage: %s <image>\n", argv[0]);
    }/**/

    //process(argv[1]);

    return EXIT_SUCCESS;
}
