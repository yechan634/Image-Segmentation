#include "graph.h"
#include "utilities.h"
#include "imgProcessing.h"

#include <opencv2/opencv.hpp>
#include <opencv2/saliency.hpp>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include <iostream>

#define SIGMA 5
#define SOURCE 1
#define SINK 2

// assume pixels are valid
weightType euclidDifference(cv::Vec3b pixel1, cv::Vec3b pixel2)
{
    // return sqrt((pixel1 - pixel2).dot(pixel1 - pixel2));
    int dx = static_cast<int>(pixel1[0]) - static_cast<int>(pixel2[0]); // avoiding dot imporives efficiency
    int dy = static_cast<int>(pixel1[1]) - static_cast<int>(pixel2[1]);
    int dz = static_cast<int>(pixel1[2]) - static_cast<int>(pixel2[2]);
    return sqrt(dx * dx + dy * dy + dz * dz);
}

// lower weight implies greater similarity
weightType calcWeightBetweenTwoPixels(cv::Vec3b pixel1, cv::Vec3b pixel2)
{
    auto e = euclidDifference(pixel1, pixel2);
    return exp(-1 * (euclidDifference(pixel1, pixel2)) / (2 * pow(SIGMA, 2)));
    // return 1;
}

// returns coordinates of node's corresponding pixel
std::pair<int, int> getNodeCoord(nodeType n, int width, int height)
{
    return std::make_pair(
        (n - SINK - 1) / height, // does integer division
        (n - SINK - 1) % width);
}

cv::Mat createMask(std::set<nodeType> nodesToKeep, int width, int height)
{
    cv::Mat mask = cv::Mat::zeros(height, width, CV_8UC1); // each value automatically uchar
    for (auto n : nodesToKeep)
    {
        auto coord = getNodeCoord(n, width, height);
        std::cout << std::to_string(coord.first) + " " + std::to_string(coord.second) + "node:" + std::to_string(n) + "\n";
        mask.at<uchar>(coord.first, coord.second) = 1; // WHY DOES IT WORK WITH 0 BUT NOT 1
    }
    return mask;
}

cv::Mat computeSaliency(const cv::Mat &img)
{
    // Create the saliency algorithm object
    cv::Ptr<cv::saliency::StaticSaliencySpectralResidual> saliencyAlg =
        cv::saliency::StaticSaliencySpectralResidual::create();

    // Compute saliency map
    cv::Mat saliencyMap;
    bool success = saliencyAlg->computeSaliency(img, saliencyMap);
    if (!success)
    {
        throw std::runtime_error("Saliency computation failed.");
    }

    // Convert saliency map to CV_64F and normalize to [0, 1]
    saliencyMap.convertTo(saliencyMap, CV_64F, 1.0 / 255.0);

    // Normalize to the range [0, 1] for better visualization
    cv::normalize(saliencyMap, saliencyMap, 0, 1, cv::NORM_MINMAX);

    return saliencyMap;
}

weightType getSourceWeightToPixel(int y, int x, cv::Mat saliencyMap)
{
    return static_cast<weightType>(saliencyMap.at<double>(y, x));
}

weightType getSinkWeightToPixel(int y, int x, cv::Mat saliencyMap)
{
    return (1 - static_cast<weightType>(saliencyMap.at<double>(y, x)));
}

cv::Mat getExtractedObject(const std::string &imgPath)
{
    cv::Mat img = cv::imread(imgPath, cv::IMREAD_COLOR);
    if (img.empty())
    {
        throw std::invalid_argument("Couldn't open or find image\n");
    }
    auto imgGraph = new Graph(img.rows * img.cols);
    //auto r = imgGraph -> createResidualGraph();
    nodeType source = 1;
    nodeType sink = 2;
    nodeType n = sink + 1;

    cv::Mat saliencyMap = computeSaliency(img);
    // adding all weights between pixels in 4-neighbourhood format
    for (int y = 0; y < img.rows; y++)
    {
        for (int x = 0; x < img.cols; x++)
        {
            auto currentPixel = img.at<cv::Vec3b>(y, x);
            if (x < img.cols - 1)
            {
                auto rightPixel = img.at<cv::Vec3b>(y, x + 1);
                imgGraph->addNewConnection(n, n + 1, calcWeightBetweenTwoPixels(currentPixel, rightPixel));
                //r->addNewConnection(n, n + 1, calcWeightBetweenTwoPixels(currentPixel, rightPixel), false);
            }
            if (x > 0)
            {
                auto leftPixel = img.at<cv::Vec3b>(y, x - 1);
                imgGraph->addNewConnection(n, n - 1, calcWeightBetweenTwoPixels(currentPixel, leftPixel));
                //r->addNewConnection(n, n - 1, calcWeightBetweenTwoPixels(currentPixel, leftPixel), false);
            }
            if (y < img.rows - 1)
            {
                auto belowPixel = img.at<cv::Vec3b>(y + 1, x);
                imgGraph->addNewConnection(n, n + img.cols, calcWeightBetweenTwoPixels(currentPixel, belowPixel));
                //r->addNewConnection(n, n + img.cols, calcWeightBetweenTwoPixels(currentPixel, belowPixel), false);
            }
            if (y > 0)
            {
                auto abovePixel = img.at<cv::Vec3b>(y - 1, x);
                imgGraph->addNewConnection(n, n - img.cols, calcWeightBetweenTwoPixels(currentPixel, abovePixel));
                //r->addNewConnection(n, n - img.cols, calcWeightBetweenTwoPixels(currentPixel, abovePixel), false);
            }
            imgGraph->addNewConnection(source, n, getSourceWeightToPixel(y, x, saliencyMap));
            imgGraph->addNewConnection(n, sink, getSinkWeightToPixel(y, x, saliencyMap));
            ++n;
        }
    }
    printf("finished making graph\n");
    auto r = imgGraph->createResidualGraph();
    auto nodesToKeep = r->getMinCut(source, sink);
    // source, sink nodes don't have corresponding pixels in image
    nodesToKeep.erase(source);
    nodesToKeep.erase(sink);

    printNodeSet(nodesToKeep);

    cv::Mat mask = createMask(nodesToKeep, img.cols, img.rows);
    cv::Mat output;
    cv::bitwise_and(img, img, output, mask);

    // clearing up
    delete imgGraph;
    delete r;

    return output;
}
