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
#include "VisonTools.hpp"


struct ModelPaths {
    std::string en; 
    std::string zh; 
};

class UsbSpeaker {
public:
    
    // 'language = 0 for En'
    // 'language = 1 for Zh'
    UsbSpeaker(const std::string& deviceName, 
               const ModelPaths& models, 
               int channels = 1,
               int language = 0);
    ~UsbSpeaker();

    bool open();
    void close();
    
    // PLAY PCM data directly (used by eSpeak callback)
    void playInternal(const std::vector<short>& data);
    
    // PLAY text by synthesizing it with eSpeak
    void play(const std::string& text);
    
    void stop();

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
    // STATIC INSTANCE for callback access
    static UsbSpeaker* _instance;

    std::condition_variable _textCV;
    std::condition_variable _audioCV;
};

#endif