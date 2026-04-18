#ifndef SPEAKER_ENGINE_HPP
#define SPEAKER_ENGINE_HPP

#include <alsa/asoundlib.h>
#include <espeak-ng/speak_lib.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <fstream>

#include "c-api.h"
#include "Tools.hpp"
#include "Config.hpp"
#include "TaskMonitor.hpp"

struct Taskdata {
    int id;
    std::string speaktext;
};

class UsbSpeaker {
public:
    
    // 'language = 0 for En'
    // 'language = 1 for Zh'
    UsbSpeaker(const std::string& deviceName, 
               int channels,
               int language,
               std::shared_ptr<TaskMonitor> taskMonitor);
    ~UsbSpeaker();

    bool open();
    void close();
    
    void play(const std::string& text);
    
    void start_thread(int core);

    void stop_thread();

private:

    void playbackLoop();

    void synthesisLoop();

    void playInternal(const std::vector<short>& data);
    
    void synthesisTask(const std::string& text);

    bool is_innerText(const std::string& text);

    void warmupCache(const std::unordered_set<std::string>& texts);

    std::vector<short> generate_pcm(const std::string& text);

    bool load_innerText(const std::string& filepath);

    const SherpaOnnxOfflineTts* _tts = nullptr;
    SherpaOnnxOfflineTtsConfig _config;
    
    snd_pcm_t* _handle = nullptr;
    std::string _deviceName;
    unsigned int _sampleRate;
    int _channels;
    snd_pcm_uframes_t _frames; 

    // THREADING
    std::thread _playbackThread;
    std::thread _synthesisThread; 
    std::atomic<bool> _running{false};

    std::queue<std::vector<short>> _dataQueue;
    std::queue<std::string> _textQueue;

    std::mutex _queueMutex;
    std::mutex _textMutex;
    std::mutex _cacheMutex;

    std::condition_variable _textCV;
    std::condition_variable _audioCV;

    std::shared_ptr<TaskMonitor> _taskMonitor;

    // 任务Task
    std::atomic<int> task_id = 2000;

    TaskDescribe _taskdescribe;

    EmptyResult bg;

    // 多一个队列来维护id和文本用来处理跨函数记录
    ThreadSafeQueue<Taskdata> _testdata;  
    
    // 缓存
    std::unordered_map<std::string, std::vector<short>> _ttsCache;

    // 后续inner_text不会全部缓存，只缓存指定的text，因此设置为unordered_set方便直接用find
    std::unordered_set<std::string> _inner_text;
};

#endif