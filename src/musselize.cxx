#include <cstdlib>
#include <iostream>

#include <opencv/cv.hpp>
//#include <opencv/highgui.hpp>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <image>\n", argv[0]);
    }

    cv::namedWindow("out", cv::WINDOW_NORMAL | cv::WINDOW_GUI_NORMAL);
    cv::resizeWindow("out", 640, 480);

    cv::Mat tmp, img = cv::imread(argv[1]);
    cv::compare(img, 128, tmp, cv::CMP_GE);

    cv::imshow("out", tmp);
    cv::waitKey(0);

    return EXIT_SUCCESS;
}
