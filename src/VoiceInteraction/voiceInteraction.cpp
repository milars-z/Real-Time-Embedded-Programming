#include "config_voice.hpp"
#include "VisonTools.hpp"
#include "voiceInteraction.hpp"
#include <iostream>
#include <chrono>


// 构造函数
RobotCore::RobotCore() : motionManager("../../Motor/servo_config.txt"),
      isRunning(false) {
}

// 析构函数 -停止所有相关模块运行
RobotCore::~RobotCore() {
    stop(); 
}

// 初始化
bool RobotCore::init() {

    // Find device name
    std::string speaker_path = find_alsa_device(Config::Hardware::SPEAKER_NAME);
    std::string mic_path = find_alsa_device(Config::Hardware::MIC_NAME);
    if (speaker_path.empty() || mic_path.empty()) {
        std::cerr << "[Error] Devices not found!" << std::endl;
        return false;
    }

    // Init NLU
    nlu = new NLUEngine(Config::Path::NLU_MODEL_DIR);
    if (!nlu->init()) {
        std::cerr << "[Error] NLUEngine failed." << std::endl;
        goto fail;
    }

    // Init Vosk
    voskModel = vosk_model_new(Config::Path::VOSK_MODEL_DIR.c_str());
    if (!voskModel) {
        std::cerr << "[Error] Vosk model load failed" << std::endl;
        goto fail;
    }
    recognizer = vosk_recognizer_new(voskModel, 16000.0);
    if(!recognizer){
        std::cerr << "[Error] vosk_recognizer load failed" << std::endl;
        goto fail;
    }

    // Init Speaker
    speaker = new UsbSpeaker(speaker_path, Config::Path::SPEAKER_MODELS, 2, 0);
    if (!speaker->open()) {
        std::cerr << "[Error] Speaker open failed" << std::endl;
        goto fail;
    }

    // Init Microphone
    mic = new UsbMicrophone(mic_path, 16000, 1);
    if (!mic->open()) {
        std::cerr << "[Error] Mic open failed" << std::endl;
        goto fail;
    }

    cam = new CameraHandle("../../Camera/model/mobilenet_v2_slice.onnx", "../../Camera/feature/my_features.yml");
    if (!cam->open()) {
        std::cerr << "[Error] Camera open failed" << std::endl;
        goto fail;
    }

    std::cout << "[Init] System initialized successfully." << std::endl;
    return true;

fail:
    if(mic)       { delete mic; mic = nullptr;}
    if(speaker)   { delete speaker; speaker = nullptr;}
    if(recognizer){ vosk_recognizer_free(recognizer); recognizer = nullptr;}
    if(voskModel) { vosk_model_free(voskModel); voskModel = nullptr;}
    if(nlu)       { delete nlu; nlu = nullptr; }
    if(cam)       { delete cam; cam = nullptr; }
    return false;
}

// 启动
bool RobotCore::start() {
    if (isRunning) return true;
    isRunning = true;

    // 重置队列状态
    textInputQueue.reset();
    speechOutputQueue.reset();

    try{
        nluThread = std::thread(&RobotCore::nluWorker,this);
        speakerThread = std::thread(&RobotCore::speakerWorker,this);
        pinThreadToCore(nluThread,"nlu", 3);
        pinThreadToCore(speakerThread,"speaker", 3);
    }catch (const std::system_error& e){
        std::cerr<<"[Fatat] Thread speaker and nlu error: " << e.what()<< std::endl;
        stopInternal();
        return false;
    }

    bool micStarted = mic->start([this] (const std::vector<short>& data){
        this -> audioCallback(data);
    });
    if (!micStarted){
        stopInternal();
        return false;
    }

    std::cout << "[System] Started." << std::endl;
    return true;
}

// 停止
void RobotCore::stop() {
    if (!isRunning) return;
    
    std::cout << "[System] Stopping..." << std::endl;
    isRunning = false;

    // 停止麦克风
    if (mic) mic->stop();

    // 停止队列
    textInputQueue.stop();
    speechOutputQueue.stop();

    // 停止线程
    if (nluThread.joinable()) nluThread.join();
    if (speakerThread.joinable()) speakerThread.join();

    // 关闭ALSA
    if (speaker) speaker->stop(); 

    // 释放vosk
    if (recognizer) { vosk_recognizer_free(recognizer); recognizer = nullptr; }
    if (voskModel)  { vosk_model_free(voskModel); voskModel = nullptr; }

    // 关闭Camera
    if (cam) cam->stop();

    // 删除对象
    delete mic; mic = nullptr;
    delete speaker; speaker = nullptr;
    delete nlu; nlu = nullptr;
    delete cam; cam = nullptr;

    std::cout << "[System] Stopped and resources cleaned." << std::endl;
}

bool RobotCore::running() const {
    return isRunning;
}

// use in RobotCore::start
void RobotCore::stopInternal() {
    isRunning = false; 
    textInputQueue.stop();
    speechOutputQueue.stop();
    if (nluThread.joinable()) nluThread.join();
    if (speakerThread.joinable()) speakerThread.join();
    if(mic) mic->stop();
    if(cam) cam->stop();

}


// microphone callback
void RobotCore::audioCallback(const std::vector<short>& data) {
    if (vosk_recognizer_accept_waveform(recognizer, 
                                       (const char*)data.data(), 
                                       data.size() * sizeof(short))) {
        
        std::string result = vosk_recognizer_result(recognizer);
        std::string text = extractText(result); 

        if (!text.empty()) {
            std::cout << "[Mic] Heard: " << text << std::endl;
            textInputQueue.push(text); 
        }
    }
}

// 重构，设计为一个整体状态机模式
void RobotCore::nluWorker() {
    std::string text;
    while (isRunning) {
        if (!textInputQueue.pop(text)) break;

        std::string responseText = "";
        MotionTask direct_task;

        // 又是个屎山
        // 应该外置一个状态机统一管理
        if (_isLearningMode) {
            // 开始学习
            processLearningInput(text, responseText);
        }else{
        // 测试用
            if (motionHandle.parseMotionCommand(text, direct_task)) {
                motionManager.enqueue_motion(direct_task);
                responseText = "get motion command";
            } 
            else {
                // 进入状态机分发
                nlu_output nlu_raw = nlu->predict(text);
                printf("[NLU] Intent: %s, Value: %s\n", nlu_raw.intent.c_str(), nlu_raw.currentValue.c_str());
                RobotIntent intent = mapToIntent(nlu_raw.intent);
                std::string val = nlu_raw.currentValue;

                switch (intent) {
                    case RobotIntent::DO_MOTION:
                    // 一直检测不到，做一个保险，后面再去做nlu
                        if (val.empty()){
                            get_motion_name_from_text(text, val);
                        }
                        handleDoMotion(val, responseText);
                        break;
                    case RobotIntent::LEARN_MOTION:
                        handleLearnMotion(val, responseText);
                        break;
                    case RobotIntent::FIND_OBJ:
                        handleFindObj(val, responseText);
                        break;
                    case RobotIntent::LEARN_OBJ:
                        handleLearnObj(val, responseText);
                        break;
                    case RobotIntent::GREET:
                        responseText = "hello!";
                        motionManager.read_motion_set("hello");
                        break;
                    case RobotIntent::SERVO_INIT:
                        motionManager.servo_set_init();
                        responseText = "servos initialized";
                        break;
                    case RobotIntent::NORMAL:
                    default:
                        responseText = "Sorry, I didn't understand that. Could you please rephrase?";
                        break;
                }
            }

            // 统一语音反馈
            if (!responseText.empty()) {
                speechOutputQueue.push(responseText);
            }
        }
    }
}


// speaker thread
void RobotCore::speakerWorker() {
    std::string textToPlay;
    while (isRunning) {
        // wait for NLU--speechOutputQueue
        if (speechOutputQueue.pop(textToPlay)) {
            speaker->play(textToPlay);
        } else {
            break;
        }
    }
}

// 状态机配置
RobotIntent RobotCore::mapToIntent(const std::string& nlu_intent) {
    if (nlu_intent == "do_motion")    return RobotIntent::DO_MOTION;
    if (nlu_intent == "learn_motion") return RobotIntent::LEARN_MOTION;
    if (nlu_intent == "find_obj")     return RobotIntent::FIND_OBJ;
    if (nlu_intent == "learn_obj")    return RobotIntent::LEARN_OBJ;
    if (nlu_intent == "greet")        return RobotIntent::GREET;
    return RobotIntent::NORMAL;
}

// 状态：执行动作逻辑
void RobotCore::handleDoMotion(const std::string& val, std::string& response) {
    if (motionManager.read_motion_set(val)) {
        response = "do motion " + val;
    } else {
        response = "can't find motion " + val;
    }
}

// 状态：学习动作逻辑
// 修改_currentLearningName 的值，并设置_isLearningMode为true，进入学习状态
void RobotCore::handleLearnMotion(const std::string& val, std::string& response) {
    _isLearningMode = true;
    _currentLearningName = val;
    _tempTasks.clear();
    response = " now start to learn " + val;
}

// 状态：寻找物体逻辑
void RobotCore::handleFindObj(const std::string& val, std::string& response) {
    if (val.empty()) {
        response = "find what?";
        return;
    }
    cam->Find_obj(val);
    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ObjPosition pos = cam->getObjectPosition();
    printf("Object Position - x: %d, y: %d\n", pos.x, pos.y);
    if (pos.x != -1 && pos.y != -1) {
        if (pos.x < 320) {
            response +=  val + " is on the left. ";
        } else {
            response +=  val + " is on the right. ";
        }
    } else {
        response = "can't find " + val;
    }
}

// 状态：学习物体逻辑
void RobotCore::handleLearnObj(const std::string& val, std::string& response) {
    if (val.empty()) {
        response = "learn what?";
        return;
    }
    cam->Learn_obj(val);
    response = "learn object " + val;
}

// 状态：一般
void RobotCore::handleNormal(const std::string& text, std::string& response) {
    response = "Normal model is not ready yet.";
}

// 堆屎山代码来了
// 这块应该放在handle中，但是要调用motionmanager中的函数，层级在最开始的时候没设计好
// 希望会有重构吧
void RobotCore::processLearningInput(const std::string& text, std::string& response){

    // 检查是否结束
    if (text.find("done") != std::string::npos || text.find("stop") != std::string::npos || text.find("finish") != std::string::npos) {
        if (saveMotionSet(_currentLearningName, _tempTasks)) {
            response = "learn motion " + _currentLearningName;
        } else {
            response = "save motion failed";
        }
        _isLearningMode = false;
        return;
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
        motionManager.enqueue_motion(task);
        // 加入缓存等待最后合并
        _tempTasks.push_back(task);
        response = "done"; 
    }

};

// 这也是屎山
// 保存motionset
bool RobotCore::saveMotionSet(std::string motionName, std::vector<MotionTask>& rawTasks){
    if (rawTasks.empty()) return false;

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
    json j;
    j["name"] = motionName;
    for (const auto& task : mergedTasks) {
        json t;
        t["joint"] = motionManager.JointName(task.joint); // 调用之前写的工具函数转 string
        t["method"] = "REL";
        t["val"] = task.targetAngle;
        t["speed"] = task.motionSpeed;
        j["tasks"].push_back(t);
    }

    std::ofstream file( motionManager._motion_folder + "/" + motionName + ".json");
    if (!file.is_open()) return false;
    file << j.dump(4);
    
    // 刷新
    motionManager.learn_motion_fresh();
    motionManager.servo_set_init(); 
    return true;

};


// 保险函数
void RobotCore::get_motion_name_from_text(const std::string& text, std::string& val) {
    
    std::string target = text;
    std::transform(target.begin(), target.end(), target.begin(), ::tolower);

    const std::string pattern = "please do ";
    size_t pos = target.find(pattern);

    if (pos != std::string::npos) {
        
        std::string raw_name = text.substr(pos + pattern.length());

        size_t first = raw_name.find_first_not_of(' ');
        if (std::string::npos == first) {
            val.clear();
        }
        size_t last = raw_name.find_last_not_of(' ');

        val = raw_name.substr(first, (last - first + 1));
    }
}

CameraHandle* RobotCore::getCamHandle() const {
    return cam;
}