#include <cstdlib>
#include <iostream>
#include <vector>

#include <opencv/cv.hpp>
//#include <opencv/highgui.hpp>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <image>\n", argv[0]);
    }

    cv::namedWindow("in", cv::WINDOW_NORMAL | cv::WINDOW_GUI_NORMAL);
    cv::resizeWindow("in", 640, 480);
    cv::namedWindow("out", cv::WINDOW_NORMAL | cv::WINDOW_GUI_NORMAL);
    cv::resizeWindow("out", 640, 480);
    cv::namedWindow("eroded", cv::WINDOW_NORMAL | cv::WINDOW_GUI_NORMAL);
    cv::resizeWindow("eroded", 640, 480);

    cv::Mat tmp, tmp2, tmp3, img = cv::imread(argv[1]);
    img.convertTo(tmp, CV_32FC3, 1/255.0);
    cv::cvtColor(tmp, tmp, cv::COLOR_RGB2GRAY, 1);
    cv::blur(tmp, tmp, cv::Size(5, 5));
    cv::threshold(tmp, tmp, 0.26, 1.0, cv::THRESH_TOZERO);
    const cv::Mat se = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(6, 6));
    const cv::Point2i anchor { -1, -1 };
    tmp.convertTo(tmp2, CV_8UC1, 255.0);
    cv::compare(tmp2, 0, tmp2, cv::CMP_GT);

    cv::morphologyEx(tmp2, tmp2, cv::MORPH_OPEN, se, anchor, 5);
    cv::morphologyEx(tmp2, tmp2, cv::MORPH_CLOSE, se, anchor, 7);

    std::vector<std::vector<cv::Point2i>> contours;
    cv::Mat t;
    cv::findContours(tmp2, contours, t, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
    for (const auto& cont: contours) {
        for (const auto& pt: cont) {
            std::cout << pt;
        }
        std::cout << std::endl;
    }

    cv::imshow("in", img);
    cv::imshow("out", tmp);
    cv::imshow("eroded", tmp2);
    cv::waitKey(0);

    return EXIT_SUCCESS;
}
