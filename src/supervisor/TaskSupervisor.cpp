#include <iostream>
#include <fstream>
#include <unordered_map>
#include <chrono>
#include <iomanip>
#include <thread>
#include <atomic>

#include "TaskSupervisor.hpp"
#include "SpeakerApp.hpp"
#include "MotionApp.hpp"

class SpeakerExecutor;
class MotionExecutor;

namespace fs = std::filesystem;

TaskSupervisor::TaskSupervisor( std::shared_ptr<TaskMonitor> monitor,
                                std::shared_ptr<MotionExecutor> motion,
                                std::shared_ptr<SpeakerExecutor> speaker)
:_speaker(speaker),_motion(motion),_monitor(monitor)
{
    Initfile();
}

TaskSupervisor::~TaskSupervisor() = default;

void TaskSupervisor::start_thread(int core){
    _running = true;
    _TaskWorker = std::thread(&TaskSupervisor::processLoop, this);
    pinThreadToCore(_TaskWorker,"Supervisor",core);

}

void TaskSupervisor::stop_thread(){
        _running = false;
        _monitor->stop(); 
        if (_TaskWorker.joinable()) _TaskWorker.join();
}

// writen with AI
void TaskSupervisor::Initfile(){
    std::string Fp = Config::Test::TEST_FILE;
    fs::path p(Fp);
    try {
        // 检查并创建文件夹
        if (p.has_parent_path() && !fs::exists(p.parent_path())) {
            fs::create_directories(p.parent_path());
        }

        // 打开文件（追加模式）
        _logFile.open(Fp, std::ios::app);

        if (!_logFile.is_open()) {
            throw std::runtime_error("Failed to open log file: " + Fp);
        }

        _logFile.seekp(0, std::ios::end);
        if (_logFile.tellp() == 0) {
            _logFile << "TaskID,Module,Type,TargetName,Duration(ms),issuccessful\n";
        }

        std::cout << "[Supervisor] Logging initialized at: " << Fp << std::endl;
    } 
    catch (const std::exception& e) {
        std::cerr << "[Critical][Supervisor] Logger initialization failed: " << e.what() << std::endl;
    }
}


// thread
void TaskSupervisor::processLoop(){
    TaskEvent event;

    while (_monitor->waitEvent(event)) {
        if (!_running) break;

        if (event.status == TaskStatus::STARTED) {
            _pendingTasks[event.taskId] = event.timestamp;
        }else if (event.status == TaskStatus::FINISHED) {
            auto it = _pendingTasks.find(event.taskId);
            const auto& describe = std::get<TaskDescribe>(event.taskType);
            if (it != _pendingTasks.end()) {
                // 计算耗时
                auto duration = std::chrono::duration<double, std::milli>(event.timestamp - it->second).count();
                
                // 写入基本信息
                _logFile << event.taskId << "," 
                         << event.moduleName << ","
                         << describe.TaskType << ","
                         << describe.Name << ","
                         << std::fixed << std::setprecision(3) << duration << ","
                         << event.issuccessful << ",";

                handleTaskResult(event);
                
                _logFile << std::endl;
                _logFile.flush(); 

                _pendingTasks.erase(it);
            }else{
                // 来到这里说明出现了异常，即有结束但是没有开始
                std::cerr << "[TaskSupervisor]file system error,ID : " << event.taskId << "Module : " << event.moduleName << std::endl;
            }

        }
    }
}

void TaskSupervisor::handleTaskResult(const TaskEvent& e) {
    // 根据反馈的细节进行调节并部署新的task
    //判断是否是cam
    if (e.moduleName == "Camera"){
        // 如果是camera的结果，分为三类
        // 更新背景
        // 获取任务描述
        const auto& describe = std::get<TaskDescribe>(e.taskType);
        if(describe.TaskType == "Update"){
            // 暂时没有做失败的逻辑，应该不会失败
            _speaker->pushTask("background update successfully");
        }

        // 学习模式
        else if(describe.TaskType == "Learn"){
            
            // 学习模式，可能会出现没更新背景，需要反馈
            if(describe.Name == "NoBackground"){
                _speaker->pushTask("please update background first");
                return;
            }
            // 学习模式可能学到可能没学到，分别反馈
            const auto& learn_ans = std::get<CameraResult>(e.result);
            if(learn_ans.isdetecte == false){
                _speaker->pushTask("can't find object");
            }else if(learn_ans.isdetecte == true){
                std::string ans = "learn object" + learn_ans.objectName + "successfully";
                _speaker->pushTask(ans);
            }
            

        }
        
        // 检测模式
        else if(describe.TaskType == "Detecte"){
            // 学习模式，可能会出现没更新背景，需要反馈
            if(describe.Name == "NoBackground"){
                _speaker->pushTask("please update background first");
                return;
            }
            const auto& learn_ans = std::get<CameraResult>(e.result);
            std::string name = learn_ans.objectName;
            if(learn_ans.isdetecte == false){
                std::string ans = "can not find" + name;
                _speaker->pushTask(ans);
            }else if(learn_ans.isdetecte == true){
                int position_x = learn_ans.position_x;
                int position_y = learn_ans.position_y;
                std::cout << "find obj at x : " << position_x << "y : " << position_y << std::endl;
            }
        }




    }else if (e.moduleName == "Motion"){





    }else{
        // noway do nothing
    }
}



// class TaskSupervisor {
// public:

//     TaskSupervisor( std::shared_ptr<TaskMonitor> monitor,
//                     std::shared_ptr<MotionExecutor> motion,
//                     std::shared_ptr<SpeakerExecutor> speaker); 
//     ~TaskSupervisor(); 

// private:
//     void processLoop();

//     void handleTaskResult(const TaskEvent& e, std::ofstream& log);

//     void MotionT(int x, int y);

//     void SpeakerT(std::string Command);


// };