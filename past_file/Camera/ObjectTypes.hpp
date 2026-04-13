#ifndef OBJECT_TYPES_HPP
#define OBJECT_TYPES_HPP

#include <opencv2/opencv.hpp>
#include <vector>

struct DetectedObject {
    int id;                     // Temporarily assigned ID
    cv::Rect box;               // The bounding box on the original image
    std::vector<float> feature; // Extracted feature vectors (used for identity matching)
    float score;                // Test confidence/heat value
    std::string match_name;     // Matched name (default is empty)
    float match_dist;           // Matching distance (smaller, more similar)
};

#endif // OBJECT_TYPES_HPP