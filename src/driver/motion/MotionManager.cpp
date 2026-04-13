#include "MotionManager.hpp"

// const std::unordered_map<std::string, MotionSet> MOTIONSET = {
    
//     {
//         "hello",
//         {
//             "hello",
//             {
//                 {Joint::Base, MoveMethod::REL,  20.0f, 50},
//                 {Joint::Base, MoveMethod::REL, -20.0f, 50},
//                 {Joint::Base, MoveMethod::REL,  20.0f, 50},
//                 {Joint::Base, MoveMethod::REL, -20.0f, 50}
//             }
//         }
//     },
// };



MotionManager::MotionManager(const std::string& configFile):_arm(configFile){

    if (_arm.lastStatus != SUCCESS) {
            std::cerr << "failed to initialize robot arm controller, error code: "
                    << _arm.lastStatus << std::endl;
            _ready = false;
            _isRunning = false;
            _stopRequested = true;
            return;
        }

    refresh_motion_list();
    
    std::cerr << "[Init] MotionManager init successfully"<< std::endl;
    _ready = true;
    _isRunning = true;
    _stopRequested = false;

    // _motionworker = std::thread(&MotionManager::motionworker, this);
    // // _servoworker  = std::thread(&MotionManager::servoworker, this);
    // pinThreadToCore(_motionworker,"motion", 1);
    // // pinThreadToCore(_servoworker, "servo", 1);

};

MotionManager::~MotionManager() = default;

void MotionManager::start_thread(int core){

     _motionworker = std::thread(&MotionManager::motionworker, this);
    pinThreadToCore(_motionworker,"motion", core);
    

}

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

    if (!_ready ) return BugCode_M::MotionQueError;
    if (!_isRunning) return BugCode_M::MotionQueError;

    MotionQueue.push(cmd);
    
    return BugCode_M::Success;

};



BugCode_M MotionManager::read_motion_set(const std::string& motion_set_name) {

    // // 找name
    // if (_available_motions.find(motion_set_name) == _available_motions.end()) {
    //     std::cerr << "[Motion] Motion set '" << motion_set_name << "' not found in folder." << std::endl;
    //     return false;
    // }

    // 找motion
    BugCode_M state = BugCode_M::Init;
    std::string filePath = _motion_folder + "/" + motion_set_name + ".json";
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
            task.joint = stringToJoint(item["joint"]);
            task.method = stringToMethod(item["method"]);
            task.targetAngle = item["val"];
            task.motionSpeed = item["speed"];
            
            enqueue_motion(task);
        }
        std::cout << "[Motion] Loaded and enqueued: " << motion_set_name << std::endl;
        state = BugCode_M::Success;
        return state;

    } catch (nlohmann::json::parse_error& e) {
        std::cerr << "[Motion] Parse error in " << motion_set_name << ": " << e.what() << std::endl;
        state = BugCode_M::ReadInvalidSet;
        return state;
    }
}

// motion控制相关，链接底层pwm控制，将指定joint移动到指定角度
// 将详细指令push进pwm控制层的队列中
// 暂时不需要反馈，因此改成void
void MotionManager::move_joint_to_angle(Joint joint,float targetAngle,int motionSpeed){

    std::string name;
    float nowAngle;
    float angleChange;
    bool state = false;
    int step;
    ServoTask cmd;

    name = JointName(joint);
    // if (name == "None") return state;

    nowAngle = _arm.getAngle(name);
    angleChange = motionSpeed * 0.02f; 

    // if (angleChange <= 0.0f) return state;
    while(std::abs(targetAngle - nowAngle)>0.2f){
        // if (_stopRequested) break;

        if (nowAngle - targetAngle < -angleChange){
            nowAngle = nowAngle + angleChange;
        }else if (nowAngle - targetAngle > angleChange){
            nowAngle = nowAngle - angleChange;
        }else{
            nowAngle = targetAngle;
        }

        // state = _arm.setAngle(name, nowAngle);
        _arm.setAngle(name, nowAngle);
        // 后续追加小队列来解决sleep问题
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        // cmd.name = name;
        // cmd.nowAngle = nowAngle;
        // ServoQueue.push(cmd);
    }

    // return state;
};

// motion控制相关，链接底层pwm控制，将指定joint移动相对角度
void MotionManager::move_joint_with_val(Joint joint,float angleVal,int motionSpeed){

    std::string name;
    float nowAngle;
    float angleChange;
    float targetAngle;
    // bool state = false;
    int step;
    ServoTask cmd;

    name = JointName(joint);
    // if (name == "None") return state;

    nowAngle = _arm.getAngle(name);
    angleChange = motionSpeed * 0.02f;
    targetAngle = nowAngle + angleVal;

    // if (angleChange <= 0.0f) return state;
    while(std::abs(targetAngle - nowAngle)>0.2f){
        // if (_stopRequested) break;

        if (nowAngle - targetAngle < -angleChange){
            nowAngle = nowAngle + angleChange;
        }else if (nowAngle - targetAngle > angleChange){
            nowAngle = nowAngle - angleChange;
        }else{
            nowAngle = targetAngle;
        }

        // state = _arm.setAngle(name, nowAngle);

        _arm.setAngle(name, nowAngle);
        
        // 后续追加小队列来解决sleep问题
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        // // 追加队列
        // 队列有问题
        // cmd.name = name;
        // cmd.nowAngle = nowAngle;
        // ServoQueue.push(cmd);
    }
    printf("[Motion]now_joint:base,target_:%f\n",nowAngle);
    // return state;
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

//
void MotionManager::refresh_motion_list() {
    _available_motions.clear();
    if (!std::filesystem::exists(_motion_folder)) {
        std::filesystem::create_directory(_motion_folder);
        printf("[Motion] Motion folder '%s' created. Please add motion set JSON files and restart.\n", _motion_folder.c_str());
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(_motion_folder)) {
        if (entry.path().extension() == ".json") {
            // 将文件名（不含后缀）存入集合，例如 hello.json 存为 "hello"
            _available_motions.insert(entry.path().stem().string());
        }
    }
    std::cout << "[Init] MotionManager: Found " << _available_motions.size() << " motion sets in folder." << std::endl;
}

void MotionManager::learn_motion_fresh() {
    refresh_motion_list();
};



// // motion控制相关，重置当前状态
// 写在了excuteReset
// bool MotionManager::reset(){
//     return true;
// };

// motion控制相关，停止一切动作且清空指令集队列
bool MotionManager::stop_motion(){
    return true;
};

// motion线程，执行队列中的指令，与Pwm类接轨
// 其实是大任务分小任务，现已优化
void MotionManager::motionworker(){

    MotionTask cmd;
    while ((_isRunning) && (MotionQueue.pop(cmd))) {
        // cmd.motionSpeed = 10;
        executeMotion(cmd);
    }
}

// motion线程，执行详细任务，manager模块与pwm的唯一接口
// 需要保证上一个任务完成了再进行下一个任务
// void MotionManager::servoworker(){

//     ServoTask cmd;
//     while ((_isRunning) && (ServoQueue.pop(cmd)) && (!_stopRequested)) {
        
//         while( (abs(_arm.getAngle(_last_name) - _last_angle)< 0.20f) || servoworker_flag ){
//             _arm.setAngle(cmd.name,cmd.nowAngle);
//             _last_name  = cmd.name;
//             _last_angle = cmd.nowAngle;
//             servoworker_flag = false;
//         }
        

        
//     }
// }

// void MotionManager::servoworker() {
//     ServoTask cmd;

//     while (_isRunning && ! _stopRequested) {
//         if (!ServoQueue.pop(cmd)) {
//             continue;
//         }

//         // 第一次任务直接执行
//         if (servoworker_flag) {
//             _arm.setAngle(cmd.name, cmd.nowAngle);
//             _last_name = cmd.name;
//             _last_angle = cmd.nowAngle;
//             servoworker_flag = false;
//         } else {
//             // 等上一个完成
//             while (_isRunning && !_stopRequested &&
//                    std::abs(_arm.getAngle(_last_name) - _last_angle) >= 0.02f) {
//                 std::this_thread::sleep_for(std::chrono::milliseconds(5));
//             }

//             _arm.setAngle(cmd.name, cmd.nowAngle);
//             _last_name = cmd.name;
//             _last_angle = cmd.nowAngle;
//         }
//     }
// }

// motion工具，将指令集中的Joint类转换为string
std::string MotionManager::JointName(Joint joint){
    
    std::string name = "None" ;
    if (joint == Joint::Base) name = "Base";
    else if (joint == Joint::Elbow) name = "Elbow";
    else if (joint == Joint::Shoulder) name = "Shoulder";

    return name;

}

Joint MotionManager::stringToJoint(const std::string& name) {
    if (name == "Base") return Joint::Base;
    if (name == "Elbow") return Joint::Elbow;
    if (name == "Shoulder") return Joint::Shoulder;
    return Joint::Base; // 默认值
}

MoveMethod MotionManager::stringToMethod(const std::string& method) {
    if (method == "ABS") return MoveMethod::ABS;
    if (method == "REL") return MoveMethod::REL;
    return MoveMethod::REL;
}

// 初始化
void MotionManager::servo_set_init()
{
    move_joint_to_angle(Joint::Base, 90.0f, 100);
    move_joint_to_angle(Joint::Shoulder, 0.0f, 100);
    move_joint_to_angle(Joint::Elbow, 45.0f, 100);
}

// string -> MotionTask
// 抽象指令到具象指令的转换
// 后续直接可直接升级
MotionTask MotionManager::getMotionTask(const std::string& cmd) {
    auto it = motion_map.find(cmd);
    if (it != motion_map.end()) {
        return it->second;
    }
    return {Joint::UNKNOWN, MoveMethod::REL, 0.0f, 0};
}

// 执行简单任务
// APP侧接口
BugCode_M MotionManager::excuteTask(const std::string& cmd){
    MotionTask task;
    task = getMotionTask(cmd);
    // task.motionSpeed = 10;
    if(task.joint != Joint::UNKNOWN) 
        executeMotion(task);
        return BugCode_M::Success;
    return BugCode_M::UnkonwJoint;
}

// 执行指令集
// APP侧接口
BugCode_M MotionManager::excuteMotionSet(const std::string& name){
    
    BugCode_M state = BugCode_M::Init;

    // 找name
    if (_available_motions.find(name) == _available_motions.end()) {
        std::cerr << "[Motion] Motion set '" << name << "' not found in folder." << std::endl;
        state = BugCode_M::NoMotion;
        return state;
    }

    state = read_motion_set(name);
    return state;

}

void MotionManager::excuteReset(){
    
    servo_set_init();

}

void MotionManager::excuteStop(){

    // 怎么实现？
    // 状态反转
    if(_isRunning){
        _stopRequested = !_stopRequested;
    }
    

}


// 写代码就像拆炸弹，别写屎山了迟早重构
BugCode_M MotionManager::processLearningInput(const std::string& text, const std::string& name){

    BugCode_M state = BugCode_M::Init;
    _currentLearningName = name;

    
    std::cout << "[MotionManager]:" << text << std::endl; 
    // 任务下发 excuteTask

    // 键盘任务
    if (text == "CONFIRM") {
        state = saveMotionSet(_currentLearningName, _tempTasks);
        return state;
    }

    // key 指令
    std::string JiontMotion;

    const std::string do_prefix = "DOMOTION:";
    if (text.rfind(do_prefix, 0) == 0) {
        MotionTask task;
        JiontMotion = text.substr(do_prefix.size());
        // 执行动作
        
        task.method = MoveMethod::REL;
        task.motionSpeed = 50;
        if      (JiontMotion == "left_r") {task.joint = Joint::Base; task.targetAngle = 5.0f; }
        else if (JiontMotion == "left_l") {task.joint = Joint::Base; task.targetAngle = -5.0f; }
        else if (JiontMotion == "left_u") {task.joint = Joint::Shoulder; task.targetAngle = 5.0f; }
        else if (JiontMotion == "left_d") {task.joint = Joint::Shoulder; task.targetAngle = -5.0f; }
        else if (JiontMotion == "right_u") {task.joint = Joint::Elbow; task.targetAngle = 5.0f; }
        else if (JiontMotion == "right_d") {task.joint = Joint::Elbow; task.targetAngle = -5.0f; }
        // else if (JiontMotion == "right_r") {task.joint = Joint::Hand; task.targetAngle = 5.0f; }
        // else if (JiontMotion == "right_l") {task.joint = Joint::Hand; task.targetAngle = -5.0f; }

        state = enqueue_motion(task);
        _tempTasks.push_back(task); 
        return state;
    }
    
    // text 指令
    // 检查是否结束
    if (text.find("done") != std::string::npos || text.find("stop") != std::string::npos || text.find("finish") != std::string::npos) {
        state = saveMotionSet(_currentLearningName, _tempTasks);
        return state;
    }

    // 关节提取
    if (text.find("base") != std::string::npos ) _currentJoint = Joint::Base;
    else if (text.find("shoulder") != std::string::npos ) _currentJoint = Joint::Shoulder;
    else if (text.find("elbow") != std::string::npos ) _currentJoint = Joint::Elbow;

    // 更新
    float angleStep = 0.0f;

        // 暂时固定5度
    // 后续可接收多角度，暂时用这个测试
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
        return state;
    }
    
    return state;
};


BugCode_M MotionManager::saveMotionSet(std::string motionName, std::vector<MotionTask>& rawTasks){

    BugCode_M state = BugCode_M::Init;

    if (rawTasks.empty()) {
        state = BugCode_M::WriteInvalidSet;
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
    for (const auto& task : rawTasks) {
        nlohmann::json t;
        t["joint"] = JointName(task.joint); 
        t["method"] = "REL";
        t["val"] = task.targetAngle;
        t["speed"] = task.motionSpeed;
        j["tasks"].push_back(t);
    }

    std::ofstream file( motionsetPath + "/" + motionName + ".json");
    if (!file.is_open()){
        state = BugCode_M::CannotOpenMotionFile;
        return state;
    }
    file << j.dump(4);
    
    // 刷新
    learn_motion_fresh();
    servo_set_init(); 
    state = BugCode_M::LearningSuccess;
    _currentLearningName = "None";
    _tempTasks.clear();
    return state;
};