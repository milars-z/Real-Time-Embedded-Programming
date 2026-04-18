#pragma once

#include <string>
#include <thread>

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

