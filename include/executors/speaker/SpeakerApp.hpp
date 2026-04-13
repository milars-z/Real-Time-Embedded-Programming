#pragma once

#include "Executor.hpp"
#include <string>
#include <memory>

class UsbSpeaker;

class SpeakerExecutor : public BaseExecutor<std::string> {
private:
    std::unique_ptr<UsbSpeaker> speaker;

public:
    
    // 基础构造函数，暂时只有英文，后续可以添加语言参数
    SpeakerExecutor(const std::string& path);
    ~SpeakerExecutor();

    // 任务实现
    void onExecute(const std::string& text) override;

    void pinThread(int core);

    void _stop() override;

    void _start(int core) override;

    std::string get_module_name() override;
};