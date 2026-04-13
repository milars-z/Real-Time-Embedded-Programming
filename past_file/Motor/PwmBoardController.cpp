#include "PwmBoardController.hpp"

RobotArmController::RobotArmController(const std::string& configFile)
{
    _fd = open("/dev/i2c-1", O_RDWR);

    if (_fd < 0 || ioctl(_fd, I2C_SLAVE, 0x40) < 0) {
        std::cerr << "wrong i2c device or slave address, check sudo i2cdetect -y 1" << std::endl;
        lastStatus = WRONG_IIC;
        return;
    }

    if (!initHardware()) {
        std::cerr << "failed to initialize PCA9685" << std::endl;
        lastStatus = PWM_FAIL;
        return;
    }

    if (!loadConfig(configFile)) {
        std::cerr << "failed to load config file" << std::endl;
        lastStatus = CONFIG_FAIL;
        return;
    }

    std::cout << "wait for init complete" << std::endl;

    for (auto& pair : servos) {
        if(!setAngle(pair.first, pair.second.initAngle)) {
            std::cerr << "failed to set initial angle for servo: " << pair.first << std::endl;
            lastStatus = SET_FAIL;
            return;
        }
        usleep(100000);
    }

    std::cout << "[Init] RobotArmController init successfully" << std::endl;
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
        cerr << "Failed to write servo " << s.name << " via batch write!" << endl;
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
            cerr << "failed to write register 0x" << hex << (int)reg << dec << endl;
            return false;
        }
        return true;
    }

bool RobotArmController::loadConfig(const string& path) {
        ifstream file(path);
        if (!file.is_open()) { 
            cerr << "cant find config file: " << path << endl;
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
                cout << "load: " << name << " channel: " << ch << endl;
            }
        }
        return true;
    }


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

// int main() {

//     cout << "\nuse ws;ad;zx to control the robot arm" << endl;
//     cout << "\nuse r to detach all servos, q to quit" << endl;

//     arm = new RobotArmController("../servo_config.txt");
//     if (arm->lastStatus != SUCCESS) {
//         cerr << "failed to initialize robot arm controller, error code: " << arm->lastStatus << endl;
//         return -1;
//     }

//     KeypressPublisherStdFunc publisher;
//     publisher.registerEventCallback(onKeyPress);
//     publisher.start();

//     while (running) {
//         this_thread::sleep_for(chrono::milliseconds(100));
//     }

//     arm->detachAll();
//     publisher.stop();
//     delete arm;
//     cout << "\nsystem shutdown complete." << endl;
//     return 0;
// }