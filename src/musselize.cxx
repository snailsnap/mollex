#include <cstdlib>
#include <iostream>

#include <opencv/cv.hpp>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <image>", argv[0]);
    }
    cv::Mat img = cv::imread(argv[1]);

    return EXIT_SUCCESS;
}
