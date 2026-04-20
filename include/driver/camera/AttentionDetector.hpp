#ifndef ATTENTION_DETECTOR_HPP
#define ATTENTION_DETECTOR_HPP

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <vector>
#include <string>
#include "ObjectTypes.hpp"

/**
 * @brief Feature-difference-based object detector.
 *
 * This class uses a neural network to extract image features and detects
 * objects by comparing the difference between the current frame and the
 * background frame. It also generates feature vectors for detected objects.
 */

// Detection configuration parameters
struct DetConfig {
    cv::Size INPUT_SIZE = cv::Size(224, 224);
    float BINARY_THRESH = 0.5f;           // Threshold for binarization
    int MIN_PIXEL_AREA = 300;             // Minimum object area
    float EDGE_SUPPRESSION_RATIO = 0.07f; // Edge suppression ratio
    int MAX_OUTPUT_TARGETS = 10;          // Maximum number of output objects
};

class AttentionDetector {
public:
    AttentionDetector(const std::string& model_path);
    
    // Update background features
    void update_background(const cv::Mat& frame);
    
    // Detect objects and return a list (including feature vectors)
    std::vector<DetectedObject> detect(const cv::Mat& frame);
    
    // Check whether the detector is ready
    bool is_ready() const;

    // Edge suppression mask
    cv::Mat edge_mask; 

private:
    cv::dnn::Net net;
    cv::Mat bg_features;
    DetConfig cfg;
    bool has_background = false;
    
    // Normalization parameters
    const cv::Scalar mean_val = cv::Scalar(0.485, 0.456, 0.406);
    const cv::Scalar std_val = cv::Scalar(0.229, 0.224, 0.225);

    // Internal function: extract full-image features
    cv::Mat get_features(const cv::Mat& img);
    
    // Internal function: extract feature vector from ROI in feature map
    std::vector<float> extract_feature_vector(const cv::Mat& feature_map, const cv::Rect& original_box, const cv::Size& original_size);
    // Initialize edge suppression mask
    void init_edge_mask(int H, int W);
};

#endif
