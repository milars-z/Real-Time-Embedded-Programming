#include "MotionHandle.hpp"

bool MotionHandle::parseMotionCommand(std::string text,MotionTask& cmd){

    std::string lowerText = toLower(text);

    // Match joint keywords as complete words
    std::regex jointPattern(R"(\b(base|shoulder|elbow)\b)");
    std::smatch jointMatch;

    if (!std::regex_search(lowerText, jointMatch, jointPattern)) {
        return false;
    }

    std::string jointStr = jointMatch[1].str();


    cmd.method = MoveMethod::REL;
    cmd.motionSpeed = 100;
    cmd.targetAngle = 10;

    if (jointStr == "base") {
        cmd.joint = Joint::Base;
    } else if (jointStr == "shoulder") {
        cmd.joint = Joint::Shoulder;
    } else if (jointStr == "elbow") {
        cmd.targetAngle = cmd.targetAngle * -1;
        cmd.joint = Joint::Elbow;
    } else { // 防止代码改一半
        cmd.joint = Joint::UNKNOWN;
        return false;
    }



    return true;
}

// void MotionHandle::sendmotion(MotionTask cmd){

// }

// ----------------------
// Helper: convert string to lowercase
// ----------------------
std::string MotionHandle::toLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return text;
}