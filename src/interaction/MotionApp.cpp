#include "MotionApp.hpp"

#include "MotionManager.hpp"
#include "MotionHandle.hpp"
#include "config_voice.hpp" 

#include <nlohmann/json.hpp>
#include <iostream>



// 需要考虑一个问题，这个模块到底管理哪一部分任务？
// 任务层是不是需要负责任务的学习比较好？
// brain只负责改变状态，如果是学习状态则直接把所有信息传给motion，motion来负责学习？

MotionExecutor::MotionExecutor() {

    manager = std::make_unique<MotionManager>(Config::Motion::MOTION_CONFIG);
    std::cout << "[MotionExecutor] 动作管理器已初始化" << std::endl;

    pinThreadToCore(worker,"MotionTask",1);
}

MotionExecutor::~MotionExecutor() = default; 

void MotionExecutor::onExecute(const std::string& task) {

    if (!manager) return;

    // 这里执行motion相关的内容
    // 收到的指令如下
    // 这个模块只负责指导机器人动作！因此learnmotion，confirm都不需要在这里
    // 只需要知道，如果是单一motion，就移动，motionset，就执行整个set，reset，就重置，stop，就停！
    // 你说的不对，这一个模块应该也负责learning和confirm，大脑只负责将task分到motion，speaker，camera三个模块
    // DO_MOTION:
    // MOTIONSET:
    // RESET
    // STOP
    // LEARNMOTION
    // CONFIRM

    // 小动作为
    // left/right + u/d/r/l

    // Motion还是一样，直接简单分词

    // 每次执行还是先清空下，以免上一个动作的obj没删

    // 非学习模式
    if(armMode == ARMMODE::IDLE){
        
        MotionCommand cmd;
        BugCode_M state;

        std::cout << "[Motion] 正在执行动作: " << task << std::endl;
        cmd = analyzecommand(task);
        // 简易动作
        if (cmd.command == "DO_MOTION"){
            manager->excuteTask(cmd.obj);
        }
        // motion set
        else if(cmd.command == "MOTIONSET"){
            state = manager->excuteMotionSet(cmd.obj);
            HandleState(state);
        }
        // 重置
        else if(cmd.command == "RESET"){
            manager->excuteReset();
        }
        // 暂停
        else if(cmd.command == "STOP"){
            manager->excuteStop();
        }
        // 开启学习模式
        else if(cmd.command == "LEARNMOTION"){
            armMode == ARMMODE::LEARNING;
        }
        // 确定，一般不会走到这里
        else if(cmd.command == "CONFIRM"){
            armMode == ARMMODE::IDLE;
        }
    }
    // 学习模式，在这个模式下只会接收简易动作
    // 做的事情有两件，让下级执行动作与记录motionset
    else if(armMode == ARMMODE::LEARNING){
        
        BugCode_M state;
        // 从这里开始改 0410
        state = processLearningInput(task);

    }
    


}

// 学习阶段的时候用，进来一个格式化text，经过小分词器直接执行任务
void MotionExecutor::doDirectTask(const MotionTask& t) {
    if (!manager) return;


}

MotionCommand MotionExecutor::analyzecommand(const std::string& text){

    MotionCommand cmd;

    if (text == "RESET") {
        cmd.command = "RESET";
        cmd.obj = "";
        return cmd;
    }

    if (text == "STOP") {
        cmd.command = "STOP";
        cmd.obj = "";
        return cmd;
    }

    if (text == "CONFIRM") {
        cmd.command = "CONFIRM";
        cmd.obj = "";
        return cmd;
    }

    const std::string do_prefix = "DO_MOTION:";
    if (text.rfind(do_prefix, 0) == 0) {
        cmd.command = "DO_MOTION";
        cmd.obj = text.substr(do_prefix.size());
        return cmd;
    }

    const std::string set_prefix = "MOTIONSET:";
    if (text.rfind(set_prefix, 0) == 0) {
        cmd.command = "MOTIONSET";
        cmd.obj = text.substr(set_prefix.size());
        return cmd;
    }

    const std::string learn_prefix = "LEARNMOTION:";
    if (text.rfind(set_prefix, 0) == 0) {
        cmd.command = "LEARNMOTION";
        cmd.obj = text.substr(learn_prefix.size());
        return cmd;
    }

    return cmd;
}

void MotionExecutor::HandleState(BugCode_M msg){

    // 后面再加

}


BugCode_M MotionExecutor::processLearningInput(const std::string& text){

    BugCode_M state = BugCode_M::Init;
    // 检查是否结束
    if (text.find("done") != std::string::npos || text.find("stop") != std::string::npos || text.find("finish") != std::string::npos) {
        state = saveMotionSet(_currentLearningName, _tempTasks);
        armMode = ARMMODE::IDLE;
        return state;
    }

    // 关节提取
    if (text.find("base") != std::string::npos || text.find("base") != std::string::npos) _currentJoint = Joint::Base;
    else if (text.find("shoulder") != std::string::npos || text.find("shoulder") != std::string::npos) _currentJoint = Joint::Shoulder;
    else if (text.find("elbow") != std::string::npos || text.find("elbow") != std::string::npos) _currentJoint = Joint::Elbow;

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

    if (std::abs(angleStep) > 0.1f) {
        MotionTask task;
        task.joint = _currentJoint;
        task.method = MoveMethod::REL;
        task.targetAngle = angleStep;
        task.motionSpeed = 50;

        // 立即执行动作
        // manager->enqueue_motion(task);
        // 加入缓存等待最后合并
        _tempTasks.push_back(task); 
    }

};


BugCode_M MotionExecutor::saveMotionSet(std::string motionName, std::vector<MotionTask>& rawTasks){

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
    for (const auto& task : mergedTasks) {
        nlohmann::json t;
        t["joint"] = manager->JointName(task.joint); 
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
    manager->learn_motion_fresh();
    manager->servo_set_init(); 
    state = BugCode_M::Success;
    return state;
};