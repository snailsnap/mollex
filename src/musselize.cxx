#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>

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
#define BOXINESS_CUTOFF_LO 1.18d
#define BOXINESS_CUTOFF_HI 1.5d
/*
 * The minimum area occupied by a possible mollusc.
 */
#define MINIMUM_AREA 1e5
#define ENABLE_THRESHOLD 0
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

    return (max_idx - 8.0) / 64.0;
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
    const cv::Mat se { get_structuring_element(3) };
    const cv::Point2i anchor { -1, -1 };
#if 0
    for (int i = 0; i < 10; i++) {
        const cv::Mat se = get_structuring_element(i);
        cv::morphologyEx(tmp2, tmp2, cv::MORPH_OPEN, se, anchor, 1);
    }

    for (int i = 0; i < 10; i++) {
        const cv::Mat se = get_structuring_element(i);
        cv::morphologyEx(tmp2, tmp2, cv::MORPH_CLOSE, se, anchor, 1);
    }
#endif
    cv::morphologyEx(img, img, cv::MORPH_OPEN, se, anchor, 8);
    cv::morphologyEx(img, img, cv::MORPH_CLOSE, se, anchor, 2);
//  cv::morphologyEx(img, imt, cv::MORPH_GRADIENT, se, anchor, 1);
}

void process(const char* img_fname) {
    cv::namedWindow("in", cv::WINDOW_NORMAL | cv::WINDOW_GUI_NORMAL);
    cv::resizeWindow("in", 640, 480);
    cv::namedWindow("out", cv::WINDOW_NORMAL | cv::WINDOW_GUI_NORMAL);
    cv::resizeWindow("out", 640, 480);
    cv::namedWindow("eroded", cv::WINDOW_NORMAL | cv::WINDOW_GUI_NORMAL);
    cv::resizeWindow("eroded", 640, 480);

    cv::Mat tmp, tmp2, tmp3, img = cv::imread(img_fname);
    cv::Mat hsv[3];
    img.convertTo(tmp, CV_32FC3, 1/255.0);
    cv::cvtColor(tmp, tmp, cv::COLOR_BGR2HSV, 3);
    cv::split(tmp, hsv);

    threshold(hsv[1], tmp2);
    cv::imshow("out", tmp2);
//  cv::Canny(tmp2, tmp2, 25, 40);

    morphological_filtering(tmp2);

    std::vector<std::vector<cv::Point2i>> contours { find_contours(tmp2) };

    const cv::Scalar red { 0, 0, 255 };
    cv::drawContours(img, contours, -1, red, 3);

    cv::imshow("in", img);
    cv::imshow("eroded", tmp2);
    cv::waitKey(0);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <image>\n", argv[0]);
    }

    process(argv[1]);

    return EXIT_SUCCESS;
}
