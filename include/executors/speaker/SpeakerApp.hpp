#pragma once

#include "Executor.hpp"
#include "TaskMonitor.hpp"
#include <string>
#include <memory>
#include <unordered_map>

#include "Tools.hpp"

class UsbSpeaker;

struct multi_lang {
    std::string en;
    std::string zh;
};

class SpeakerExecutor : public BaseExecutor<std::string> {
private:
    std::unique_ptr<UsbSpeaker> speaker;
    std::shared_ptr<TaskMonitor> _taskMonitor;

    std::string _speaker_path;

    // command -> text
    std::unordered_map<std::string, multi_lang> text_lib;

    // 默认英文
    std::string currentLang = "en";

    // 变量配置，如host_name;robot_name
    std::unordered_map<std::string, std::string> variables;

    // 加载lib
    bool loadLibrary();

    
    

    // 配置语言-暂时不配置
    void setLanguage(const std::string& lang);

    // config_name
    std::string _host_name = "milars";
    std::string _robot_name = "robot";

public:
    
    // 基础构造函数，暂时只有英文，后续可以添加语言参数
    SpeakerExecutor(std::atomic<int>& system_state, const std::string& path,const std::string& text_path, std::shared_ptr<TaskMonitor> taskMonitor);
    ~SpeakerExecutor();

    // 任务实现
    void onExecute(const std::string& text) override;

    void pinThread(int core);

    void _stop() override;

    void _start(int core) override;

    std::string get_module_name() override;

    // 获取Lib对应的文本，外部调用
    std::string getText(const std::string& key);

    // 刷新变量
    bool loadVariable();

    // 更新变量
    void setVariable(const std::string& key, const std::string& value);
};