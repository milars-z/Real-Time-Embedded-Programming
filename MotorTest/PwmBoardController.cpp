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
    map<string, ServoConfig> servos;

    RobotArmController(const string& configFile) {

        // init i2c device
        _fd = open("/dev/i2c-1", O_RDWR);
        if (_fd < 0 || ioctl(_fd, I2C_SLAVE, 0x40) < 0) {
            cerr << "wrong i2c device or slave address, check sudo i2cdetect -y 1" << endl;
            lastStatus = WRONG_IIC;
            return;
        }

        // init PCA9685
        if (!initHardware()) {
            cerr << "failed to initialize PCA9685" << endl;
            lastStatus = PWM_FAIL;
            return;
        }

        // load config
        if (!loadConfig(configFile)) {
            cerr << "failed to load config file" << endl;
            lastStatus = CONFIG_FAIL;
            return;
        }

        // set initial angles
        cout << "wait for init complete" << endl;
        for (auto& pair : servos) {
            if(!setAngle(pair.first, pair.second.initAngle)) {
                cerr << "failed to set initial angle for servo: " << pair.first << endl;
                lastStatus = SET_FAIL;
                return;
            }
            usleep(100000); 
        }
        cout << "init complete" << endl;
    }

    ~RobotArmController() {
        detachAll();
        if (_fd >= 0) close(_fd);
    }


    // set angle by name, with bounds checking and register writing
    bool setAngle(const string& name, float angle) {

        if (servos.find(name) == servos.end()) 
        return false;
        
        lock_guard<mutex> lock(_mtx);
        ServoConfig& s = servos[name];

        // set max and min angle limits
        if (angle < s.minAngle) angle = s.minAngle;
        if (angle > s.maxAngle) angle = s.maxAngle;
        s.currentAngle = angle;

        
        // peior :0.5ms(0°) ≈ 102, 2.5ms(180°) ≈ 512
        int offValue = (int)(102 + (s.currentAngle / 180.0) * (512 - 102));

        // for every channel, there are 4 registers: ON_L, ON_H, OFF_L, OFF_H
        // 0x06 is the base address for channel 0, then +4 for each subsequent channel
        int regBase = (uint8_t)(0x06 + (4 * s.channel));

        // cout << "set servo [" << s.name << "]  " << s.channel << " angle " << s.currentAngle << " reg 0x" << hex << regBase << dec << endl;

        // servo signal:
        // ON_L, ON_H: 0 (signal starts at the beginning of the cycle)
        // OFF_L, OFF_H: calculated value based on angle (when the signal goes low [102 : (0);512 : (180)]
        // write_state &= writeReg(regBase + 0, 0x00);         // ON_L
        // write_state &= writeReg(regBase + 1, 0x00);         // ON_H
        // write_state &= writeReg(regBase + 2, offValue & 0xFF);  // OFF_L
        // write_state &= writeReg(regBase + 3, offValue >> 8);    // OFF_H

        uint8_t buffer[5];
        buffer[0] = regBase;    
        buffer[1] = 0x00;        // ON_L
        buffer[2] = 0x00;        // ON_H
        buffer[3] = offValue & 0xFF; // OFF_L
        buffer[4] = offValue >> 8;   // OFF_H
        if (write(_fd, buffer, 5) != 5) {
            cerr << "Failed to write servo " << s.name << " via batch write!" << endl;
            return false;
        }      
        return true;
    }

    void detachAll() {
        for (auto& pair : servos) {
            int reg = 0x06 + (4 * pair.second.channel);
            writeReg(reg + 2, 0); 
            writeReg(reg + 3, 0);
        }
    }

    float getAngle(const string& name) { 
        if (servos.find(name) == servos.end()) return -1;
        return servos[name].currentAngle; 
    }

    

private:
    int _fd;
    mutex _mtx;

    // 9865 init sequence:
    bool initHardware() {
        
        bool init_state = true;
        // prescale calculation: 25MHz / (4096 * 50Hz) - 1
        // +0.5 for rounding to nearest integer
        // 25MHz innner clock, 12-bit resolution (4096 steps), 50Hz signal frequency
        uint8_t prescale = (uint8_t)floor(25000000.0 / 4096.0 / 50.0 - 1 + 0.5);


        // reset PCA9685
        // avoid unexpected breakdown
        init_state = writeReg(0x00, 0x00) & init_state; 
        usleep(10000);

        // sleep mode: 0x10 = 00010000 (SLEEP=1)
        init_state = writeReg(0x00, 0x10) & init_state; 
        // set prescale: 0xFE = prescale
        init_state = writeReg(0xFE, prescale) & init_state; 
        // wake up: 0x00 = 00000000 (SLEEP=0)
        init_state = writeReg(0x00, 0x00) & init_state; 
        usleep(10000);

        // Auto-Increment
        // write many registers in one go
        init_state = writeReg(0x00, 0xA1) & init_state; 
        return init_state;
    }

    bool writeReg(uint8_t reg, uint8_t val) {
        uint8_t buf[2] = {reg, val};
        if (write(_fd, buf, 2) != 2) {
            cerr << "failed to write register 0x" << hex << (int)reg << dec << endl;
            return false;
        }
        return true;
    }

    bool loadConfig(const string& path) {
        ifstream file(path);
        if (!file.is_open()) { 
            cerr << "cant find config file: " << path << endl;
            return false; 
        }

        string name;
        
        // get one line
        while (file >> name) {
            // #for comment
            if (name[0] == '#') {
                string dummy; 
                getline(file, dummy); 
                continue;
            }

            // read data: name channel init min max
            int ch;
            float init, minA, maxA;
            if (file >> ch >> init >> minA >> maxA) {
                // currentAngle = init
                servos[name] = {name, ch, init, minA, maxA, init};
                cout << "load: " << name << " channel: " << ch << endl;
            }
        }
        return true;
    }
};


// control for key envent
// not use in feaure project
RobotArmController* arm = nullptr;
bool running = true;

void onKeyPress(int key) {
    if (!arm) return;

    switch(key) {
        case 'a': case 'A': arm->setAngle("Base", arm->getAngle("Base") - 2.0f); break;
        case 'd': case 'D': arm->setAngle("Base", arm->getAngle("Base") + 2.0f); break;
        
        case 'w': case 'W': arm->setAngle("Shoulder", arm->getAngle("Shoulder") + 2.0f); break;
        case 's': case 'S': arm->setAngle("Shoulder", arm->getAngle("Shoulder") - 2.0f); break;
        
        case 'z': case 'Z': arm->setAngle("Elbow", arm->getAngle("Elbow") - 2.0f); break;
        case 'x': case 'X': arm->setAngle("Elbow", arm->getAngle("Elbow") + 2.0f); break;
        
        case 'r': case 'R': arm->detachAll(); cout << "depatch all servos" << endl; break;
        case 'q': case 'Q': running = false; break;
    }

    
    printf("\r[now] base:%.1f | Shoulder:%.1f | Elbow:%.1f    ", 
           arm->getAngle("Base"), arm->getAngle("Shoulder"), arm->getAngle("Elbow"));
    fflush(stdout);
}

int main() {

    cout << "\nuse ws;ad;zx to control the robot arm" << endl;
    cout << "\nuse r to detach all servos, q to quit" << endl;

    arm = new RobotArmController("servo_config.txt");
    if (arm->lastStatus != SUCCESS) {
        cerr << "failed to initialize robot arm controller, error code: " << arm->lastStatus << endl;
        return -1;
    }

    KeypressPublisherStdFunc publisher;
    publisher.registerEventCallback(onKeyPress);
    publisher.start();

    while (running) {
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    arm->detachAll();
    publisher.stop();
    delete arm;
    cout << "\nsystem shutdown complete." << endl;
    return 0;
}