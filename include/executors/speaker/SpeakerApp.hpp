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

/**
 * @brief Speaker execution module.
 *
 * This class handles text-to-speech execution by interacting with the underlying
 * UsbSpeaker engine. It manages multi-language text mapping, variable substitution
 * (e.g., host name, robot name), and executes speech tasks received from the system.
 */
class SpeakerExecutor : public BaseExecutor<std::string> {
private:
    std::unique_ptr<UsbSpeaker> speaker;
    std::shared_ptr<TaskMonitor> _taskMonitor;

    std::string _speaker_path;

    // Command -> text
    std::unordered_map<std::string, multi_lang> text_lib;

    // Default language: English
    std::string currentLang = "en";

    // Variable configuration (e.g., host_name, robot_name)
    std::unordered_map<std::string, std::string> variables;

    // Config_name
    std::string _host_name = "milars";
    std::string _robot_name = "robot";

public:
    
    // Constructor (currently supports English only, multi-language support can be added later)
    SpeakerExecutor(std::atomic<int>& system_state, const std::string& path,const std::string& text_path, std::shared_ptr<TaskMonitor> taskMonitor);
    ~SpeakerExecutor();

    void pinThread(int core);

    void _stop() override;

    void _start(int core) override;

    // Update variables
    void setVariable(const std::string& key, const std::string& value);

    // Get text from library by key (external call)
    std::string getText(const std::string& key);

private:

    // Task execution implementation
    void onExecute(const std::string& text) override;

    // Internal executor function to retrieve the current module name
    std::string get_module_name() override;

    // Refresh variables
    bool loadVariable();

    // Load text library
    bool loadLibrary();

    // Set language (currently not fully implemented)
    void setLanguage(const std::string& lang);
};
