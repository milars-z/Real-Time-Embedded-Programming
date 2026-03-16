#pragma once

#include "PwmBoardController.hpp"
#include "ThreadSafeQueue.hpp" 

#include <thread>
#include <atomic>

enum class Joint {
    Base,
    Shoulder,
    Elbow,
};

enum class Motion_class {
    MoveJoint,
    Reset,
    Detach,
    Stop,
};

enum class MoveMethod {
    ABS,
    REL,
};

struct MotionTask {

    Joint joint;   // "Base" / "Shoulder" / "Elbow"
    MoveMethod method;
    float targetAngle = 0.0;       
    int motionSpeed = 100;     

};

class MotionManager{
public:

    MotionManager(const std::string& configFile);
    ~MotionManager();

    // 将需要执行的motion加入队列，测试用
    bool enqueue_motion(const MotionTask& cmd);

    // 后续开发
    // 下版本更新
    void create_motion(std::string motion_name);
    void create_motion_set(std::string motion_set_name);
    void read_motion_set(std::string motion_set_name);

private:


    RobotArmController _arm;
    bool _ready = false;
    std::thread _motionworker;

    ThreadSafeQueue<MotionTask> MotionQueue; // MotionManager -> PwmBoardController

    // motion底层链接相关函数
    bool move_joint_to_angle(Joint joint,float targetAngle,int motionSpeed);
    bool move_joint_with_val(Joint joint,float angleVal,int motionSpeed);
    void executeMotion(const MotionTask& cmd);
    bool reset();
    bool stop();

    // 相关工具
    std::string JointName(Joint joint);

    // motion thread
    void motionworker();

    // thread
    std::thread motionThread;
    std::atomic<bool> _isRunning;
    std::atomic<bool> _stopRequested;
    
};