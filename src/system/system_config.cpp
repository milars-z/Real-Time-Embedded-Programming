#include "system_config.hpp"

#include <iostream>
#include <string>

SystemConfig makecfg(const std::string& testmodule) {
    SystemConfig cfg;

    // speaker测试模式，测试环境下speaker持续输出指定的text，测量耗时
    if (testmodule == "speaker") {
        cfg.enableNlu         = false;
        cfg.enableSpeaker     = true;
        cfg.enableCamera      = false;
        cfg.enableMotion      = false;
        cfg.enableMicrophone  = false;
        cfg.enableScreen      = false;
        cfg.testmode          = TestMode::SPEAKERTEST;

    // camera测试模式，测试camera检测的延迟，需要屏幕显示
    }else if (testmodule == "camera") {
        cfg.enableNlu         = false;
        cfg.enableSpeaker     = false;
        cfg.enableCamera      = true;
        cfg.enableMotion      = false;
        cfg.enableMicrophone  = false;
        cfg.enableScreen      = true;
        cfg.testmode          = TestMode::CAMERATEST;

    // motion测试模式，测试screen发送信号到motion实际相应的时间
    }else if (testmodule == "motion") {
        cfg.enableNlu         = false;
        cfg.enableSpeaker     = false;
        cfg.enableCamera      = false;
        cfg.enableMotion      = true;
        cfg.enableMicrophone  = false;
        cfg.enableScreen      = true;
        cfg.testmode          = TestMode::MOTIONTEST;

    // microphone测试模式，测试stt
    }else if (testmodule == "microphone") {
        cfg.enableNlu         = true;
        cfg.enableSpeaker     = false;
        cfg.enableCamera      = false;
        cfg.enableMotion      = false;
        cfg.enableMicrophone  = true;
        cfg.enableScreen      = false;
        cfg.testmode          = TestMode::MICROPHONETEST;

    // nlu测试模式，测试nlu转换性能，同时激活microphone，在屏幕输出text
    // nlu测试暂时和microphone绑定，后续规划中该功能仅用作（是否启动nlu模块）而非测试nlu提取
    }else if (testmodule == "nlu") {
        cfg.enableNlu         = true;
        cfg.enableSpeaker     = false;
        cfg.enableCamera      = false;
        cfg.enableMotion      = false;
        cfg.enableMicrophone  = true;
        cfg.enableScreen      = false;
        cfg.testmode          = TestMode::NORMAL; // 还没写

    // 普通模式
    }else{
        cfg.enableNlu         = true;
        cfg.enableSpeaker     = true;
        cfg.enableCamera      = true;
        cfg.enableMotion      = true;
        cfg.enableMicrophone  = true;
        cfg.enableScreen      = true;
        cfg.testmode          = TestMode::NORMAL;
    }


    return cfg;
}