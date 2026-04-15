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
    
    // PLAY PCM data directly (used by eSpeak callback)
    void playInternal(const std::vector<short>& data);
    
    // PLAY text by synthesizing it with eSpeak
    void play(const std::string& text);
    
    // void stop();

    void start_thread(int core);

    void stop_thread();

private:

    void playbackLoop();

    void synthesisLoop();
    
    void synthesisTask(std::string text);

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

    std::condition_variable _textCV;
    std::condition_variable _audioCV;

    std::shared_ptr<TaskMonitor> _taskMonitor;

    // 任务Task
    std::atomic<int> task_id = 2000;

    TaskDescribe _taskdescribe;

    EmptyResult bg;

    // 多一个队列来维护id和文本用来处理跨函数记录
    ThreadSafeQueue<Taskdata> _testdata;             
};

#endif