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

    _motionworker = std::thread(&MotionManager::motionworker, this);
    pinThreadToCore(_motionworker,"motion", 1);

};

MotionManager::~MotionManager(){
    
    _isRunning = false;
    _stopRequested = true;
    MotionQueue.stop();

    if (_motionworker.joinable()) {
        _motionworker.join();
    }
};

// 线程队列相关，在线程queue中追加指令集
bool MotionManager::enqueue_motion(const MotionTask& cmd){

    if (!_ready ) return false;
    if (!_isRunning) return false;

    MotionQueue.push(cmd);
    
    return true;

};

// motion指令相关，创建指令
// 下版本更新
void MotionManager::create_motion(std::string motion_name){

}

// motion指令相关，创建指令集
// 下版本更新
void MotionManager::create_motion_set(std::string motion_set_name){

}

// // motion指令相关，读取指令集并将指令加入队列
// // 下版本更新
// bool MotionManager::read_motion_set(const std::string& motion_set_name){

    
//     auto it = MOTIONSET.find(motion_set_name);
    
//     if (it == MOTIONSET.end()){
//         return false;
//     }

//     const MotionSet& motion_set = it->second; 

//     for (const auto& task : motion_set.tasks){
//         enqueue_motion(task);
//     }
//     return true;
    
// }

bool MotionManager::read_motion_set(const std::string& motion_set_name) {

    // 找name
    if (_available_motions.find(motion_set_name) == _available_motions.end()) {
        std::cerr << "[Motion] Motion set '" << motion_set_name << "' not found in folder." << std::endl;
        return false;
    }

    // 找motion
    std::string filePath = _motion_folder + "/" + motion_set_name + ".json";
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

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
        return true;

    } catch (nlohmann::json::parse_error& e) {
        std::cerr << "[Motion] Parse error in " << motion_set_name << ": " << e.what() << std::endl;
        return false;
    }
}

// motion控制相关，链接底层pwm控制，将指定joint移动到指定角度
bool MotionManager::move_joint_to_angle(Joint joint,float targetAngle,int motionSpeed){

    std::string name;
    float nowAngle;
    float angleChange;
    bool state = false;
    int step;

    name = JointName(joint);
    if (name == "None") return state;

    nowAngle = _arm.getAngle(name);
    angleChange = motionSpeed * 0.02f; 

    if (angleChange <= 0.0f) return state;
    while(std::abs(targetAngle - nowAngle)>0.1f){
        if (_stopRequested) break;

        if (nowAngle - targetAngle < -angleChange){
            nowAngle = nowAngle + angleChange;
        }else if (nowAngle - targetAngle > angleChange){
            nowAngle = nowAngle - angleChange;
        }else{
            nowAngle = targetAngle;
        }

        state = _arm.setAngle(name, nowAngle);
        // 后续追加小队列来解决sleep问题
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    return state;
};

// motion控制相关，链接底层pwm控制，将指定joint移动相对角度
bool MotionManager::move_joint_with_val(Joint joint,float angleVal,int motionSpeed){

    std::string name;
    float nowAngle;
    float angleChange;
    float targetAngle;
    bool state = false;
    int step;

    name = JointName(joint);
    if (name == "None") return state;

    nowAngle = _arm.getAngle(name);
    angleChange = motionSpeed * 0.02f;
    targetAngle = nowAngle + angleVal;

    if (angleChange <= 0.0f) return state;
    while(std::abs(targetAngle - nowAngle)>0.1f){
        if (_stopRequested) break;

        if (nowAngle - targetAngle < -angleChange){
            nowAngle = nowAngle + angleChange;
        }else if (nowAngle - targetAngle > angleChange){
            nowAngle = nowAngle - angleChange;
        }else{
            nowAngle = targetAngle;
        }

        state = _arm.setAngle(name, nowAngle);
        
        // 后续追加小队列来解决sleep问题
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    printf("[Motion]now_joint:base,target_:%f\n",nowAngle);
    return state;
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



// motion控制相关，重置当前状态
bool MotionManager::reset(){
    return true;
};

// motion控制相关，停止一切动作且清空指令集队列
bool MotionManager::stop(){
    return true;
};

// motion线程，执行队列中的指令，与Pwm类接轨
void MotionManager::motionworker(){

    MotionTask cmd;

    while ((_isRunning) && (MotionQueue.pop(cmd))) {
        executeMotion(cmd);

    }
}

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