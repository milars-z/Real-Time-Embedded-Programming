#ifndef ATTENTION_DETECTOR_HPP
#define ATTENTION_DETECTOR_HPP

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <vector>
#include <string>
#include "ObjectTypes.hpp"

// 配置参数
struct DetConfig {
    cv::Size INPUT_SIZE = cv::Size(224, 224);
    float BINARY_THRESH = 0.5f;       // 二值化阈值
    int MIN_PIXEL_AREA = 300;          // 最小面积
    float EDGE_SUPPRESSION_RATIO = 0.07f; // 周边屏蔽比例
    int MAX_OUTPUT_TARGETS = 10; // 最大obj限制
};

class AttentionDetector {
public:
    AttentionDetector(const std::string& model_path);
    
    // 更新背景
    void update_background(const cv::Mat& frame);
    
    // 检测并返回对象列表 (包含特征向量)
    std::vector<DetectedObject> detect(const cv::Mat& frame);
    
    // 检查是否准备好
    bool is_ready() const;

    // 边缘检测模板
    cv::Mat edge_mask; 

private:
    cv::dnn::Net net;
    cv::Mat bg_features;
    DetConfig cfg;
    bool has_background = false;
    
    // 标准化参数
    const cv::Scalar mean_val = cv::Scalar(0.485, 0.456, 0.406);
    const cv::Scalar std_val = cv::Scalar(0.229, 0.224, 0.225);

    // 内部函数：推理全图特征
    cv::Mat get_features(const cv::Mat& img);
    
    // 内部函数：从特征图上通过 ROI 提取特征向量
    std::vector<float> extract_feature_vector(const cv::Mat& feature_map, const cv::Rect& original_box, const cv::Size& original_size);
    
    void init_edge_mask(int H, int W);
};

#endif