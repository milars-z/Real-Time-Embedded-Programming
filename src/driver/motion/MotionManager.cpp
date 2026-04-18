#include "MotionManager.hpp"
#include "SystemCode.hpp"

// 构造函数
// 构造底层engine
// 刷新motion_set
// 启动状态
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

// 外部调用，线程相关
void MotionManager::start_thread(int core){

    _stopRequested = false;
     _motionworker = std::thread(&MotionManager::motionworker, this);
    pinThreadToCore(_motionworker,"motion", core);

}

// 外部调用，线程相关
void MotionManager::stop_thread(){
    _isRunning = false;
    _stopRequested = true;
    MotionQueue.stop();

    if (_motionworker.joinable()) {
        _motionworker.join();
    }
}

// 线程队列相关，在线程queue中追加指令集
BugCode_M MotionManager::enqueue_motion(const MotionTask& cmd){

    if (!_isRunning) return BugCode_M::MotionQueError;
    MotionQueue.push(cmd);
    
    return BugCode_M::Success;

};

// 获取motion_set中的指令并执行
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
            // 配置默认值以防读取错误
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

// motion控制相关，链接底层pwm控制，将指定joint移动到指定角度
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

// motion控制相关，链接底层pwm控制，将指定joint移动相对角度
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

// motion控制相关，队列指令执行，通过method来调用不同函数
void MotionManager::executeMotion(const MotionTask& cmd){

    if (cmd.method == MoveMethod::ABS){
        move_joint_to_angle(cmd.joint,cmd.targetAngle,cmd.motionSpeed);
    }
    if (cmd.method == MoveMethod::REL){
        move_joint_with_val(cmd.joint,cmd.targetAngle,cmd.motionSpeed);
    }

}

// 刷新内部motion_list
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

// 刷新已学到的motion指令集
void MotionManager::learn_motion_fresh() {
    refresh_motion_list();
};


void MotionManager::motionworker(){
    MotionTask cmd;

    while ((_isRunning) && (MotionQueue.pop(cmd))) {
            executeMotion(cmd);
    }
}

// 初始化
void MotionManager::servo_set_init(){
    _arm.IninServo();
}

// APP侧接口
// 执行简单任务
BugCode_M MotionManager::excuteTask(const std::string& cmd){
    MotionTask task;
    task = getMotionTask(cmd);
    // task.motionSpeed = 10;
    if(task.joint != Joint::UNKNOWN) 
        executeMotion(task);
        return BugCode_M::Success;
    return BugCode_M::UnkonwJoint;
}

// APP侧接口
// 执行指令集
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

// APP侧接口
// Joint重置
void MotionManager::excuteReset(){
    servo_set_init();
}

// APP侧接口
// Joint暂停
void MotionManager::excuteStop(){
    if(_isRunning){
        _stopRequested = !_stopRequested;
    }
}

// MANA层，当收到camera检测到的物体的坐标后进行2D->3D处理并生成motion_set
void MotionManager::get_obj_MANA(int position_x, int position_y){

    Point3D res;
    res = _arm_calculator.pixelToBase(position_x,position_y,325);
    std::cout << "[MotionManager][Detect]" << res.x << "--" << res.y << "--" << res.z << std::endl;
    generate_motion(res);

}

// 根据2D-3D后的结果选择运行不同的内置motionset
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

// 学习模式处理
// 根据UI的指令和Mic的指令进行动作并保存
// 当收到完成指令后合并所有相同的joint并生成motionset进行保存
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
    // 任务下发 excuteTask

    // 键盘任务
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

    // key 指令
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
        // 立即执行动作
        state = enqueue_motion(task);
        // 加入缓存等待最后合并
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
    
    // text 指令
    // 检查是否结束
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

    // 关节提取
    if (text.find("base") != std::string::npos ) _currentJoint = Joint::Base;
    else if (text.find("shoulder") != std::string::npos ) _currentJoint = Joint::Shoulder;
    else if (text.find("elbow") != std::string::npos ) _currentJoint = Joint::Elbow;
    else if (text.find("hand") != std::string::npos ) _currentJoint = Joint::Hand;

    // 更新
    float angleStep = 0.0f;

    if (text.find("right") != std::string::npos ) angleStep = 5.0f;
    else if (text.find("left") != std::string::npos ) angleStep = -5.0f;
    else if (text.find("up") != std::string::npos) angleStep = 5.0f;
    else if (text.find("down") != std::string::npos ) angleStep = -5.0f;
    else if (text.find("forward") != std::string::npos ) angleStep = 5.0f;
    else if (text.find("back") != std::string::npos ) angleStep = -5.0f;

    // 任务学习准备
    if (std::abs(angleStep) > 0.1f) {
        MotionTask task;
        task.joint = _currentJoint;
        task.method = MoveMethod::REL;
        task.targetAngle = angleStep;
        task.motionSpeed = 50;
        // 立即执行动作
        state = enqueue_motion(task);
        // 加入缓存等待最后合并
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

// 新增，意外保存错误时清空task并清空学习的名字
BugCode_M MotionManager::saveMotionSet(std::string motionName, std::vector<MotionTask>& rawTasks){

    BugCode_M state = BugCode_M::Init;

    if (rawTasks.empty()) {
        state = BugCode_M::WriteInvalidSet;
        _tempTasks.clear();
        return state;
    }

    std::vector<MotionTask> mergedTasks;
    
    // 任务合并
    for (const auto& task : rawTasks) {
        if (mergedTasks.empty()) {
            mergedTasks.push_back(task);
            continue;
        }

        auto& last = mergedTasks.back();
        // 如果关节相同，且移动方向相同（正负号一致），则合并
        if (last.joint == task.joint && (last.targetAngle * task.targetAngle > 0)) {
            last.targetAngle += task.targetAngle;
        } else {
            mergedTasks.push_back(task);
        }
    }

    // 转化为 JSON 格式保存 
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
    
    // 刷新
    learn_motion_fresh();
    servo_set_init(); 
    state = BugCode_M::LearningSuccess;
    _tempTasks.clear();
    return state;
};

// 内部调用，检查错误计数
// 超出限制数量则退出学习模式
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


// 转换工具
std::string MotionManager::JointName(Joint joint){
    
    std::string name = "None" ;
    if (joint == Joint::Base) name = "Base";
    else if (joint == Joint::Elbow) name = "Elbow";
    else if (joint == Joint::Shoulder) name = "Shoulder";
    else if (joint == Joint::Hand) name = "Hand";

    return name;

}

// 转换工具
Joint MotionManager::stringToJoint(const std::string& name) {
    if (name == "Base") return Joint::Base;
    if (name == "Elbow") return Joint::Elbow;
    if (name == "Shoulder") return Joint::Shoulder;
    if (name == "Hand") return Joint::Hand;
    return Joint::Base; // 默认值
}

// 转换工具
MoveMethod MotionManager::stringToMethod(const std::string& method) {
    if (method == "ABS") return MoveMethod::ABS;
    if (method == "REL") return MoveMethod::REL;
    return MoveMethod::REL;
}

// 转换工具
MotionTask MotionManager::getMotionTask(const std::string& cmd) {
    auto it = motion_map.find(cmd);
    if (it != motion_map.end()) {
        return it->second;
    }
    return {Joint::UNKNOWN, MoveMethod::REL, 0.0f, 0};
}