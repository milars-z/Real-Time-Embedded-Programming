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

class UsbSpeaker {
public:
    
    UsbSpeaker(const std::string& deviceName = "default", 
               unsigned int sampleRate = 44100, 
               int channels = 2);
    ~UsbSpeaker();

    bool open();
    void close();
    
    // PLAY PCM data directly (used by eSpeak callback)
    void play(const std::vector<short>& data);
    
    // PLAY text by synthesizing it with eSpeak
    void play(const std::string& text);
    
    void stop();

private:

    void playbackLoop();
    

    static int espeakCallback(short* wav, int numsamples, espeak_EVENT* events);

    // ALSA 
    snd_pcm_t* _handle = nullptr;
    std::string _deviceName;
    unsigned int _sampleRate;
    int _channels;
    snd_pcm_uframes_t _frames; 

    // THREADING
    std::thread _playbackThread;
    std::atomic<bool> _running{false};
    std::queue<std::vector<short>> _dataQueue;
    std::mutex _queueMutex;
    std::condition_variable _cv;

    // STATIC INSTANCE for callback access
    static UsbSpeaker* _instance;
};

#endif