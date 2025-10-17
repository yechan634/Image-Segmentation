#include "ImgSegModel.h"

ImgSegModel::ImgSegModel() {}

ImgSegModel::~ImgSegModel() {}

cv::Mat ImgSegModel::getExtractedObject(const std::string &imgPath) {
  cv::Mat img = cv::imread(imgPath, cv::IMREAD_COLOR);
  if (img.empty()) {
    throw std::invalid_argument("Couldn't open or find image\n");
  }
  auto imgGraph = getImgGraph(img);
  imgGraph->performRelabel(0, 1);
  std::unordered_set<int> nodesToKeep = imgGraph->getNodesOfSource();
  // Remove source and sink from nodesToKeep
  nodesToKeep.erase(0);
  nodesToKeep.erase(1);
  cv::Mat mask = createMask(nodesToKeep, img.cols, img.rows);
  cv::Mat output;
  cv::bitwise_and(img, img, output, mask);
  return output;
}

std::unique_ptr<Graph> ImgSegModel::getImgGraph(const cv::Mat &img) {
  assert(!img.empty());

  std::unique_ptr<Graph> imgGraph = std::make_unique<Graph>();
  int sourceNode = imgGraph->addNodeWithVal();
  int sinkNode = imgGraph->addNodeWithVal();

  std::cout << sourceNode << " " << sinkNode << std::endl;

  cv::Mat probabilityMap = getProbabilityMap(img);

  int n = sinkNode + 1;

  for (int i = 0; i < img.rows * img.cols; i++) {
    imgGraph->addNodeWithVal();
  }

  for (int y = 0; y < img.rows; y++) {
    for (int x = 0; x < img.cols; x++) {
      auto currentPixel = img.at<cv::Vec3b>(y, x);
      if (x < img.cols - 1) {
        auto rightPixel = img.at<cv::Vec3b>(y, x + 1);
        imgGraph->addEdgeFromTo(
            n, n + 1, calcWeightBetweenTwoPixels(currentPixel, rightPixel));
      }
      if (x > 0) {
        auto leftPixel = img.at<cv::Vec3b>(y, x - 1);
        imgGraph->addEdgeFromTo(
            n, n - 1, calcWeightBetweenTwoPixels(currentPixel, leftPixel));
      }
      if (y < img.rows - 1) {
        auto belowPixel = img.at<cv::Vec3b>(y + 1, x);
        imgGraph->addEdgeFromTo(
            n, n + img.cols,
            calcWeightBetweenTwoPixels(currentPixel, belowPixel));
      }
      if (y > 0) {
        auto abovePixel = img.at<cv::Vec3b>(y - 1, x);
        imgGraph->addEdgeFromTo(
            n, n - img.cols,
            calcWeightBetweenTwoPixels(currentPixel, abovePixel));
      }
    double p = static_cast<double>(probabilityMap.at<double>(y, x));
    imgGraph->addEdgeFromTo(sourceNode, n, p);
    imgGraph->addEdgeFromTo(n, sinkNode, 1.0 - p);
      ++n;
    }
  }

  return imgGraph;
}

cv::Mat ImgSegModel::computeSaliency(const cv::Mat &img) {
  cv::Ptr<cv::saliency::StaticSaliencySpectralResidual> saliencyAlg =
      cv::saliency::StaticSaliencySpectralResidual::create();
  cv::Mat saliencyMap;
  bool success = saliencyAlg->computeSaliency(img, saliencyMap);
  if (!success) {
    throw std::runtime_error("Saliency computation failed.");
  }
  saliencyMap.convertTo(saliencyMap, CV_64F, 1.0 / 255.0);
  cv::normalize(saliencyMap, saliencyMap, 0, 1, cv::NORM_MINMAX);
  return (1 - saliencyMap);
}

cv::Mat computeGMMmap(const cv::Mat &image, int numComponents = 5)
{
    // reshaping the image into a 2D matrix with each row as a pixel (in 3D color space)
    cv::Mat samples = image.reshape(1, image.rows * image.cols);
    samples.convertTo(samples, CV_64F); // Convert to double type for GMM

    cv::Ptr<cv::ml::EM> gmm = cv::ml::EM::create();
    gmm->setClustersNumber(numComponents);
    gmm->setCovarianceMatrixType(cv::ml::EM::COV_MAT_DIAGONAL);
    gmm->setTermCriteria(cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 100, 0.1));

    if (gmm->trainEM(samples) < 0)
    {
        std::cerr << "GMM training failed." << std::endl;
        return cv::Mat(); // Return an empty matrix if training fails
    }

    cv::Mat weights = gmm->getWeights();
    cv::Mat probabilityMap(image.rows, image.cols, CV_64F, cv::Scalar(0)); // Initialize to zero

    for (int row = 0; row < image.rows; ++row)
    {
        for (int col = 0; col < image.cols; ++col)
        {
            cv::Vec3d pixel = samples.at<cv::Vec3d>(row * image.cols + col);
            cv::Mat posterior;
            gmm->predict2(pixel, posterior);
            double foregroundProbability = 0.0;
            for (int i = 0; i < numComponents; ++i)
            {
                foregroundProbability += posterior.at<double>(i) * weights.at<double>(i);
            }
            probabilityMap.at<double>(row, col) = foregroundProbability;
        }
    }
    cv::normalize(probabilityMap, probabilityMap, 0, 1, cv::NORM_MINMAX);
    return (1 - probabilityMap);
}

cv::Mat ImgSegModel::getProbabilityMap(const cv::Mat &img) {
  // auto map = computeSaliency(img);
  // auto map = saliencyWithRegionGrowing(img);
  auto map = computeGMMmap(img, 3);
  // showProbabilityMap(map);
  return map;
}

double ImgSegModel::euclidDifference(const cv::Vec3b &pixel1,
                                     const cv::Vec3b &pixel2) {
  int dx = static_cast<int>(pixel1[0]) -
           static_cast<int>(pixel2[0]); // avoiding dot improves efficiency
  int dy = static_cast<int>(pixel1[1]) - static_cast<int>(pixel2[1]);
  int dz = static_cast<int>(pixel1[2]) - static_cast<int>(pixel2[2]);
  return sqrt(dx * dx + dy * dy + dz * dz);
}
// lower weight implies greater similarity
double ImgSegModel::calcWeightBetweenTwoPixels(cv::Vec3b pixel1,
                                               cv::Vec3b pixel2) {
  auto e = euclidDifference(pixel1, pixel2);
  return exp(-1 * (euclidDifference(pixel1, pixel2)) /
             (2 * pow(constants::SIGMA, 2)));
}

std::pair<int, int> ImgSegModel::getNodeCoord(int n, int width, int height) {
  return std::make_pair(
      static_cast<int>((n - 2) / height), // does integer division
      static_cast<int>((n - 2) % width));
}

cv::Mat ImgSegModel::createMask(std::unordered_set<int> &nodesToKeep, int width,
                                int height) {
  cv::Mat mask =
      cv::Mat::zeros(height, width, CV_8UC1); // each value automatically uchar
  for (auto n : nodesToKeep) {
    auto coord = getNodeCoord(n, width, height);
    // Only set mask if coordinates are in bounds
    if (coord.first >= 0 && coord.first < height && coord.second >= 0 &&
        coord.second < width) {
      mask.at<uchar>(coord.first, coord.second) = 255;
    }
  }

  std::cout << nodesToKeep.size() << " " << width * height << std::endl;
  return mask;
}
