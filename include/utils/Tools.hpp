#pragma once

#include <string>
#include <thread>
#include <unordered_map>

#include "system_config.hpp"

#include "Config.hpp"

struct config_var{
    std::string lang;
    std::string host;
    std::string robot;
};

enum class IntentType {
    OTHER,
    DO_MOTION,
    FIND_OBJ,
    LEARN_MOTION,
    LEARN_OBJ,
    CHECK_HOST_NAME,
    CHECK_ROT_NAME,
    GREET,
    BYE,
    UNKNOWN
};

// --- MotionManager Related Definitions --

/// @brief Robot arm joint types.
enum class Joint {
    Base,
    Shoulder,
    Elbow,
    Hand,
    UNKNOWN,
};

/// @brief Movement method definitions.
enum class MoveMethod {
    ABS,
    REL,
};


/**
 * @brief Represents a single motion task/step.
 */
struct MotionTask {

    Joint joint;   // "Base" / "Shoulder" / "Elbow"
    MoveMethod method;
    float targetAngle = 0.0;       
    int motionSpeed = 100;     

};

/**
 * @brief Structure for a complete motion set (a sequence of tasks).
 */
struct MotionSet {
    std::string name;
    std::vector<MotionTask> tasks;
};

/// @brief Classification of motion set types.
enum class MotionSetType {
    Inner,
    External
};

// motion_string -> Task
static const std::unordered_map<std::string, MotionTask> motion_map = {

    {"left_l",  {Joint::Base,     MoveMethod::REL, -5.0f, 100}},
    {"left_r",  {Joint::Base,     MoveMethod::REL,  5.0f, 100}},
    {"left_u",  {Joint::Shoulder, MoveMethod::REL,  5.0f, 100}},
    {"left_d",  {Joint::Shoulder, MoveMethod::REL, -5.0f, 100}},

    {"right_l", {Joint::Hand,     MoveMethod::REL, -5.0f, 100}},
    {"right_r", {Joint::Hand,     MoveMethod::REL,  5.0f, 100}},
    {"right_u", {Joint::Elbow,    MoveMethod::REL,  5.0f, 100}},
    {"right_d", {Joint::Elbow,    MoveMethod::REL, -5.0f, 100}},
};



std::string extractText(const std::string& json);
std::string find_alsa_device(const std::string& keyword);

// tool for multithread
void pinThreadToCore(std::thread &th, std::string thread_name,int core_id);

void print_startup_banner(SystemConfig cfg);

config_var screen_get_var();

void saveVariablesToFile(const std::string& lang, const std::string& host, const std::string& robot );

IntentType parseIntent(const std::string& intent);

// text -> word vector
// use in RobotBrain(extractIntent)
std::vector<std::string> split_text(const std::string& text);

