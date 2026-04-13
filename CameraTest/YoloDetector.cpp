// YoloDetector.cpp
#include "YoloDetector.h"

#include <opencv2/dnn.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>

YoloDetector::YoloDetector(const std::string& modelPath, int inputW, int inputH, float confThreshold, float nmsThreshold)
    : inpW(inputW), inpH(inputH), confThresh(confThreshold), nmsThresh(nmsThreshold), valid(false) {
    try {
        net = cv::dnn::readNet(modelPath);
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_DEFAULT);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        valid = true;
    } catch (const cv::Exception& e) {
        std::cerr << "[YoloDetector] failed to load model: " << e.what() << std::endl;
        valid = false;
    }
}

bool YoloDetector::isValid() const {
    return valid;
}

std::vector<Detection> YoloDetector::detect(const cv::Mat& image) {
    std::vector<Detection> detections;
    if (!valid || image.empty()) return detections;

    cv::Mat blob;
    cv::Mat img;
    // convert to RGB if needed (assume input BGR from OpenCV)
    image.copyTo(img);

    cv::dnn::blobFromImage(img, blob, 1.0/255.0, cv::Size(inpW, inpH), cv::Scalar(), true, false);
    net.setInput(blob);

    std::vector<cv::Mat> outputs;
    cv::Mat out = net.forward();

    // Handle common YOLO ONNX output shapes
    cv::Mat pred;
    if (out.dims == 3) {
        const int dim1 = out.size[1];
        const int dim2 = out.size[2];
        cv::Mat mat(dim1, dim2, CV_32F, out.ptr<float>());
        pred = (dim1 < dim2) ? mat.t() : mat;
    } else if (out.dims == 2) {
        pred = out;
    } else {
        pred = out.reshape(1, static_cast<int>(out.total() / out.size[out.dims - 1]));
    }

    const int numCols = pred.cols;

    std::vector<cv::Rect> boxes;
    std::vector<float> scores;
    std::vector<int> classIds;

    float xFactor = static_cast<float>(image.cols) / inpW;
    float yFactor = static_cast<float>(image.rows) / inpH;

    for (int i = 0; i < pred.rows; ++i) {
        const float* data = pred.ptr<float>(i);
        float box_cx = data[0];
        float box_cy = data[1];
        float box_w = data[2];
        float box_h = data[3];
        const bool hasObj = (numCols == 85);
        float obj_conf = hasObj ? data[4] : 1.0f;

        int bestClass = -1;
        float bestScore = 0.0f;
        const int clsStart = hasObj ? 5 : 4;
        for (int c = clsStart; c < numCols; ++c) {
            float s = data[c];
            if (s > bestScore) { bestScore = s; bestClass = c - clsStart; }
        }

        float score = obj_conf * bestScore;
        if (score < confThresh) continue;

        float cx = box_cx;
        float cy = box_cy;
        float w = box_w;
        float h = box_h;

        int left = static_cast<int>((cx - w/2.0f) * xFactor);
        int top = static_cast<int>((cy - h/2.0f) * yFactor);
        int width = static_cast<int>(w * xFactor);
        int height = static_cast<int>(h * yFactor);

        left = std::max(0, std::min(left, image.cols-1));
        top = std::max(0, std::min(top, image.rows-1));
        width = std::max(0, std::min(width, image.cols - left));
        height = std::max(0, std::min(height, image.rows - top));

        boxes.emplace_back(left, top, width, height);
        scores.emplace_back(score);
        classIds.emplace_back(bestClass);
    }

    // nms
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, scores, confThresh, nmsThresh, indices);
    for (int id : indices) {
        Detection d;
        d.box = boxes[id];
        d.score = scores[id];
        d.classId = classIds[id];
        detections.push_back(d);
    }

    return detections;
}

void YoloDetector::drawDetections(cv::Mat& image, const std::vector<Detection>& dets, const std::vector<std::string>& classNames) {
    for (const auto& d : dets) {
        cv::rectangle(image, d.box, cv::Scalar(0, 255, 0), 2);
        std::ostringstream label;
        if (!classNames.empty() && d.classId >= 0 && d.classId < (int)classNames.size()) {
            label << classNames[d.classId] << ": ";
        } else {
            label << "id=" << d.classId << ": ";
        }
        label << std::fixed << std::setprecision(2) << d.score;
        int baseLine;
        cv::Size labelSize = cv::getTextSize(label.str(), cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
        int top = std::max(d.box.y, labelSize.height);
        cv::rectangle(image, cv::Point(d.box.x, top - labelSize.height - 4),
                      cv::Point(d.box.x + labelSize.width, top + baseLine - 4), cv::Scalar(0, 255, 0), cv::FILLED);
        cv::putText(image, label.str(), cv::Point(d.box.x, top - 4), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0,0,0), 1);
    }
}
