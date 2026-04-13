#ifndef FEATURE_MANAGER_HPP
#define FEATURE_MANAGER_HPP

#include "ObjectTypes.hpp"
#include <map>
#include <string>

class FeatureManager {
public:
    FeatureManager(const std::string& db_path = "features_db.yml");

    // 加载本地特征库
    void load();

    // 保存特征到本地
    // input: detected_obj (包含特征), name (如 "obj_1")
    bool save_feature(const DetectedObject& obj, const std::string& name);

    // 匹配函数
    // input: target_name objects 
    // output: 返回匹配到的物体在 objects 列表中的索引，如果没有匹配返回 -1
    // threshold: 距离阈值 (越小越严格)
    // 后期可以改为动态阈值
    int match_object(const std::string& target_name, std::vector<DetectedObject>& objects, float threshold = 0.5f);

private:
    std::string db_file_path;
    // 内存中的数据库: name -> [feature_vector_1, feature_vector_2, ...]
    std::map<std::string, std::vector<std::vector<float>>> feature_db;

    // 计算欧氏距离
    float compute_distance(const std::vector<float>& f1, const std::vector<float>& f2);
};

#endif