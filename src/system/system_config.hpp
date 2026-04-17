#pragma once

#include <iostream>
#include <string>

enum class TestMode{

    SPEAKERTEST,
    CAMERATEST,
    MOTIONTEST,
    MICROPHONETEST,
    NORMAL

};


struct SystemConfig {

    bool enableNlu          = true;
    bool enableSpeaker      = true;
    bool enableCamera       = true;
    bool enableMotion       = true;
    bool enableMicrophone   = true;
    bool enableScreen       = true;
    TestMode testmode       = TestMode::NORMAL;

};



SystemConfig makecfg(const std::string& testmodule);