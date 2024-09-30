#ifndef IMG_PROCESSING_H
#define IMG_PROCESSING_H
#include "graph.h"

#include <opencv2/opencv.hpp>

weightType euclidDifference(cv::Vec3b pixel1, cv::Vec3b pixel2);

weightType calcWeightBetweenTwoPixels(cv::Vec3b pixel1, cv::Vec3b pixel2);

std::pair<int, int> getNodeCoord(nodeType n, int width, int height);

cv::Mat createMask(std::set<nodeType> nodesToKeep, int width, int height) ;

template<typename T>
void printPixel(T pixel) {
    std::cout << "( " << 
    std::to_string(pixel[0]) + ", " + std::to_string(pixel[0]) + ", " + std::to_string(pixel[2]) <<
            ") ";
}

cv::Mat computeSaliency(const cv::Mat& img);

weightType getSourceWeightToPixel(int y, int x, cv::Mat saliencyMap);

weightType getSinkWeightToPixel(int y, int x, cv::Mat saliencyMap);

// coloured images
cv::Mat getExtractedObject(const std::string& imgPath);
#endif
