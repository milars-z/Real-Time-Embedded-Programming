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


std::string extractText(const std::string& json);
std::string find_alsa_device(const std::string& keyword);

// tool for multithread
void pinThreadToCore(std::thread &th, std::string thread_name,int core_id);

void print_startup_banner(SystemConfig cfg);

config_var screen_get_var();

void saveVariablesToFile(const std::string& lang, const std::string& host, const std::string& robot );

