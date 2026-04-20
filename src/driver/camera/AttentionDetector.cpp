#include "AttentionDetector.hpp"
#include <iostream>
#include <cmath>
#include <numeric>

using namespace cv;
using namespace std;

// Item detection category
// load model
AttentionDetector::AttentionDetector(const string& model_path) {
    try {
        net = dnn::readNetFromONNX(model_path);
        net.setPreferableBackend(dnn::DNN_BACKEND_OPENCV);
        net.setPreferableTarget(dnn::DNN_TARGET_CPU);
    } catch (const cv::Exception& e) {
        cerr << "[Error][AttentionDetector] Model loading failed: " << e.what() << endl;
    }
}

// Provide the background detection status externally
bool AttentionDetector::is_ready() const {
     return has_background; 
    }

// Obtain the features of the image 
// It is the overall feature of the image, not the obj feature 
// Enter Img 
// Return the result of model inference
Mat AttentionDetector::get_features(const Mat& img) {
    Mat blob;
    dnn::blobFromImage(img, blob, 1.0/255.0, cfg.INPUT_SIZE, Scalar(0,0,0), true, false);
    
    // Manual normalization (ImageNet Mean/Std)
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

// Update background image
// Store the updated background image in bg_features
// Update the status of has_mackground
void AttentionDetector::update_background(const Mat& frame) {
    if (frame.empty()) return;
    bg_features = get_features(frame).clone();
    has_background = true;
    cout << "[Info][AttentionDetector] Background features updated." << endl;
}

// Using the concept of ROI Pooling to quickly extract features without the need for multiple inferences
// Input : feature map, location of obj in the original image, size of the original image
// Map obj to the feature map based on its position and size in the original image to prevent data loss during the process of mapping from small to large
// Obtain the mapped feature map
// Return feature vector
vector<float> AttentionDetector::extract_feature_vector(const Mat& feature_map, const Rect& box, const Size& img_size) {
    // feature_map dimension: [1, C, H, W]
    int C = feature_map.size[1];
    int H = feature_map.size[2];
    int W = feature_map.size[3];

    // 1. Map the original image coordinates to the feature map coordinates
    float scale_x = (float)W / img_size.width;
    float scale_y = (float)H / img_size.height;

    int fx = (int)(box.x * scale_x);
    int fy = (int)(box.y * scale_y);
    int fw = (int)(box.width * scale_x);
    int fh = (int)(box.height * scale_y);

    // Boundary protection
    fw = max(1, fw); 
    fh = max(1, fh);
    fx = min(max(0, fx), W - fw);
    fy = min(max(0, fy), H - fh);

    // Extract feature vectors (average each channel within the ROI region)
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
    
    // Vector normalization 
    // (convenient for subsequent calculation of cosine similarity or L2)
    double norm_sq = 0;
    for(float v : embedding) norm_sq += v*v;
    double norm = sqrt(norm_sq) + 1e-8;
    for(float &v : embedding) v /= norm;

    return embedding;
}


// Used for feature detection
// Input: One frame of the current image
// Compare the saved background images to identify the obj and return the detected attributes
// Return: a vector of DetectedObject type, loaded with information about all objs in the frame image
vector<DetectedObject> AttentionDetector::detect(const Mat& frame) {
    vector<DetectedObject> objects;
    if (!has_background || frame.empty()) {
        //cout << "[Error] No Background! " << endl;
        return objects;
    }
        

    // Retrieve current frame features
    Mat f_obj = get_features(frame);

    int C = f_obj.size[1];
    int H = f_obj.size[2];
    int W = f_obj.size[3];

    // Store the difference map
    Mat diff_map(H, W, CV_32F, Scalar(0));
    // Create pointers to manipulate background features and obj image features
    const float* p_obj = f_obj.ptr<float>();
    const float* p_bg = bg_features.ptr<float>();
    int plane_size = H * W;

    // Traverse layer C
    for (int c = 0; c < C; c++) {

        const float* ptr_o = p_obj + c * plane_size;
        const float* ptr_b = p_bg + c * plane_size;
        // Traverse the layer's plane_size positions, calculate the difference, and save it in diff_map
        for (int i = 0; i < plane_size; i++) {
            float diff = ptr_o[i] - ptr_b[i];
            diff_map.at<float>(i) += diff * diff;
        }
    }
    // Root it, save it at the original pointer, and operate in place to save space
    sqrt(diff_map, diff_map);

    // normalization
    double min_v, max_v;
    minMaxLoc(diff_map, &min_v, &max_v);
    diff_map = (diff_map - min_v) / (max_v - min_v + 1e-8);

    // Edge suppression
    // Gradient Weight
    // Calculate the template based on diff_map, and then perform dot multiplication
    H = diff_map.rows;
    W = diff_map.cols;
    init_edge_mask(H, W);
    cv::multiply(diff_map, edge_mask, diff_map); 

    // Magnification+binarization
        // Triple spline interpolation enlarges diff_map to image size
    Mat heatmap_large;
    resize(diff_map, heatmap_large, frame.size(), 0, 0, INTER_CUBIC);
    
        // Convert decimals to integers to convert probability graphs into 255 bit black and white graphs
    Mat mask;
    heatmap_large.convertTo(heatmap_large, CV_8U, 255.0);
        // Ignore noise smaller than BINARY_THRESH
    threshold(heatmap_large, mask, cfg.BINARY_THRESH * 255, 255, THRESH_BINARY);

    // Contour extraction
    // RETR_EXTERNAL retrieves only external contours
    // CHAIN_APPROX_SIMPLE stores the contour feature points
    vector<vector<Point>> contours;
    findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);


    int obj_id = 0;
    // Discard objects exceeding MAX_OUTPUT_TARGETS
    // Discard objects with an area smaller than MIN_PIXEL_AREA
    for (const auto& cnt : contours) {
        if (obj_id >= cfg.MAX_OUTPUT_TARGETS) break;
        double area = contourArea(cnt);
        if (area < cfg.MIN_PIXEL_AREA) continue;

        // Apply bounding-rectangle processing to contours that meet the requirements
        Rect box = boundingRect(cnt);
        

        // Extract image features
        vector<float> feat_vec = extract_feature_vector(f_obj, box, frame.size());

        //Complete the attributes of the object
        DetectedObject obj;
        obj.id = obj_id++;
        obj.box = box;
        obj.score = (float)area; 
        obj.feature = feat_vec;  
        obj.match_name = "Unknown";
        obj.match_dist = 1.0f;

        objects.push_back(obj);
    }
    
    return objects;
}

// Used to eliminate the influence of edge variations
// Generate an edge suppression mask and apply gradual attenuation to edge changes
// When the image size does not change, the mask only needs to be generated once to save runtime
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
    cout << "[Info][AttentionDetector] Mask Generate (" << H << "x" << W << ")" << endl;
}
