#pragma once

#include "PwmBoardController.hpp"
#include "ThreadSafeQueue.hpp" 
#include "Tools.hpp"
#include "BugCode.hpp"
#include "Config.hpp"

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

// 这个没用了，上版本set集测试用
// extern const std::unordered_map<std::string, MotionSet> MOTIONSET;

// APP层的string在这里转换为具体的Task
static const std::unordered_map<std::string, MotionTask> motion_map = {

    // LEFT
    {"left_l",  {Joint::Base,     MoveMethod::REL, -5.0f, 100}},
    {"left_r",  {Joint::Base,     MoveMethod::REL,  5.0f, 100}},
    {"left_u",  {Joint::Shoulder, MoveMethod::REL,  5.0f, 100}},
    {"left_d",  {Joint::Shoulder, MoveMethod::REL, -5.0f, 100}},

    // RIGHT
    // 还没拉电线，正好验证下无效映射会不会出bug
    // {"right_l", {Joint::Hand,     MoveMethod::REL, -5.0f, 100}},
    // {"right_r", {Joint::Hand,     MoveMethod::REL,  5.0f, 100}},
    {"right_u", {Joint::Elbow,    MoveMethod::REL,  5.0f, 100}},
    {"right_d", {Joint::Elbow,    MoveMethod::REL, -5.0f, 100}},
};

struct ServoTask{
    std::string name;
    float nowAngle;
};



class MotionManager{
public:

    MotionManager(const std::string& configFile);
    ~MotionManager();

    void stop_thread();
    void start_thread(int core);

    // 将需要执行的motion加入队列，测试用
    BugCode_M enqueue_motion(const MotionTask& cmd);

    BugCode_M read_motion_set(const std::string& motion_set_name);

    // 给外部一个接口刷新motion
    void learn_motion_fresh();

    // 改成外部函数算了
    std::string JointName(Joint joint);
    const std::string _motion_folder = Config::Motion::MOTION_SET; // 配置文件目录

    void servo_set_init();

    //do easy task
    BugCode_M excuteTask(const std::string& task);

    // do motion set
    // 需返回执行结果
    // 改成返回错误码了
    BugCode_M excuteMotionSet(const std::string& name);

    // stop motion
    void excuteStop();

    // reset motion
    void excuteReset();

    
    // APP侧接口，学习模式
    BugCode_M processLearningInput(const std::string& text, const std::string& name);




private:


    RobotArmController _arm;
    bool _ready = false;
    
    // app -> manager
    std::thread _motionworker;
    // manager -> servo
    // std::thread _servoworker;

    ThreadSafeQueue<MotionTask> MotionQueue; // App -> Manager

    // ThreadSafeQueue<ServoTask> ServoQueue; // MotionManager -> PwmBoardController

    std::set<std::string> _available_motions; // 存储文件夹中搜到的动作集名称

    BugCode_M saveMotionSet(std::string motionName, std::vector<MotionTask>& rawTasks);
    

    // motion底层链接相关函数
    void move_joint_to_angle(Joint joint,float targetAngle,int motionSpeed);
    void move_joint_with_val(Joint joint,float angleVal,int motionSpeed);
    void executeMotion(const MotionTask& cmd);
    bool reset();
    bool stop_motion();

    // motion工具，刷新动作集列表
    void refresh_motion_list();

    // motion thread
    void motionworker();
    // 新建线程，用来放直接给servo的命令
    void servoworker();

    std::atomic<bool> _isRunning;
    // 紧急停止
    std::atomic<bool> _stopRequested;

    Joint stringToJoint(const std::string& name);
    MoveMethod stringToMethod(const std::string& method);

    //string -> MotionTask
    MotionTask getMotionTask(const std::string& cmd);

    std::string _currentLearningName = "";
    std::vector<MotionTask> _tempTasks ; 
    Joint _currentJoint = Joint::Base;
    std::string motionsetPath = Config::Motion::MOTION_SET;

    // 用来解决队列瞬发问题
    float _last_angle = 0.0;
    std::string _last_name;
    bool servoworker_flag = true;

};