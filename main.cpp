#include <opencv2/opencv.hpp>
#include "imgProcessing.h"

#define DEFAULT_OUTPUT_FILENAME "img.png"

// running this code: g++ your_file.cpp -o output_name `pkg-config --cflags --libs opencv4`

int main(int argc, char *argv[])
{
    if (argc == 1 || argc > 3)
    {
        std::cerr << "Invalid arguments";
        return 1;
    }
    // retrieving input filename
    std::string inputFileName = argv[1];

    cv::Mat result = getExtractedObject(inputFileName);

    // getting output filename
    std::string outputFileName = (argc == 2) ? DEFAULT_OUTPUT_FILENAME : argv[2];

    if (cv::imwrite(outputFileName, result))
    {
        std::cout << "Image saved sucessfuly\n";
    }
    else
    {
        std::cerr << "Image not saved sucessfuly\n";
        return 1;
    }
    return 0;
}