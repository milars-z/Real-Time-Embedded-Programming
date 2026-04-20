#include "system_config.hpp"

#include <iostream>
#include <string>

/**
 * @brief Create a system configuration based on the selected test module.
 * @param testmodule Name of the test module or operating mode.
 * @return A configured SystemConfig object.
 */
SystemConfig makecfg(const std::string& testmodule) {
    SystemConfig cfg;

    // Speaker test mode: in the test environment, the speaker continuously outputs predefined text to measure elapsed time.
    if (testmodule == "speaker") {
        cfg.enableNlu         = false;
        cfg.enableSpeaker     = true;
        cfg.enableCamera      = false;
        cfg.enableMotion      = false;
        cfg.enableMicrophone  = false;
        cfg.enableScreen      = false;
        cfg.testmode          = TestMode::SPEAKERTEST;

    // Camera test mode: measures camera detection latency and requires screen display.
    }else if (testmodule == "camera") {
        cfg.enableNlu         = false;
        cfg.enableSpeaker     = false;
        cfg.enableCamera      = true;
        cfg.enableMotion      = false;
        cfg.enableMicrophone  = false;
        cfg.enableScreen      = true;
        cfg.testmode          = TestMode::CAMERATEST;

    // Motion test mode: measures the actual response time from screen signal input to motion execution.
    }else if (testmodule == "motion") {
        cfg.enableNlu         = false;
        cfg.enableSpeaker     = false;
        cfg.enableCamera      = false;
        cfg.enableMotion      = true;
        cfg.enableMicrophone  = false;
        cfg.enableScreen      = true;
        cfg.testmode          = TestMode::MOTIONTEST;

    // Microphone test mode: tests speech-to-text performance.
    }else if (testmodule == "microphone") {
        cfg.enableNlu         = true;
        cfg.enableSpeaker     = false;
        cfg.enableCamera      = false;
        cfg.enableMotion      = false;
        cfg.enableMicrophone  = true;
        cfg.enableScreen      = false;
        cfg.testmode          = TestMode::MICROPHONETEST;

    // NLU test mode: tests NLU processing performance while also enabling the microphone and displaying text on the screen.
    // NLU test mode is currently coupled with the microphone. In future plans, this mode will only be used to determine whether the NLU module is enabled, rather than to test NLU extraction itself.
    }else if (testmodule == "nlu") {
        cfg.enableNlu         = true;
        cfg.enableSpeaker     = false;
        cfg.enableCamera      = false;
        cfg.enableMotion      = false;
        cfg.enableMicrophone  = true;
        cfg.enableScreen      = false;
        cfg.testmode          = TestMode::NORMAL; // Not implemented yet

    // Normal mode
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
