#ifndef FEATURE_MANAGER_HPP
#define FEATURE_MANAGER_HPP

#include "ObjectTypes.hpp"
#include <map>
#include <string>

class FeatureManager {
public:

    /// @brief Constructor to initialize the feature manager with path.
    FeatureManager(const std::string& db_path = "features_db.yml");

    /// @brief Load the local feature dataset
    void load();

    /**
     * @brief Save an object's feature to the local database.
     * @param obj The detected object containing feature data.
     * @param name The label/name for the object (e.g., "obj_1").
     * @return true if saved successfully, false otherwise.
     */
    bool save_feature(const DetectedObject& obj, const std::string& name);

    /**
     * @brief Match a target name against a list of detected objects.
     * @param target_name The name to search for.
     * @param objects Vector of candidate objects to match against.
     * @param threshold Distance threshold (smaller values are stricter).
     * @return Index of the matched object in the list, or -1 if no match found.
     */
    int match_object(const std::string& target_name, std::vector<DetectedObject>& objects, float threshold = 0.5f);

private:

    std::string db_file_path;  ///< Path to the local database file

    /// @brief In-memory database: name -> [feature_vector_1, feature_vector_2, ...]
    std::map<std::string, std::vector<std::vector<float>>> feature_db;

    /// @brief Compute Euclidean distance between two feature vectors.
    float compute_distance(const std::vector<float>& f1, const std::vector<float>& f2);
};

#endif