#ifndef IMGSEGMODEL_H
#define IMGSEGMODEL_H

#include "Graph.h"
#include <memory>
#include <opencv2/opencv.hpp>
#include <opencv2/saliency.hpp>
#include <cassert>

namespace constants {
constexpr double SIGMA = 5;
constexpr int SALIENCY_THRESHOLD = 0.6;
} // namespace constants

class ImgSegModel {
public:
  ImgSegModel();
  ~ImgSegModel();

    cv::Mat getExtractedObject(const std::string &imgPath);

private:
  std::unique_ptr<Graph> getImgGraph(const cv::Mat &img);


  cv::Mat computeSaliency(const cv::Mat &img);
  
  cv::Mat getProbabilityMap(const cv::Mat &img);

  double euclidDifference(const cv::Vec3b &pixel1, const cv::Vec3b &pixel2);
  // lower weight implies greater similarity
  double calcWeightBetweenTwoPixels(cv::Vec3b pixel1, cv::Vec3b pixel2);


  std::pair<int, int> getNodeCoord(int n, int width, int height);

  cv::Mat createMask(std::unordered_set<int> &nodesToKeep, int width, int height);

};

#endif