#include "SpeakerEngine.hpp"
#include <cstring>
#include <cmath>
#include <thread>
#include <sched.h>
#include <pthread.h>

//espeak need a global pointer to the instance for callback access
UsbSpeaker* UsbSpeaker::_instance = nullptr;

// tool for multithread
static void pinThreadToCore(std::thread &th, int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    int rc = pthread_setaffinity_np(th.native_handle(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        std::cerr << "Error pinning thread to core " << core_id << std::endl;
    } else {
        std::cout << "Thread bound to Core " << core_id << std::endl;
    }
}

//set the callback function for espeak
//_instance is a pointer to the class
UsbSpeaker::UsbSpeaker(const std::string& deviceName, 
                       const ModelPaths& models,
                       int channels,
                       int language)
    : _deviceName(deviceName), _channels(channels) {
    _instance = this;

    // reset the config file
    memset(&_config, 0, sizeof(_config));

    std::string modelPath;

    if(language == 0){
        // use En
        modelPath = models.en + "/en_GB-cori-medium.onnx";
    }else{
        // use Zh
        modelPath = models.zh + "/zh_CN-huayan-medium.onnx";
    }
    // use for VITS
    //std::string lexiconPath = modelDir + "/lexicon.txt";

    // use for piper
    std::string dataDirPath = models.en + "/espeak-ng-data";
    _config.model.vits.lexicon = nullptr; 

    std::string tokensPath = models.en + "/tokens.txt";

    
    _config.model.vits.model = strdup(modelPath.c_str());
    // use for VITS
    //_config.model.vits.lexicon = strdup(lexiconPath.c_str());
    _config.model.vits.tokens = strdup(tokensPath.c_str());

    // use for piper
    _config.model.vits.data_dir = strdup(dataDirPath.c_str());


    // signal-core control
    _config.model.num_threads = 1;
    _config.model.vits.length_scale = 1.0f;


    // TTS create
    _tts = SherpaOnnxCreateOfflineTts(&_config);

    if (_tts) {
        _sampleRate = SherpaOnnxOfflineTtsSampleRate(_tts);
        std::cout << "Sherpa-Onnx Init Success! Sample Rate: " << _sampleRate << std::endl;
    } else {
        std::cerr << "Error: Failed to create Sherpa-Onnx TTS engine!" << std::endl;
        std::cerr << "Please check model path: " << modelPath << std::endl;
    }

    _running = false;

}

UsbSpeaker::~UsbSpeaker() {
    stop();
    close();
    //espeak_Terminate();
    if (_tts) {
        SherpaOnnxDestroyOfflineTts(_tts);
        _tts = nullptr;
    }
    if (_config.model.vits.model) free((void*)_config.model.vits.model);
    // use for VITS
    // if (_config.model.vits.lexicon) free((void*)_config.model.vits.lexicon);
    if (_config.model.vits.tokens) free((void*)_config.model.vits.tokens);
}

bool UsbSpeaker::open() {
    // create a connection to the ALSA device
    // handle: space for PCM data
    // _deviceName.c_str(): device name, e.g. "hw:2,0"
    int rc = snd_pcm_open(&_handle, _deviceName.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        std::cerr << "Speaker Error: " << snd_strerror(rc) << std::endl;
        return false;
    }

    // set hardware parameters
    snd_pcm_hw_params_t* params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(_handle, params);
    snd_pcm_hw_params_set_access(_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(_handle, params, _channels);
    snd_pcm_hw_params_set_rate_near(_handle, params, &_sampleRate, 0);

    // set low latency parameters
    _frames = 1024; 
    snd_pcm_hw_params_set_period_size_near(_handle, params, &_frames, 0);
    snd_pcm_uframes_t bufferSize = _frames * 3; 
    snd_pcm_hw_params_set_buffer_size_near(_handle, params, &bufferSize);

    rc = snd_pcm_hw_params(_handle, params);
    if (rc < 0) return false;

    _running = true;
    _playbackThread = std::thread(&UsbSpeaker::playbackLoop, this);
    _synthesisThread = std::thread(&UsbSpeaker::synthesisLoop, this);

    pinThreadToCore(_synthesisThread, 3);
    pinThreadToCore(_playbackThread, 3);

    return true;
}

//used by espeakCallback to play PCM data
void UsbSpeaker::playInternal(const std::vector<short>& data) {
    { 
    std::lock_guard<std::mutex> lock(_queueMutex);
    _dataQueue.push(data);
    }
    _audioCV.notify_one();
}

//use by main.cpp to play text
void UsbSpeaker::play(const std::string& text) {
    {
    std::lock_guard<std::mutex> lock(_textMutex);
     _textQueue.push(text);
    }
     _textCV.notify_one();
}

void UsbSpeaker::synthesisTask(std::string text) {
    if (!_tts) return;

    const SherpaOnnxGeneratedAudio* audio = SherpaOnnxOfflineTtsGenerate(_tts, text.c_str(), 0, 1.0f);

    if (audio && audio->n > 0) {
        std::vector<short> pcmData;
        pcmData.reserve(audio->n * _channels); 
        
        for (int i = 0; i < audio->n; ++i) {
            float s = audio->samples[i];

            if (s > 1.0f) s = 1.0f;
            if (s < -1.0f) s = -1.0f;

            short sample = static_cast<short>(s * 32767);

            for (int c = 0; c < _channels; ++c) {
                pcmData.push_back(sample);
            }
        }
        playInternal(pcmData);
        SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
    }
}

void UsbSpeaker::stop() {
    if (!_running) return;
    _running = false;
    
    _textCV.notify_all();
    _audioCV.notify_all();

    if (_playbackThread.joinable()) _playbackThread.join();
    if (_synthesisThread.joinable()) _synthesisThread.join();
}

void UsbSpeaker::close() {
    if (_handle) {
        snd_pcm_close(_handle);
        _handle = nullptr;
    }
}

// void UsbSpeaker::playbackLoop() {
//     while (_running) {
//         std::vector<short> buffer;
//         bool hasData = false;
//         {
//             std::lock_guard<std::mutex> lock(_queueMutex);
            
//             if (!_dataQueue.empty()) {
//                 buffer = std::move(_dataQueue.front());
//                 _dataQueue.pop();
//                 hasData = true;
//             }

//         }
//         if (hasData) {
//             if (!buffer.empty() && _handle) {
//                 int rc = snd_pcm_writei(_handle, buffer.data(), buffer.size() / _channels);
//                 if (rc == -EPIPE) {
//                     snd_pcm_prepare(_handle);
//                 } else if (rc < 0) {
//                     std::cerr << "ALSA Write Error: " << snd_strerror(rc) << std::endl;
//                 }
//             }
//         } else {
//             std::this_thread::yield();
//         }
//     }
// }

void UsbSpeaker::playbackLoop() {
    while (_running) {
        std::vector<short> buffer;
    {
        std::unique_lock<std::mutex> lock(_queueMutex);
        _audioCV.wait(lock, [this]{ 
            return !_dataQueue.empty() || !_running; 
        });

        if (!_running) break;

        buffer = std::move(_dataQueue.front());
        _dataQueue.pop();
        
        //lock.unlock(); 
    }

        if (!buffer.empty() && _handle) {
            snd_pcm_uframes_t totalFrames = buffer.size() / _channels;
            snd_pcm_uframes_t framesWritten = 0;
            short* pData = buffer.data();

            while (framesWritten < totalFrames && _running) {
                int rc = snd_pcm_writei(_handle, pData + (framesWritten * _channels), totalFrames - framesWritten);

                if (rc == -EPIPE) {
                    snd_pcm_prepare(_handle);
                } else if (rc < 0) {
                    break; 
                } else {
                    framesWritten += rc;
                }
            }
        }
    }
}

void UsbSpeaker::synthesisLoop() {
    while (_running) {
        std::string textToSpeak;
        {
            std::unique_lock<std::mutex> lock(_textMutex);
            _textCV.wait(lock, [this]{ 
                return !_textQueue.empty() || !_running; 
            });
            if (!_running) break;
            textToSpeak = _textQueue.front();
            _textQueue.pop();
        } 
        if (!textToSpeak.empty()) {
            synthesisTask(textToSpeak);
        }
    }
}