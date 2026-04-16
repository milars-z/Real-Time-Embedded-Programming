// YoloDetector.h
#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

struct Detection {
    cv::Rect box;
    int classId;
    float score;
};

class YoloDetector {
public:
    // modelPath: path to ONNX model
    // inputW/inputH: model input size
    YoloDetector(const std::string& modelPath, int inputW = 640, int inputH = 480,
                 float confThreshold = 0.4f, float nmsThreshold = 0.45f);

    bool isValid() const;

    // detect objects in image (image is not modified)
    std::vector<Detection> detect(const cv::Mat& image);

    // draw detections onto image (in-place)
    void drawDetections(cv::Mat& image, const std::vector<Detection>& dets,
                        const std::vector<std::string>& classNames = {});

private:
    cv::dnn::Net net;
    int inpW;
    int inpH;
    float confThresh;
    float nmsThresh;
    bool valid;
};
