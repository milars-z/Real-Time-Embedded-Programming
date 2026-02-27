#ifndef MICROPHONE_ENGINE_HPP
#define MICROPHONE_ENGINE_HPP

#include <alsa/asoundlib.h>
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <iostream>


// short vector to store the message about audio (16-bit PCM)
using AudioCallback = std::function<void(const std::vector<short>&)>;

class UsbMicrophone {
public:
    // normal setting
    UsbMicrophone(const std::string& deviceName = "default",
                  unsigned int sampleRate = 16000,
                  int channels = 1);

    virtual ~UsbMicrophone();
    bool open();
    void close();
    bool start(AudioCallback callback);
    void stop();

private:
    void captureLoop();

    std::string _deviceName;
    unsigned int _sampleRate;
    int _channels;

    snd_pcm_t* _handle = nullptr;
    std::thread captureThread;
    std::atomic<bool> _running{false};
    AudioCallback _callback;

    // frames in every period
    // if the modle change to more larger this param should set larger too
    // if sample rate  = 16K and  _frames = 32
    // 32/16000 = 2*10^-3 --2ms
    // AI model need to trans audio to text in 2ms
    snd_pcm_uframes_t _frames = 32;

};
#endif