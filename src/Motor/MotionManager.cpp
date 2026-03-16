#include "MotionManager.hpp"

const std::unordered_map<std::string, MotionSet> MOTIONSET = {
    
    {
        "hello",
        {
            "hello",
            {
                {Joint::Base, MoveMethod::REL,  20.0f, 50},
                {Joint::Base, MoveMethod::REL, -20.0f, 50},
                {Joint::Base, MoveMethod::REL,  20.0f, 50},
                {Joint::Base, MoveMethod::REL, -20.0f, 50}
            }
        }
    },
};



MotionManager::MotionManager(const std::string& configFile):_arm(configFile){

    if (_arm.lastStatus != SUCCESS) {
            std::cerr << "failed to initialize robot arm controller, error code: "
                    << _arm.lastStatus << std::endl;
            _ready = false;
            _isRunning = false;
            _stopRequested = true;
            return;
        }

    std::cerr << "[Init] MotionManager init successfully"<< std::endl;
    _ready = true;
    _isRunning = true;
    _stopRequested = false;

    _motionworker = std::thread(&MotionManager::motionworker, this);

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

// motion指令相关，读取指令集并将指令加入队列
// 下版本更新
bool MotionManager::read_motion_set(const std::string& motion_set_name){

    
    auto it = MOTIONSET.find(motion_set_name);
    
    if (it == MOTIONSET.end()){
        return false;
    }

    const MotionSet& motion_set = it->second; 

    for (const auto& task : motion_set.tasks){
        enqueue_motion(task);
    }
    return true;
    
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