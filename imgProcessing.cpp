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
#define SALIENCY_THRESHOLD 0.6

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

cv::Mat computeSaliency(cv::Mat img)
{
    cv::Ptr<cv::saliency::StaticSaliencySpectralResidual> saliencyAlg =
        cv::saliency::StaticSaliencySpectralResidual::create();
    cv::Mat saliencyMap;
    bool success = saliencyAlg->computeSaliency(img, saliencyMap);
    if (!success)
    {
        throw std::runtime_error("Saliency computation failed.");
    }
    saliencyMap.convertTo(saliencyMap, CV_64F, 1.0 / 255.0);
    cv::normalize(saliencyMap, saliencyMap, 0, 1, cv::NORM_MINMAX);
    return (1 - saliencyMap);
}

void printProbabilityMap(cv::Mat probabilityMap)
{
    for (int row = 0; row < probabilityMap.rows; ++row)
    {
        for (int col = 0; col < probabilityMap.cols; ++col)
        {
            printf("%f ", probabilityMap.at<double>(row, col));
        }
        printf("\n");
    }
}

void showProbabilityMap(cv::Mat probabilityMap)
{
    cv::namedWindow("Grayscale probabilities", cv::WINDOW_NORMAL);
    cv::resizeWindow("Grayscale probabilities", 800, 800);
    cv::Mat grayscale;
    probabilityMap.convertTo(grayscale, CV_8U, 255);
    cv::imshow("Grayscale probabilities", grayscale);
    cv::waitKey(0);
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

cv::Mat saliencyWithRegionGrowing(cv::Mat inputImage)
{
    cv::Ptr<cv::saliency::StaticSaliencyFineGrained> saliencyAlgorithm = cv::saliency::StaticSaliencyFineGrained::create();
    cv::Mat saliencyMap;
    saliencyAlgorithm->computeSaliency(inputImage, saliencyMap);

    cv::normalize(saliencyMap, saliencyMap, 0, 1, cv::NORM_MINMAX);

    // thresholding saliency map to get foreground estimation
    cv::Mat initialForeground;
    cv::threshold(saliencyMap, initialForeground, SALIENCY_THRESHOLD, 1.0, cv::THRESH_BINARY);
    initialForeground.convertTo(initialForeground, CV_8U, 255);

    // distance transform to propagate probabilities from boundaries
    cv::Mat distForeground;
    cv::distanceTransform(initialForeground, distForeground, cv::DIST_L2, 5);
    cv::normalize(distForeground, distForeground, 0, 1.0, cv::NORM_MINMAX);

    // combining saliency and distance transform
    cv::Mat foregroundProb = 1 - (saliencyMap.mul(0.5) + distForeground.mul(0.5));
    return foregroundProb;
}

// returns map of probabilies that each pixel belongs to foreground
cv::Mat getProbabilityMap(cv::Mat img)
{
    auto map = computeSaliency(img);
    // auto map = saliencyWithRegionGrowing(img);
    // auto map = computeGMMmap(img, 3);
    showProbabilityMap(map);
    return map;
}

weightType getSourceWeightToPixel(int y, int x, cv::Mat probabilityMap)
{
    return static_cast<weightType>(probabilityMap.at<double>(y, x));
}

weightType getSinkWeightToPixel(int y, int x, cv::Mat probabilityMap)
{
    return (1 - static_cast<weightType>(probabilityMap.at<double>(y, x)));
}

cv::Mat getExtractedObject(const std::string &imgPath)
{
    cv::Mat img = cv::imread(imgPath, cv::IMREAD_COLOR);
    if (img.empty())
    {
        throw std::invalid_argument("Couldn't open or find image\n");
    }
    auto imgGraph = new Graph(img.rows * img.cols);
    nodeType source = 1;
    nodeType sink = 2;
    nodeType n = sink + 1;

    cv::Mat probabilityMap = getProbabilityMap(img);
    // printProbabilityMap(probabilityMap);
    //  adding all weights between pixels in 4-neighbourhood format
    for (int y = 0; y < img.rows; y++)
    {
        for (int x = 0; x < img.cols; x++)
        {
            auto currentPixel = img.at<cv::Vec3b>(y, x);
            if (x < img.cols - 1)
            {
                auto rightPixel = img.at<cv::Vec3b>(y, x + 1);
                imgGraph->addNewConnection(n, n + 1, calcWeightBetweenTwoPixels(currentPixel, rightPixel));
            }
            if (x > 0)
            {
                auto leftPixel = img.at<cv::Vec3b>(y, x - 1);
                imgGraph->addNewConnection(n, n - 1, calcWeightBetweenTwoPixels(currentPixel, leftPixel));
            }
            if (y < img.rows - 1)
            {
                auto belowPixel = img.at<cv::Vec3b>(y + 1, x);
                imgGraph->addNewConnection(n, n + img.cols, calcWeightBetweenTwoPixels(currentPixel, belowPixel));
            }
            if (y > 0)
            {
                auto abovePixel = img.at<cv::Vec3b>(y - 1, x);
                imgGraph->addNewConnection(n, n - img.cols, calcWeightBetweenTwoPixels(currentPixel, abovePixel));
            }
            imgGraph->addNewConnection(source, n, getSourceWeightToPixel(y, x, probabilityMap));
            imgGraph->addNewConnection(n, sink, getSinkWeightToPixel(y, x, probabilityMap));
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
