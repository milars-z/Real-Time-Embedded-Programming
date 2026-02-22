#include "AttentionDetector.hpp"
#include <iostream>
#include <cmath>
#include <numeric>

using namespace cv;
using namespace std;

// Item detection category
// 加载模型
AttentionDetector::AttentionDetector(const string& model_path) {
    try {
        net = dnn::readNetFromONNX(model_path);
        net.setPreferableBackend(dnn::DNN_BACKEND_OPENCV);
        net.setPreferableTarget(dnn::DNN_TARGET_CPU);
    } catch (const cv::Exception& e) {
        cerr << "[Error] 模型加载失败: " << e.what() << endl;
    }
}

// 对外提供背景检测状态
bool AttentionDetector::is_ready() const {
     return has_background; 
    }

// 获取图像的特征
// 是图像整体特征，不是obj特征
// 输入Img
// 返回模型推理的结果
Mat AttentionDetector::get_features(const Mat& img) {
    Mat blob;
    dnn::blobFromImage(img, blob, 1.0/255.0, cfg.INPUT_SIZE, Scalar(0,0,0), true, false);
    
    // 手动归一化 (ImageNet Mean/Std)
    float* data = blob.ptr<float>();
    int pixels = cfg.INPUT_SIZE.width * cfg.INPUT_SIZE.height;
    for(int c=0; c<3; ++c) {
        float m = (float)mean_val[c];
        float s = (float)std_val[c];
        for(int i=0; i<pixels; ++i) {
            int idx = c*pixels + i;
            data[idx] = (data[idx] - m) / s;
        }
    }
    net.setInput(blob);
    return net.forward();
}

// 更新背景图片
// 将更新后的背景图片存储在bg_features中
// 更新has_background状态
void AttentionDetector::update_background(const Mat& frame) {
    if (frame.empty()) return;
    bg_features = get_features(frame).clone();
    has_background = true;
    cout << "[Info] 背景特征已更新." << endl;
}

// 利用 ROI Pooling 的思想快速提取特征，无需多次推理
// 输入特征map，obj在原图像的具体位置，原图像的size
// 根据obj在原图像的位置和原图像大小，将其映射到特征map中，防止从小到大映射过程中产生的数据丢失
// 获取映射后的特征map
// 返回特征向量
vector<float> AttentionDetector::extract_feature_vector(const Mat& feature_map, const Rect& box, const Size& img_size) {
    // feature_map 维度: [1, C, H, W]
    int C = feature_map.size[1];
    int H = feature_map.size[2];
    int W = feature_map.size[3];

    // 1. 将原图坐标映射到特征图坐标
    float scale_x = (float)W / img_size.width;
    float scale_y = (float)H / img_size.height;

    int fx = (int)(box.x * scale_x);
    int fy = (int)(box.y * scale_y);
    int fw = (int)(box.width * scale_x);
    int fh = (int)(box.height * scale_y);

    // 边界保护 (确保至少有1x1像素)
    fw = max(1, fw); 
    fh = max(1, fh);
    fx = min(max(0, fx), W - fw);
    fy = min(max(0, fy), H - fh);

    // 提取特征向量 (对 ROI 区域内的每个通道求均值)
    vector<float> embedding(C);
    const float* f_ptr = feature_map.ptr<float>();
    int plane_area = H * W;

    for (int c = 0; c < C; c++) {
        const float* channel_ptr = f_ptr + c * plane_area;
        double sum = 0;
        int count = 0;

        for (int y = fy; y < fy + fh; y++) {
            for (int x = fx; x < fx + fw; x++) {
                sum += channel_ptr[y * W + x];
                count++;
            }
        }
        embedding[c] = (float)(sum / count);
    }
    
    // 向量归一化 (方便后续计算余弦相似度或欧氏距离)
    double norm_sq = 0;
    for(float v : embedding) norm_sq += v*v;
    double norm = sqrt(norm_sq) + 1e-8;
    for(float &v : embedding) v /= norm;

    return embedding;
}


// 用作特征检测
// 输入一帧当前的画面
// 对比已保存的背景图像找出其中的obj，将检测到的属性返回
// 返回一个DetectedObject类型的向量，装填该帧图像中所有obj的信息
vector<DetectedObject> AttentionDetector::detect(const Mat& frame) {
    vector<DetectedObject> objects;
    if (!has_background || frame.empty()) {
        cout << "[Error] No Background! " << endl;
        return objects;
    }
        

    // 获取当前帧特征
    Mat f_obj = get_features(frame);

    // 计算差异图
    int C = f_obj.size[1];
    int H = f_obj.size[2];
    int W = f_obj.size[3];

    // 存储差异map
    Mat diff_map(H, W, CV_32F, Scalar(0));
    // 创建遍历指针，对背景的特征与obj图片特征做操作
    const float* p_obj = f_obj.ptr<float>();
    const float* p_bg = bg_features.ptr<float>();
    // 操作次数
    int plane_size = H * W;

    // 遍历C层
    for (int c = 0; c < C; c++) {
        // 初始化指向该层首位
        const float* ptr_o = p_obj + c * plane_size;
        const float* ptr_b = p_bg + c * plane_size;
        // 遍历该层plane_size个位置，求差，并保存进diff_map中
        for (int i = 0; i < plane_size; i++) {
            float diff = ptr_o[i] - ptr_b[i];
            diff_map.at<float>(i) += diff * diff;
        }
    }
    // 开根号，保存在原指针处，原地操作节省空间
    sqrt(diff_map, diff_map);

    // 归一化
    double min_v, max_v;
    minMaxLoc(diff_map, &min_v, &max_v);
    diff_map = (diff_map - min_v) / (max_v - min_v + 1e-8);

    // 边缘抑制
    // 渐变权重
    // 根据diff_map求出模板，再进行点乘
    int H = diff_map.rows;
    int W = diff_map.cols;
    init_edge_mask(H, W);
    cv::multiply(diff_map, edge_mask, diff_map); 

    // 放大 + 二值化
        // 三次样条插值将diff_map放大至图片尺寸
    Mat heatmap_large;
    resize(diff_map, heatmap_large, frame.size(), 0, 0, INTER_CUBIC);
    
        // 小数转整数，将概率图转换为255位黑白图
    Mat mask;
    heatmap_large.convertTo(heatmap_large, CV_8U, 255.0);
        // 忽略小于BINARY_THRESH的杂音
    threshold(heatmap_large, mask, cfg.BINARY_THRESH * 255, 255, THRESH_BINARY);

    // 轮廓提取
    // RETR_EXTERNAL只取外部轮廓
    // CHAIN_APPROX_SIMPLE存储边缘特征点集
    vector<vector<Point>> contours;
    findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);


    int obj_id = 0;
    // 去除超出MAX_OUTPUT_TARGETS的物品
    // 去除面积小于MIN_PIXEL_AREA的物品
    for (const auto& cnt : contours) {
        if (obj_id >= cfg.MAX_OUTPUT_TARGETS) break;
        double area = contourArea(cnt);
        if (area < cfg.MIN_PIXEL_AREA) continue;

        // 对符合要求的点集做方形化处理
        Rect box = boundingRect(cnt);
        

        // 获取图像特征
        vector<float> feat_vec = extract_feature_vector(f_obj, box, frame.size());

        //完善obj的各项属性
        DetectedObject obj;
        obj.id = obj_id++;
        obj.box = box;
        obj.score = (float)area; // 暂时用面积当分数
        obj.feature = feat_vec;  // 保存特征
        obj.match_name = "Unknow";
        obj.match_dist = 1.0f;

        objects.push_back(obj);
    }
    
    return objects;
}

// 用来排除边缘变动的影响
// 生成一个边缘检测模板，对边缘的变化做渐变处理
// 当图像尺寸不变的时候只需要进行一次生成模板的操作，节省运行时间
void AttentionDetector::init_edge_mask(int H, int W) {

    if (!edge_mask.empty() && edge_mask.rows == H && edge_mask.cols == W) {
        return;
    }

    edge_mask = Mat::ones(H, W, CV_32F);

    int margin_h = (int)(H * cfg.EDGE_SUPPRESSION_RATIO);
    int margin_w = (int)(W * cfg.EDGE_SUPPRESSION_RATIO);

    for (int y = 0; y < H; y++) {
        float wy = 1.0f;
        if (y < margin_h) wy = (float)y / margin_h;
        else if (y >= H - margin_h) wy = (float)(H - 1 - y) / margin_h;

        float* ptr = edge_mask.ptr<float>(y);
        for (int x = 0; x < W; x++) {
            float wx = 1.0f;
            if (x < margin_w) wx = (float)x / margin_w;
            else if (x >= W - margin_w) wx = (float)(W - 1 - x) / margin_w;
            
            ptr[x] = wy * wx; 
        }
    }
    cout << "[Info] Mask Generate (" << H << "x" << W << ")" << endl;
}