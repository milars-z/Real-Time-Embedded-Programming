#include "PwmBoardController.hpp"

RobotArmController::RobotArmController(const std::string& configFile)
{
    _fd = open("/dev/i2c-1", O_RDWR);

    if (_fd < 0 || ioctl(_fd, I2C_SLAVE, 0x40) < 0) {
        std::cerr << "[Error][PwmBoardController]wrong i2c device or slave address, check sudo i2cdetect -y 1" << std::endl;
        lastStatus = WRONG_IIC;
        return;
    }

    if (!initHardware()) {
        std::cerr << "[Error][PwmBoardController]failed to initialize PCA9685" << std::endl;
        lastStatus = PWM_FAIL;
        return;
    }

    if (!loadConfig(configFile)) {
        std::cerr << "[Error][PwmBoardController]failed to load config file" << std::endl;
        lastStatus = CONFIG_FAIL;
        return;
    }

    std::cout << "[Info][[PwmBoardController]wait for init complete" << std::endl;

    IninServo();

    std::cout << "[Init][PwmBoardController]RobotArmController init successfully" << std::endl;
}

RobotArmController::~RobotArmController() {
    detachAll();
    if (_fd >= 0) close(_fd);
}

bool RobotArmController::setAngle(const std::string& name, float angle){
    if (servos.find(name) == servos.end()) 
    return false;
    
    lock_guard<mutex> lock(_mtx);
    ServoConfig& s = servos[name];

    if (angle < s.minAngle) angle = s.minAngle;
    if (angle > s.maxAngle) angle = s.maxAngle;
    s.currentAngle = angle;

    uint16_t offValue = static_cast<uint16_t>(102 + (s.currentAngle / 180.0) * (512 - 102));

    uint8_t regBase = static_cast<uint8_t>(0x06 + (4 * s.channel));

    uint8_t buffer[5];
    buffer[0] = regBase;    
    buffer[1] = 0x00;        
    buffer[2] = 0x00;       
    buffer[3] = static_cast<uint8_t>(offValue & 0xFF); 
    buffer[4] = static_cast<uint8_t>(offValue >> 8);  
    if (write(_fd, buffer, 5) != 5) {
        cerr << "[Error][PwmBoardController]Failed to write servo " << s.name << " via batch write!" << endl;
        return false;
    }      
    return true;
}

float RobotArmController::getAngle(const string& name){
    if (servos.find(name) == servos.end()) return -1;
        return servos[name].currentAngle; 
}

void RobotArmController::detachAll(){
    for (auto& pair : servos) {
        uint8_t  reg = (uint8_t)(0x06 + (4 * pair.second.channel));
        writeReg(reg + 2, 0); 
        writeReg(reg + 3, 0);
    }
}

bool RobotArmController::initHardware() {
        
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

bool RobotArmController::writeReg(uint8_t reg, uint8_t val) {
        uint8_t buf[2] = {reg, val};
        if (write(_fd, buf, 2) != 2) {
            cerr << "[Error][PwmBoardController]failed to write register 0x" << hex << (int)reg << dec << endl;
            return false;
        }
        return true;
    }

bool RobotArmController::loadConfig(const string& path) {
        ifstream file(path);
        if (!file.is_open()) { 
            cerr << "[Error][PwmBoardController]cant find config file: " << path << endl;
            return false; 
        }

        string name;
        
        while (file >> name) {
            if (name[0] == '#') {
                string dummy; 
                getline(file, dummy); 
                continue;
            }
            int ch;
            float init, minA, maxA;
            if (file >> ch >> init >> minA >> maxA) {
                servos[name] = ServoConfig{name, ch, init, minA, maxA, init};
                cout << "[Info][PwmBoardController]" <<"load: " << name << " channel: " << ch << endl;
            }
        }
        return true;
    }

void RobotArmController::IninServo(){

        for (auto& pair : servos) {
        if(!setAngle(pair.first, pair.second.initAngle)) {
            std::cerr << "[Error][PwmBoardController]failed to set initial angle for servo: " << pair.first << std::endl;
            lastStatus = SET_FAIL;
            return;
        }
        usleep(100000);
    }
}