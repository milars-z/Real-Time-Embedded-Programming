#include "FeatureManager.hpp"
#include <iostream>
#include <fstream>

using namespace cv;
using namespace std;

// 少量样本学习检索

// 特征管理类
// 打开特征文件并加载
// 没有文件则会重新创建
FeatureManager::FeatureManager(const string& path) : db_file_path(path) {
    load();
}


void FeatureManager::load() {
    feature_db.clear();
    FileStorage fs(db_file_path, FileStorage::READ);
    if (!fs.isOpened()) {
        cout << "[Init][FeatureManager] 特征库文件不存在或无法打开，将创建新文件: " << db_file_path << endl;
        return;
    }

    FileNode root = fs.root();
    for (FileNodeIterator it = root.begin(); it != root.end(); ++it) {
        string name = (*it).name();
        FileNode features_node = *it;
        vector<vector<float>> feats;
        for (auto fn : features_node) {
            vector<float> vec;
            fn >> vec;
            feats.push_back(vec);
        }
        feature_db[name] = feats;
    }
    fs.release();
    cout << "[Init][FeatureManager] 特征库加载完成，共 " << feature_db.size() << " 类物品。" << endl;
}


// 保存特征
// 全量重写，特征量过大运行速度会变慢，后续需改进
// 输入物品的特征与物品的名字
// 返回保存状态
bool FeatureManager::save_feature(const DetectedObject& obj, const string& name) {
    if (obj.feature.empty()) {
        cerr << "[Error][FeatureManager] 试图保存空特征！" << endl;
        return false;
    }

    feature_db[name].push_back(obj.feature);

    FileStorage fs(db_file_path, FileStorage::WRITE);
    if (!fs.isOpened()) return false;

    for (const auto& pair : feature_db) {
        fs << pair.first << "["; 
        for (const auto& vec : pair.second) {
            fs << vec; 
        }
        fs << "]"; 
    }
    fs.release();
    
    cout << "[Info][FeatureManager] 已保存 " << name << " 的特征 (样本数: " << feature_db[name].size() << ")" << endl;
    return true;
}

// 计算L2
float FeatureManager::compute_distance(const vector<float>& f1, const vector<float>& f2) {

    if (f1.size() != f2.size()) return 1.0f; 

    double dot_product = 0.0;
    double norm_a = 0.0;
    double norm_b = 0.0;

    for (size_t i = 0; i < f1.size(); ++i) {
        dot_product += f1[i] * f2[i];
        norm_a += f1[i] * f1[i];
        norm_b += f2[i] * f2[i];
    }

    if (norm_a == 0.0 || norm_b == 0.0) {
        return 1.0f; 
    }
    float similarity = (float)(dot_product / (sqrt(norm_a) * sqrt(norm_b)));
    
    return 1.0f - similarity; 
}

// 物体匹配
// 输入物体名称，所有当前帧中检测到的物体的特征组，检测阈值
// 输出当前匹配度最高的一个obj的id
int FeatureManager::match_object(const string& target_name, vector<DetectedObject>& objects, float threshold) {
    if (feature_db.find(target_name) == feature_db.end()) {
        cout << "[Info][FeatureManager] 数据库中没有 " << target_name << " 的特征" << endl;
        return -1;
    }

    const auto& stored_feats = feature_db[target_name];
    
    int best_match_idx = -1;
    float min_dist = 1000.0f;

    // 遍历画面中所有检测到的物体
    for (size_t i = 0; i < objects.size(); ++i) {
        // 让当前物体与数据库中该名字的所有样本比对，取最小值
        for (const auto& db_feat : stored_feats) {
            float dist = compute_distance(objects[i].feature, db_feat);
            
            if (dist < min_dist) {
                min_dist = dist;
                // 如果小于阈值，认为是候选
                if (min_dist < threshold) {
                    best_match_idx = (int)i;
                    objects[i].match_name = target_name; 
                    objects[i].match_dist = min_dist;
                }
            }
        }
    }

    if (best_match_idx != -1) {
        cout << "[Match][FeatureManager] 找到 " << target_name << " (ID: " << objects[best_match_idx].id 
             << ", Dist: " << min_dist << ")" << endl;
    }

    return best_match_idx;
}