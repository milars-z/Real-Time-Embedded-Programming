#include "MotionManager.hpp"
#include "SystemCode.hpp"


MotionManager::MotionManager(std::atomic<int>& system_state, const std::string& configFile, const std::string& camera_config):
_arm(configFile), _arm_calculator(camera_config)
{

    if (_arm.lastStatus != SUCCESS) {
            std::cerr << "failed to initialize robot arm controller, error code: "
                    << _arm.lastStatus << std::endl;
            _ready = false;
            _isRunning = false;
            _stopRequested = true;
            system_state |= ERR_MOTION_INIT;
            return;
        }

    refresh_motion_list();
    
    std::cerr << "[Init] MotionManager init successfully"<< std::endl;
    _ready = true;
    _isRunning = true;
    _stopRequested = true;



};

MotionManager::~MotionManager() = default;

void MotionManager::start_thread(int core){

    _stopRequested = false;
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



BugCode_M MotionManager::read_motion_set(const std::string& motion_set_name, MotionSetType type) {

    // 找motion
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
    else if (joint == Joint::Hand) name = "Hand";

    return name;

}

Joint MotionManager::stringToJoint(const std::string& name) {
    if (name == "Base") return Joint::Base;
    if (name == "Elbow") return Joint::Elbow;
    if (name == "Shoulder") return Joint::Shoulder;
    if (name == "Hand") return Joint::Hand;
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
    _arm.IninServo();
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


void MotionManager::get_obj_MANA(int position_x, int position_y){

    Point3D res;

    res = _arm_calculator.pixelToBase(position_x,position_y,325);

    std::cout << res.x << "--" << res.y << "--" << res.z << std::endl;

    generate_motion(res);

}

void MotionManager::generate_motion(Point3D res){

    int x = res.x;
    int y = res.y;

    // right_one: cat
    // right_two: dog
    // right_three: fish
    // left_one: monkey
    // left_two: horse
    // left_three: beef

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

// 写代码就像拆炸弹，别写屎山了迟早重构
BugCode_M MotionManager::processLearningInput(const std::string& text, const std::string& name){

    BugCode_M state = BugCode_M::Init;
    _currentLearningName = name;

    
    std::cout << "[MotionManager]:" << text << std::endl; 
    // 任务下发 excuteTask

    // 键盘任务
    if (text == "CONFIRM") {
        state = saveMotionSet(_currentLearningName, _tempTasks);
        learning_error_code = 0;
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
        else if (JiontMotion == "right_r") {task.joint = Joint::Hand; task.targetAngle = 5.0f; }
        else if (JiontMotion == "right_l") {task.joint = Joint::Hand; task.targetAngle = -5.0f; }

        state = enqueue_motion(task);
        _tempTasks.push_back(task); 
        learning_error_code = 0;
        if (state == BugCode_M::MotionQueError){
            _currentLearningName = "None";
            _tempTasks.clear();
        }
        return state;
    }
    
    // text 指令
    // 检查是否结束
    if (text.find("done") != std::string::npos || text.find("stop") != std::string::npos || text.find("finish") != std::string::npos) {
        state = saveMotionSet(_currentLearningName, _tempTasks);
        learning_error_code = 0;
        return state;
    }

    // 关节提取
    if (text.find("base") != std::string::npos ) _currentJoint = Joint::Base;
    else if (text.find("shoulder") != std::string::npos ) _currentJoint = Joint::Shoulder;
    else if (text.find("elbow") != std::string::npos ) _currentJoint = Joint::Elbow;
    else if (text.find("hand") != std::string::npos ) _currentJoint = Joint::Hand;

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
        learning_error_code = 0;
        if (state == BugCode_M::MotionQueError){
            _currentLearningName = "None";
            _tempTasks.clear();
        }
        return state;
    }
    learning_error_code++;
    return state;
};

// 新增，意外保存错误时清空task并清空学习的名字
BugCode_M MotionManager::saveMotionSet(std::string motionName, std::vector<MotionTask>& rawTasks){

    BugCode_M state = BugCode_M::Init;

    if (rawTasks.empty()) {
        state = BugCode_M::WriteInvalidSet;
        _currentLearningName = "None";
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

    std::ofstream file( motionsetPath + "/" + motionName + ".json");
    if (!file.is_open()){
        _currentLearningName = "None";
        _tempTasks.clear();
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

// 外部调用，检查错误计数
// 超出限制数量则退出学习模式
bool MotionManager::check_error_code(){

    if(learning_error_code > 3){
        return true;
    }
    return false;
}