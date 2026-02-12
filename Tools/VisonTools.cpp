#include "VisonTools.hpp"

#include <iostream>
#include <vector>
#include <string>

std::string extractText(const std::string& json) {
    size_t start = json.find("\"text\" : \"");
    if (start == std::string::npos) return "";
    start += 10;
    size_t end = json.find("\"", start);
    if (end == std::string::npos) return "";
    return json.substr(start, end - start);
}