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

#include "detect.h"
#include <opencv/cv.hpp>

void write_image(const std::string& imageName, const std::vector<std::string>& data, std::ostream& newMetaFile) {
    const cv::Mat img = cv::imread("images/" + imageName + ".jpg");
    if (!img.data) return;
	const moldec md { img };

	md.write_images(newMetaFile, data, imageName);
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
