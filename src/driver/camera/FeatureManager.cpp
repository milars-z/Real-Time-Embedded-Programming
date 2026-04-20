#include "FeatureManager.hpp"
#include <iostream>
#include <fstream>

using namespace cv;
using namespace std;

// Few-shot learning and retrieval

// Feature manager class
// Open and load the feature file
// Create a new file if it does not exist
FeatureManager::FeatureManager(const string& path) : db_file_path(path) {
    load();
}


void FeatureManager::load() {
    feature_db.clear();
    FileStorage fs(db_file_path, FileStorage::READ);
    if (!fs.isOpened()) {
        cout << "[Init][FeatureManager] Feature database file does not exist or cannot be opened. A new file will be created: " << db_file_path << endl;
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
    cout << "[Init][FeatureManager] Feature database loaded successfully, total " << feature_db.size() << " object categories." << endl;
}


// Save features
// Rewrite the entire database each time. Performance may degrade when the number of features becomes large and should be improved later
// Input the object's feature and name
// Return the save status
bool FeatureManager::save_feature(const DetectedObject& obj, const string& name) {
    if (obj.feature.empty()) {
        cerr << "[Error][FeatureManager] Attempted to save an empty feature!" << endl;
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
    
    cout << "[Info][FeatureManager] Features for " << name << " have been saved (sample count: " << feature_db[name].size() << ")" << endl;
    return true;
}

// Compute cosine-based distance
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

// Object matching
// Input the object name, the feature set of all objects detected in the current frame, and the matching threshold
// Output the id of the object with the highest matching score
int FeatureManager::match_object(const string& target_name, vector<DetectedObject>& objects, float threshold) {
    if (feature_db.find(target_name) == feature_db.end()) {
        cout << "[Info][FeatureManager] No features for " << target_name << " found in the database" << endl;
        return -1;
    }

    const auto& stored_feats = feature_db[target_name];
    
    int best_match_idx = -1;
    float min_dist = 1000.0f;

    // Traverse all detected objects in the current frame
    for (size_t i = 0; i < objects.size(); ++i) {
        // Compare the current object with all samples of the target name in the database and take the minimum distance
        for (const auto& db_feat : stored_feats) {
            float dist = compute_distance(objects[i].feature, db_feat);
            
            if (dist < min_dist) {
                min_dist = dist;
                // If it is smaller than the threshold, treat it as a candidate
                if (min_dist < threshold) {
                    best_match_idx = (int)i;
                    objects[i].match_name = target_name; 
                    objects[i].match_dist = min_dist;
                }
            }
        }
    }

    if (best_match_idx != -1) {
        cout << "[Match][FeatureManager] Found " << target_name << " (ID: " << objects[best_match_idx].id 
             << ", Dist: " << min_dist << ")" << endl;
    }

    return best_match_idx;
}
