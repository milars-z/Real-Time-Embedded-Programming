#pragma once

#include "PwmBoardController.hpp"
#include "ThreadSafeQueue.hpp" 
#include "VisonTools.hpp"

#include <thread>
#include <atomic>
#include <set>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <fstream>


enum class Joint {
    Base,
    Shoulder,
    Elbow,
    UNKNOWN,
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

struct MotionSet {
    std::string name;
    std::vector<MotionTask> tasks;
};

extern const std::unordered_map<std::string, MotionSet> MOTIONSET;


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
    bool read_motion_set(const std::string& motion_set_name);

    // 给外部一个接口刷新motion
    void learn_motion_fresh();

    // 改成外部函数算了
    std::string JointName(Joint joint);
    const std::string _motion_folder = "./Motion_set"; // 配置文件目录

    void servo_set_init();

private:


    RobotArmController _arm;
    bool _ready = false;
    std::thread _motionworker;

    ThreadSafeQueue<MotionTask> MotionQueue; // MotionManager -> PwmBoardController

    std::set<std::string> _available_motions; // 存储文件夹中搜到的动作集名称
    

    // motion底层链接相关函数
    bool move_joint_to_angle(Joint joint,float targetAngle,int motionSpeed);
    bool move_joint_with_val(Joint joint,float angleVal,int motionSpeed);
    void executeMotion(const MotionTask& cmd);
    bool reset();
    bool stop();

    // motion工具，刷新动作集列表
    void refresh_motion_list();


    // motion thread
    void motionworker();

    // thread
    std::thread motionThread;
    std::atomic<bool> _isRunning;
    std::atomic<bool> _stopRequested;

    Joint stringToJoint(const std::string& name);
    MoveMethod stringToMethod(const std::string& method);
    
};