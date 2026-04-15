#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <cmath>
#include <mutex>
#include <cstdint>

#include "KeypressPublisherStdFunc.h"

using namespace std;

struct ServoConfig {
    string name;
    int channel;
    float initAngle;
    float minAngle;
    float maxAngle;
    float currentAngle;
};

enum BugCode {
    SUCCESS = 0,
    WRONG_IIC = -1,
    PWM_FAIL = -2,
    CONFIG_FAIL = -3,
    ILLIGLE_NAME = -4,
    SET_FAIL = -5
};

class RobotArmController {
public:

    BugCode lastStatus = SUCCESS;
    std::map<std::string, ServoConfig> servos;

    RobotArmController(const std::string& configFile);
    ~RobotArmController();

    bool setAngle(const std::string& name, float angle);
    void detachAll();
    float getAngle(const std::string& name);

    // 外部调用，根据config重置角度
    void IninServo();

private:

    int _fd;
    std::mutex _mtx;

    bool initHardware();
    bool writeReg(uint8_t reg, uint8_t val);
    bool loadConfig(const std::string& path);
};
