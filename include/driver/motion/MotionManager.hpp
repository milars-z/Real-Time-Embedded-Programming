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

    /// @brief Stop and start the motion processing thread.
    void stop_thread();
    void start_thread(int core);

    /// @brief Execute a simple task.
    BugCode_M excuteTask(const std::string& task);

     /// @brief Execute a specific set of motion sequences.
    BugCode_M excuteMotionSet(const std::string& name);

    /// @brief Stop all current motions immediately.
    void excuteStop();

    /// @brief Reset the sevro state init position.
    void excuteReset();

    /**
     * @brief Application-side API for Learning Mode.
     * @param text The input command 
     * @param name The name of motion set.
     * @return BugCode_M Status of the learning process.
     */
    BugCode_M processLearningInput(const std::string& text, const std::string& name);

    /**
     * @brief Application-side API for grabbing an object.
     * @param position_x The X-coordinate for the grab target.
     * @param position_y The Y-coordinate for the grab target.
     */
    void get_obj_MANA(int position_x, int position_y);


private:

    // --- Motion Learning ---
    /// @brief Save a learned motion set to the system.
    BugCode_M saveMotionSet(std::string motionName, std::vector<MotionTask>& rawTasks);
    
    // --- Low-level Linkage & Control ---
    /// @brief Move a specific joint to a target absolute angle.
    void move_joint_to_angle(Joint joint,float targetAngle,int motionSpeed);

    /// @brief Move a specific joint by a relative value.
    void move_joint_with_val(Joint joint,float angleVal,int motionSpeed);

    /// @brief Execute a raw motion command.
    void executeMotion(const MotionTask& cmd);

    /// @brief Initialize the servo system/settings.
    void servo_set_init();

    // --- Motion Utilities ---
    /// @brief Refresh the list of available motion sets.
    void refresh_motion_list();

    /// @brief Main motion processing worker thread.
    void motionworker();

     /** 
     * @brief Check for error codes. 
     * @details If errors exceed 3, clear signals and exit Learning Mode.
     * @return true if error threshold is reached, false otherwise.(no use now feedback by supervisor)
     */
    bool check_error_code();

    /// @brief External call to refresh/reload motion sets.
    void learn_motion_fresh();

    /// @brief Add a motion to the execution queue (primarily for testing).
    BugCode_M enqueue_motion(const MotionTask& cmd);

    /// @brief Read a motion set by name and type.
    BugCode_M read_motion_set(const std::string& motion_set_name,MotionSetType type = MotionSetType::External);

    // --- Type Converters ---
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
    
    // --- Core Components ---
    RobotArmController _arm;                    ///< Controller for the robot arm hardware
    CoordinateTransformer _arm_calculator;      ///< Handles kinematics and coordinate transformations
    std::shared_ptr<TaskMonitor> _taskMonitor;  ///< Shared monitor for task execution status

    // --- Configuration Paths ---
    const std::string _motion_folder = Config::Motion::MOTION_SET;         ///< Directory for user-defined motion sets
    const std::string _inner_motion  = Config::Motion::INNER_MOTION_SET;   ///< Directory for internal/system motion sets

    // --- Execution Control ---
    std::atomic<bool> _isRunning;                ///< Main operational status of the manager
    std::atomic<bool> _stopRequested = false;    ///< Flag for emergency stop requests
    
    // --- Threading & Communication ---
    std::thread _motionworker;                   ///< Internal background thread for motion processing
    ThreadSafeQueue<MotionTask> MotionQueue;     ///< Task queue 

    // --- Learning Mode State ---
    std::set<std::string> _available_motions;    ///< Set of motion names found in the storage folders
    std::string _currentLearningName = "";       ///< Name of the motion set currently being learned
    std::vector<MotionTask> _tempTasks ;         ///< Temporary buffer for newly recorded motion tasks
    Joint _currentJoint = Joint::Base;           ///< The specific joint normal use

    /** 
     * @brief Error tracking for the learning process. 
     * Used to handle redundant/irrelevant information or to trigger an exit from learning mode.
     */
    int learning_error_code = 0;

    /// @brief Flag to indicate if the learning process has just initiated.
    bool is_first_learning = true;

};