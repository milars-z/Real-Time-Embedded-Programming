#ifndef VISON_TOOLS_HPP
#define VISON_TOOLS_HPP
#include <string>
#include <thread>

std::string extractText(const std::string& json);
std::string find_alsa_device(const std::string& keyword);

// tool for multithread
void pinThreadToCore(std::thread &th, std::string thread_name,int core_id);


#endif