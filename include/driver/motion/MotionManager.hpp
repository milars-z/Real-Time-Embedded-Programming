#pragma once

#include "PwmBoardController.hpp"
#include "ThreadSafeQueue.hpp" 
#include "Tools.hpp"
#include "BugCode.hpp"
#include "Config.hpp"

#include "CoordinateTransformer.hpp"

#include "TaskMonitor.hpp"

#include <thread>
#include <atomic>
#include <set>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <fstream>


class MotionManager{
public:

    MotionManager(std::atomic<int>& system_state, 
                    const std::string& configFile, 
                    const std::string& camera_config, 
                    std::shared_ptr<TaskMonitor> taskMonitor);

    ~MotionManager();

    void stop_thread();
    void start_thread(int core);

    // do easy task
    BugCode_M excuteTask(const std::string& task);

    // do motion set
    BugCode_M excuteMotionSet(const std::string& name);

    // stop motion
    void excuteStop();

    // reset motion
    void excuteReset();

    // APP侧接口，学习模式
    BugCode_M processLearningInput(const std::string& text, const std::string& name);

    // APP侧接口，抓取物体
    void get_obj_MANA(int position_x, int position_y);


private:
    // motion学习相关，保存motionset
    BugCode_M saveMotionSet(std::string motionName, std::vector<MotionTask>& rawTasks);
    
    // motion底层链接相关函数
    void move_joint_to_angle(Joint joint,float targetAngle,int motionSpeed);
    void move_joint_with_val(Joint joint,float angleVal,int motionSpeed);
    void executeMotion(const MotionTask& cmd);
    void servo_set_init();

    // motion工具，刷新动作集列表
    void refresh_motion_list();

    // motion thread
    void motionworker();

    // 检查错误次数，超出3次则清空信号并退出学习模式
    bool check_error_code();

    // 外部调用，motionset刷新
    void learn_motion_fresh();

    // 将需要执行的motion加入队列，测试用
    BugCode_M enqueue_motion(const MotionTask& cmd);

    BugCode_M read_motion_set(const std::string& motion_set_name,MotionSetType type = MotionSetType::External);

    // motion工具
    //string -> Joint
    Joint stringToJoint(const std::string& name);
    // Joint -> string
    std::string JointName(Joint joint);
    //string -> MotionMethod
    MoveMethod stringToMethod(const std::string& method);
    //string -> MotionTask
    MotionTask getMotionTask(const std::string& cmd);


    // 3D_point -> motion_set
    void generate_motion(Point3D res);


private:
    
    // 相关成员结构体
    RobotArmController _arm;
    CoordinateTransformer _arm_calculator;
    std::shared_ptr<TaskMonitor> _taskMonitor;

    // 配置文件目录
    const std::string _motion_folder = Config::Motion::MOTION_SET; 
    const std::string _inner_motion  = Config::Motion::INNER_MOTION_SET;

    // Mananger运行控制
    std::atomic<bool> _isRunning;

    // 紧急停止
    std::atomic<bool> _stopRequested = false;
    
    // 线程相关
    // app -> manager
    std::thread _motionworker;
    ThreadSafeQueue<MotionTask> MotionQueue; 

    // motion学习相关
    // 存储文件夹中搜到的动作集名称
    std::set<std::string> _available_motions; 
    std::string _currentLearningName = "";
    std::vector<MotionTask> _tempTasks ; 
    Joint _currentJoint = Joint::Base;

    // 用来解决学习过程中收到过多无关信息或者学习退出功能
    int learning_error_code = 0;

    // 是否是刚开始学习
    bool is_first_learning = true;

};