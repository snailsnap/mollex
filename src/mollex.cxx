#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>

#include <opencv/cv.hpp>

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
/*
 * Perform thresholding on the image.
 * The threshold is determined by considering the image's histogram.
 */
#define THRESHOLD
/*
 * Perform a single Gaussian blur prefiltering step.
 * NOTE: This is not known to noticeably improve contour detection
 * performance, however it is not known to harm either.
 */
#define PREFILTER_GAUSSIAN
/*
 * Perform just one morphological open/close operation using a
 * fixed structuring element.
 */
#define MORPH_SINGLE
/*
 * Perform n morphological open/close operations using structuring
 * elements of order [1;n).
 */
#define MORPH_MULTIPLE
/*
 * Number of open/close iterations to use with a fixed
 * structuring element.
 */
#define MORPH_SINGLE_OPEN_ITERATIONS 5
#define MORPH_SINGLE_CLOSE_ITERATIONS 0

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

cv::Mat threshold(const cv::Mat& in) {
    cv::Mat out, tmp = in.clone();

#ifdef THRESHOLD
    const double threshold = determine_threshold(in);
    std::cout << "threshold: " << threshold << std::endl;
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
#ifdef PREFILTER_GAUSSIAN
    cv::GaussianBlur(in, in, cv::Size { 5, 5 }, 10.0);
#endif
    cv::bilateralFilter(in, filtered, 9, 100, 100);
    cv::pyrMeanShiftFiltering(filtered, tmp, 3.0, 20.0, 5);

    tmp.convertTo(tmp, CV_32FC3, 1/255.0);
    cv::cvtColor(tmp, tmp, cv::COLOR_BGR2GRAY, 1);

    return tmp;
}

void process(const char* img_fname) {
    cv::namedWindow("in", cv::WINDOW_NORMAL | cv::WINDOW_GUI_NORMAL);
    cv::resizeWindow("in", 640, 480);
    cv::namedWindow("out", cv::WINDOW_NORMAL | cv::WINDOW_GUI_NORMAL);
    cv::resizeWindow("out", 640, 480);
    cv::namedWindow("eroded", cv::WINDOW_NORMAL | cv::WINDOW_GUI_NORMAL);
    cv::resizeWindow("eroded", 640, 480);

    const cv::Mat img = cv::imread(img_fname);
    const cv::Mat filtered = prefilter(img);
    cv::Mat thresholded = threshold(filtered);
    cv::imshow("out", thresholded);
   
    morphological_filtering(thresholded);
    cv::imshow("eroded", thresholded);

    const std::vector<std::vector<cv::Point2i>> contours {
        find_contours(thresholded)
    };
    const cv::Scalar red { 0, 0, 255 };
    cv::drawContours(img, contours, -1, red, 3);

    cv::imshow("in", img);
    cv::waitKey(0);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <image>\n", argv[0]);
    }

    process(argv[1]);

    return EXIT_SUCCESS;
}
