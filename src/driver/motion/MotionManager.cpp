#include "MotionManager.hpp"
#include "SystemCode.hpp"

// Constructor
// Construct the underlying engine
// Refresh the motion set
// Initial state
MotionManager::MotionManager(std::atomic<int>& system_state, 
                             const std::string& configFile, 
                             const std::string& camera_config, 
                             std::shared_ptr<TaskMonitor> taskMonitor):
_arm(configFile), _arm_calculator(camera_config), _taskMonitor(taskMonitor)
{

    if (_arm.lastStatus != SUCCESS) {
            std::cerr << "[Error][MotionManager]failed to initialize robot arm controller, error code: "
                    << _arm.lastStatus << std::endl;
            _isRunning = false;
            _stopRequested = true;
            system_state |= ERR_MOTION_INIT;
            return;
        }

    refresh_motion_list();
    
    std::cerr << "[Init][MotionManager] init successfully"<< std::endl;
    _isRunning = true;
    _stopRequested = true;

};

MotionManager::~MotionManager() = default;

// Externally callable, thread-related
void MotionManager::start_thread(int core){

    _stopRequested = false;
     _motionworker = std::thread(&MotionManager::motionworker, this);
    pinThreadToCore(_motionworker,"motion", core);

}

// Externally callable, thread-related
void MotionManager::stop_thread(){
    _isRunning = false;
    _stopRequested = true;
    MotionQueue.stop();

    if (_motionworker.joinable()) {
        _motionworker.join();
    }
}

// Thread queue related, append motion commands to the queue
BugCode_M MotionManager::enqueue_motion(const MotionTask& cmd){

    if (!_isRunning) return BugCode_M::MotionQueError;
    MotionQueue.push(cmd);
    
    return BugCode_M::Success;

};

// Load commands from the motion set and execute them
BugCode_M MotionManager::read_motion_set(const std::string& motion_set_name, MotionSetType type) {

    BugCode_M state = BugCode_M::Init;
    std::string filePath;
    if (type == MotionSetType::External){
        filePath = _motion_folder + "/" + motion_set_name + ".json";
    }else{
        filePath = _inner_motion + "/" + motion_set_name + ".json";
    }
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        state = BugCode_M::CannotOpenMotionFile;
        return state;
    }

    try {
        nlohmann::json j;
        file >> j;

        for (const auto& item : j["tasks"]) {
            MotionTask task;
            // Set default values in case of read errors
            task.joint = Joint::Base;
            task.method = MoveMethod::REL;
            task.motionSpeed = 50;
            task.targetAngle = 5;
            task.joint = stringToJoint(item["joint"]);
            task.method = stringToMethod(item["method"]);
            task.targetAngle = item["val"];
            task.motionSpeed = item["speed"];
            
            enqueue_motion(task);
        }
        std::cout << "[Info][MotionManager] Loaded and enqueued: " << motion_set_name << std::endl;
        state = BugCode_M::DoingSuccess;
#ifdef TESTMODE
        if (type == MotionSetType::External){
            TaskEvent _taskevent;
            _taskevent.moduleName = "MotionSet";
            MotionResult _res;
            _res.name = motion_set_name;
            _res.result = state;
            _taskevent.result = _res;
            _taskMonitor->postEvent(_taskevent);
        }
#endif
        state = BugCode_M::Success;
        return state;

    } catch (nlohmann::json::parse_error& e) {
        std::cerr << "[Error][MotionManager] Parse error in " << motion_set_name << ": " << e.what() << std::endl;
        state = BugCode_M::ReadInvalidSet;
        return state;
    }
}

// Motion-control related, connect to the underlying PWM control and move the specified joint to the target angle
void MotionManager::move_joint_to_angle(Joint joint,float targetAngle,int motionSpeed){

    std::string name;
    float nowAngle;
    float angleChange;
    bool state = false;
    int step;

    name = JointName(joint);

    nowAngle = _arm.getAngle(name);
    angleChange = motionSpeed * 0.02f; 

    while(std::abs(targetAngle - nowAngle)>0.2f){

        if (nowAngle - targetAngle < -angleChange){
            nowAngle = nowAngle + angleChange;
        }else if (nowAngle - targetAngle > angleChange){
            nowAngle = nowAngle - angleChange;
        }else{
            nowAngle = targetAngle;
        }

        _arm.setAngle(name, nowAngle);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    // return state;
};

// Motion-control related, connect to the underlying PWM control and move the specified joint by a relative angle
void MotionManager::move_joint_with_val(Joint joint,float angleVal,int motionSpeed){

    std::string name;
    float nowAngle;
    float angleChange;
    float targetAngle;
    int step;

    name = JointName(joint);

    nowAngle = _arm.getAngle(name);
    angleChange = motionSpeed * 0.02f;
    targetAngle = nowAngle + angleVal;

    while(std::abs(targetAngle - nowAngle)>0.2f){

        if (nowAngle - targetAngle < -angleChange){
            nowAngle = nowAngle + angleChange;
        }else if (nowAngle - targetAngle > angleChange){
            nowAngle = nowAngle - angleChange;
        }else{
            nowAngle = targetAngle;
        }
        _arm.setAngle(name, nowAngle);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
};

// Motion-control related, execute queued commands and call different functions based on the method
void MotionManager::executeMotion(const MotionTask& cmd){

    if (cmd.method == MoveMethod::ABS){
        move_joint_to_angle(cmd.joint,cmd.targetAngle,cmd.motionSpeed);
    }
    if (cmd.method == MoveMethod::REL){
        move_joint_with_val(cmd.joint,cmd.targetAngle,cmd.motionSpeed);
    }

}

// Refresh the internal motion list
void MotionManager::refresh_motion_list() {
    _available_motions.clear();
    if (!std::filesystem::exists(_motion_folder)) {
        std::filesystem::create_directory(_motion_folder);
        printf("[Init][MotionManager] Motion folder '%s' created.\n", _motion_folder.c_str());
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(_motion_folder)) {
        if (entry.path().extension() == ".json") {
            _available_motions.insert(entry.path().stem().string());
        }
    }
    std::cout << "[Init][MotionManager] Found " << _available_motions.size() << " motion sets in folder." << std::endl;
}

// Refresh the learned motion sets
void MotionManager::learn_motion_fresh() {
    refresh_motion_list();
};


void MotionManager::motionworker(){
    MotionTask cmd;

    while ((_isRunning) && (MotionQueue.pop(cmd))) {
            executeMotion(cmd);
    }
}

// Initialization
void MotionManager::servo_set_init(){
    _arm.IninServo();
}

// APP-side interface
// Execute a simple task
BugCode_M MotionManager::excuteTask(const std::string& cmd){
    MotionTask task;
    task = getMotionTask(cmd);
    // task.motionSpeed = 10;
    if(task.joint != Joint::UNKNOWN) 
        executeMotion(task);
        return BugCode_M::Success;
    return BugCode_M::UnkonwJoint;
}

// APP-side interface
// Execute a motion set
BugCode_M MotionManager::excuteMotionSet(const std::string& name){
    
    BugCode_M state = BugCode_M::Init;

    if (_available_motions.find(name) == _available_motions.end()) {
        std::cerr << "[Info][MotionManager] Motion set '" << name << "' not found in folder." << std::endl;
        state = BugCode_M::NoMotion;
#ifdef TESTMODE
        TaskEvent _taskevent;
        _taskevent.moduleName = "MotionSet";
        MotionResult _res;
        _res.name = name;
        _res.result = state;
        _taskevent.result = _res;
        _taskMonitor->postEvent(_taskevent);
#endif
        return state;
    }
    state = read_motion_set(name);
    return state;
}

// APP-side interface
// Reset joints
void MotionManager::excuteReset(){
    servo_set_init();
}

// APP-side interface
// Pause joints
void MotionManager::excuteStop(){
    if(_isRunning){
        _stopRequested = !_stopRequested;
    }
}

// At the MANA layer, when coordinates of a camera-detected object are received, perform 2D-to-3D processing and generate a motion set
void MotionManager::get_obj_MANA(int position_x, int position_y){

    Point3D res;
    res = _arm_calculator.pixelToBase(position_x,position_y,325);
    std::cout << "[MotionManager][Detect]" << res.x << "--" << res.y << "--" << res.z << std::endl;
    generate_motion(res);

}

// Select and run different built-in motion sets based on the 2D-to-3D result
void MotionManager::generate_motion(Point3D res){

    int x = res.x;
    int y = res.y;

    if ( x > 0 ){
        if ( x <50 ){
            read_motion_set("cat",MotionSetType::Inner);
        }else if (x < 100 ){
            read_motion_set("dog",MotionSetType::Inner);
        }else{
            read_motion_set("fish",MotionSetType::Inner);
        }
    }else{
        if ( x > -50 ){
            read_motion_set("monkey",MotionSetType::Inner);
        }else if( x > -100 ){
            read_motion_set("horse",MotionSetType::Inner);
        }else{
            read_motion_set("beef",MotionSetType::Inner);
        }
    }    
}

// Learning mode processing
// Execute actions and save them based on UI and microphone commands
// When a completion command is received, merge all tasks for the same joint and generate a motion set for saving
BugCode_M MotionManager::processLearningInput(const std::string& text, const std::string& name){

    BugCode_M state = BugCode_M::Init;

    if(is_first_learning){
        _currentLearningName = name;
        is_first_learning = false;
    }
    

#ifdef TESTMODE
    TaskEvent _taskevent;
    _taskevent.moduleName = "MotionSet";
    MotionResult _res;
#endif

    std::cout << "[Info][MotionManager][processLearningInput]:" << text << std::endl; 
    // Task dispatch executeTask

    // Keyboard task
    if (text == "CONFIRM") {
        state = saveMotionSet(_currentLearningName, _tempTasks);
        learning_error_code = 0;
#ifdef TESTMODE
        _res.name = _currentLearningName;
        _res.result = state;
        _taskevent.result = _res;
        _taskMonitor->postEvent(_taskevent);
#endif
        _currentLearningName = "";
        is_first_learning = true;
        return state;
    }

    // Key command
    std::string JiontMotion;

    const std::string do_prefix = "DOMOTION:";
    if (text.rfind(do_prefix, 0) == 0) {
        MotionTask task;
        JiontMotion = text.substr(do_prefix.size());
        
        
        task.method = MoveMethod::REL;
        task.motionSpeed = 50;
        if      (JiontMotion == "left_r") {task.joint = Joint::Base; task.targetAngle = 5.0f; }
        else if (JiontMotion == "left_l") {task.joint = Joint::Base; task.targetAngle = -5.0f; }
        else if (JiontMotion == "left_u") {task.joint = Joint::Shoulder; task.targetAngle = 5.0f; }
        else if (JiontMotion == "left_d") {task.joint = Joint::Shoulder; task.targetAngle = -5.0f; }
        else if (JiontMotion == "right_u") {task.joint = Joint::Elbow; task.targetAngle = 5.0f; }
        else if (JiontMotion == "right_d") {task.joint = Joint::Elbow; task.targetAngle = -5.0f; }
        else if (JiontMotion == "right_r") {task.joint = Joint::Hand; task.targetAngle = 5.0f; }
        else if (JiontMotion == "right_l") {task.joint = Joint::Hand; task.targetAngle = -5.0f; }
        // Execute the action immediately
        state = enqueue_motion(task);
        // Add to the buffer and wait for final merging
        _tempTasks.push_back(task); 
        learning_error_code = 0;
        if (state == BugCode_M::MotionQueError){
            _currentLearningName = "None";
            _tempTasks.clear();
            is_first_learning = true;
        }
#ifdef TESTMODE
        _res.name = _currentLearningName;
        _res.result = state;
        _taskevent.result = _res;
        _taskMonitor->postEvent(_taskevent);
#endif
        return state;
    }
    
    // Text command
    // Check whether the process should end
    if (text.find("done") != std::string::npos || text.find("stop") != std::string::npos || text.find("finish") != std::string::npos) {
        state = saveMotionSet(_currentLearningName, _tempTasks);
        learning_error_code = 0;
#ifdef TESTMODE
        _res.name = _currentLearningName;
        _res.result = state;
        _taskevent.result = _res;
        _taskMonitor->postEvent(_taskevent);
#endif
        _currentLearningName = "";
        is_first_learning = true;
        return state;
    }

    // Joint extraction
    if (text.find("base") != std::string::npos ) _currentJoint = Joint::Base;
    else if (text.find("shoulder") != std::string::npos ) _currentJoint = Joint::Shoulder;
    else if (text.find("elbow") != std::string::npos ) _currentJoint = Joint::Elbow;
    else if (text.find("hand") != std::string::npos ) _currentJoint = Joint::Hand;

    // Update
    float angleStep = 0.0f;

    if (text.find("right") != std::string::npos ) angleStep = 5.0f;
    else if (text.find("left") != std::string::npos ) angleStep = -5.0f;
    else if (text.find("up") != std::string::npos) angleStep = 5.0f;
    else if (text.find("down") != std::string::npos ) angleStep = -5.0f;
    else if (text.find("forward") != std::string::npos ) angleStep = 5.0f;
    else if (text.find("back") != std::string::npos ) angleStep = -5.0f;

    // Prepare for motion learning
    if (std::abs(angleStep) > 0.1f) {
        MotionTask task;
        task.joint = _currentJoint;
        task.method = MoveMethod::REL;
        task.targetAngle = angleStep;
        task.motionSpeed = 50;
        // Execute the action immediately
        state = enqueue_motion(task);
        // Add to the buffer and wait for final merging
        _tempTasks.push_back(task); 
        learning_error_code = 0;
        if (state == BugCode_M::MotionQueError){
            _currentLearningName = "None";
            _tempTasks.clear();
            is_first_learning = true;
        }
#ifdef TESTMODE
        _res.name = _currentLearningName;
        _res.result = state;
        _taskevent.result = _res;
        _taskMonitor->postEvent(_taskevent);
#endif
        return state;
    }
    
#ifdef TESTMODE
    _res.name = _currentLearningName;
    _res.result = state;
    _taskevent.result = _res;
    _taskMonitor->postEvent(_taskevent);
#endif
    learning_error_code++;
    check_error_code();
    return state;
};

// Added: clear tasks and reset the learning name when an unexpected save error occurs
BugCode_M MotionManager::saveMotionSet(std::string motionName, std::vector<MotionTask>& rawTasks){

    BugCode_M state = BugCode_M::Init;

    if (rawTasks.empty()) {
        state = BugCode_M::WriteInvalidSet;
        _tempTasks.clear();
        return state;
    }

    std::vector<MotionTask> mergedTasks;
    
    // Task merging
    for (const auto& task : rawTasks) {
        if (mergedTasks.empty()) {
            mergedTasks.push_back(task);
            continue;
        }

        auto& last = mergedTasks.back();
        // If the joint is the same and the movement direction is the same (same sign), merge them
        if (last.joint == task.joint && (last.targetAngle * task.targetAngle > 0)) {
            last.targetAngle += task.targetAngle;
        } else {
            mergedTasks.push_back(task);
        }
    }

    // Convert to JSON format and save 
    nlohmann::json j;
    j["name"] = motionName;
    for (const auto& task : mergedTasks) {
        nlohmann::json t;
        t["joint"] = JointName(task.joint); 
        t["method"] = "REL";
        t["val"] = task.targetAngle;
        t["speed"] = task.motionSpeed;
        j["tasks"].push_back(t);
    }

    std::ofstream file( _motion_folder + "/" + motionName + ".json");
    if (!file.is_open()){
        _tempTasks.clear();
        state = BugCode_M::CannotOpenMotionFile;
        is_first_learning = true;
        return state;
    }
    file << j.dump(4);
    
    // Refresh
    learn_motion_fresh();
    servo_set_init(); 
    state = BugCode_M::LearningSuccess;
    _tempTasks.clear();
    return state;
};

// Internal call, check the error count
// Exit learning mode if the error count exceeds the limit
bool MotionManager::check_error_code(){

    if(learning_error_code > 3){
#ifdef TESTMODE
        TaskEvent _taskevent;
        MotionResult _res;
        _taskevent.moduleName = "MotionSet";
        _res.name = "";
        _res.result = BugCode_M::TooMuchNoise;
        _taskevent.result = _res;
        _taskMonitor->postEvent(_taskevent);
#endif
        learning_error_code = 0;
    }
    return false;
}


// Conversion utility
std::string MotionManager::JointName(Joint joint){
    
    std::string name = "None" ;
    if (joint == Joint::Base) name = "Base";
    else if (joint == Joint::Elbow) name = "Elbow";
    else if (joint == Joint::Shoulder) name = "Shoulder";
    else if (joint == Joint::Hand) name = "Hand";

    return name;

}

// Conversion utility
Joint MotionManager::stringToJoint(const std::string& name) {
    if (name == "Base") return Joint::Base;
    if (name == "Elbow") return Joint::Elbow;
    if (name == "Shoulder") return Joint::Shoulder;
    if (name == "Hand") return Joint::Hand;
    return Joint::Base; // Default value
}

// Conversion utility
MoveMethod MotionManager::stringToMethod(const std::string& method) {
    if (method == "ABS") return MoveMethod::ABS;
    if (method == "REL") return MoveMethod::REL;
    return MoveMethod::REL;
}

// Conversion utility
MotionTask MotionManager::getMotionTask(const std::string& cmd) {
    auto it = motion_map.find(cmd);
    if (it != motion_map.end()) {
        return it->second;
    }
    return {Joint::UNKNOWN, MoveMethod::REL, 0.0f, 0};
}
