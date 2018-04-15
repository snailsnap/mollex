#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>

#include <opencv/cv.hpp>
//#include <opencv/highgui.hpp>

bool decide(std::vector<cv::Point2i> cont) {
    const double area = cv::contourArea(cont);
    const cv::Rect brect = cv::boundingRect(cont);
    const double brect_area = brect.area();
    const double quotient = brect_area/area;
    std::cout << area << " b:" << brect_area << " " << quotient;
        if (cont.size() >= 5 && 0) {
            const cv::RotatedRect ell = cv::fitEllipse(cont);
            const double ell_area = ell.size.area();
            std::cout << " " << ell_area << " " << ell_area/area;
        }
        if (area < 1e5) {
            std::cout << " ignore" << std::endl;
            return true;
        }
        std::cout << std::endl;
    return quotient < 1.15;
}

void denoise(const cv::Mat& in, cv::Mat& out) {
    const cv::Mat se = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(6, 6));
    const cv::Point2i anchor { -1, -1 };

    cv::Mat tmp;
    cv::blur(in, tmp, cv::Size(3, 3));
    cv::threshold(tmp, tmp, 0.22, 1.0, cv::THRESH_TOZERO);
    tmp.convertTo(out, CV_8UC1, 255.0);
    cv::compare(out, 0, out, cv::CMP_GT);
}

void process(const char* img_fname) {
    const cv::Mat se = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(6, 6));
    const cv::Point2i anchor { -1, -1 };



    cv::namedWindow("in", cv::WINDOW_NORMAL | cv::WINDOW_GUI_NORMAL);
    cv::resizeWindow("in", 640, 480);
    cv::namedWindow("out", cv::WINDOW_NORMAL | cv::WINDOW_GUI_NORMAL);
    cv::resizeWindow("out", 640, 480);
    cv::namedWindow("eroded", cv::WINDOW_NORMAL | cv::WINDOW_GUI_NORMAL);
    cv::resizeWindow("eroded", 640, 480);

    cv::Mat tmp, tmp2, tmp3, img = cv::imread(img_fname);
    img.convertTo(tmp, CV_32FC3, 1/255.0);
    cv::cvtColor(tmp, tmp, cv::COLOR_RGB2GRAY, 1);

    denoise(tmp, tmp2);

    cv::morphologyEx(tmp2, tmp2, cv::MORPH_OPEN, se, anchor, 5);
    cv::morphologyEx(tmp2, tmp2, cv::MORPH_CLOSE, se, anchor, 7);

    std::vector<std::vector<cv::Point2i>> contours;
    cv::Mat t;
    cv::findContours(tmp2, contours, t, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
    contours.erase(std::remove_if(contours.begin(), contours.end(), decide), contours.end());
    std::cout << contours.size() << std::endl;

    const cv::Scalar red { 0, 0, 255 };
    cv::drawContours(img, contours, -1, red, 3);

    cv::imshow("in", img);
    cv::imshow("out", tmp);
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
