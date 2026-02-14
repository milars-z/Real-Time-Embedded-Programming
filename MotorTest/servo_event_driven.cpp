#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include "KeypressPublisherStdFunc.h" 

using namespace std;

/**
 * ServoEngine - raspi5 ServoEngine
 * 
 * basic reference : berndporr
 * rebuild for raspi5 : Ziyin Zeng
 * License ：MIT
 * Time : 01,30,2026
 * 
 **/

// --- MG90S_Servo ---
class MG90sController {
public:
    // servo init setting 
    MG90sController(int gpio) {

        string cmd = "sudo pinctrl set " + to_string(gpio) + " a0";
        system(cmd.c_str());
        system("echo 0 | sudo tee /sys/class/pwm/pwmchip0/export > /dev/null 2>&1");
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        write_sysfs("enable", "0");

        // initial setting
        write_sysfs("period", "20000000"); 


        // initial angle
        // This value depends on the position during assembly and may need to be adjusted
        setAngle(30.0); 

        write_sysfs("enable", "1");
        
    }

    // change angle
    void setAngle(float angle) {
        lock_guard<mutex> lock(mtx);
        // these values depends on the position during assembly and may need to be adjusted
        if (angle < 30) angle = 30;
        if (angle > 150) angle = 150;
        currentAngle = angle;

        long duty = 500000 + (long)(currentAngle / 180.0 * 2000000);
        write_sysfs("duty_cycle", to_string(duty));
        cout << "\r current angle: " << currentAngle << "    " << flush;
    }

    float getAngle() {
         return currentAngle; 
    }

private:
    // This value depends on the position during assembly and may need to be adjusted
    float currentAngle = 30.0;
    mutex mtx;
    const string path = "/sys/class/pwm/pwmchip0/pwm0/";

    void write_sysfs(string file, string val) {
        ofstream fs(path + file);
        if (fs.is_open()) fs << val;
    }
};

// --- create a servo on GPIO12 ---
MG90sController servo(12); 
bool exitservo = false;

// --- Event function ---
void onKeyPress(int key) {

    if (key == 'a' || key == 'A') servo.setAngle(servo.getAngle() - 5.0f);
    if (key == 'd' || key == 'D') servo.setAngle(servo.getAngle() + 5.0f);
    if (key == 'q' || key == 'Q') exitservo = true;

}

int main() {
    cout << "servo MG90S test" << endl;
    cout << "pring [a][d] to control, q for exit" << endl;

    KeypressPublisherStdFunc publisher;
    publisher.registerEventCallback(onKeyPress);
    publisher.start();

    while (!exitservo) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    publisher.stop();
    cout << "\n exit successfully " << endl;
    return 0;
}