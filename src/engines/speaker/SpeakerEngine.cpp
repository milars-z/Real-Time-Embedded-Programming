#include "SpeakerEngine.hpp"
#include "Tools.hpp"
#include <cstring>
#include <cmath>
#include <thread>
#include <sched.h>
#include <pthread.h>

//set the callback function for espeak
//_instance is a pointer to the class
UsbSpeaker::UsbSpeaker(const std::string& deviceName, 
                       int channels,
                       int language,
                       std::shared_ptr<TaskMonitor> taskMonitor)
: _deviceName(deviceName), _channels(channels), _taskMonitor(std::move(taskMonitor))
{

    // reset the config file
    memset(&_config, 0, sizeof(_config));

    std::string modelPath;
    std::string dataDirPath;
    std::string tokensPath;

    if(language == 0){
        // use En
        modelPath = Config::Speaker::SPEAKER_MODELS_EN + "/en_GB-cori-medium.onnx";
        dataDirPath = Config::Speaker::SPEAKER_MODELS_EN + "/espeak-ng-data";
        tokensPath = Config::Speaker::SPEAKER_MODELS_EN + "/tokens.txt";
    }else{
        // use Zh
        modelPath = Config::Speaker::SPEAKER_MODELS_ZH + "/zh_CN-huayan-medium.onnx";
        dataDirPath = Config::Speaker::SPEAKER_MODELS_ZH + "/espeak-ng-data";
        tokensPath = Config::Speaker::SPEAKER_MODELS_ZH + "/tokens.txt";
    }
    // use for VITS
    //std::string lexiconPath = modelDir + "/lexicon.txt";

    // use for piper
    // std::string dataDirPath = models.en + "/espeak-ng-data";
    _config.model.vits.lexicon = nullptr; 

    // std::string tokensPath = models.en + "/tokens.txt";

    
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
        std::cout << "[Init][SpeakerEngine]Sherpa-Onnx Init Success! Sample Rate: " << _sampleRate << std::endl;
    } else {
        std::cerr << "[Error][SpeakerEngine] Failed to create Sherpa-Onnx TTS engine!" << std::endl;
        std::cerr << "[Error][SpeakerEngine]Please check model path: " << modelPath << std::endl;
    }

    _running = false;

    _taskdescribe.Name = "None";
    _taskdescribe.TaskType = "TTS";

    load_innerText(Config::Speaker::INNER_TEXT);
#ifdef PRECACHE
    std::cout << "[Info][speakerEngine]Loading internal phrases..." << std::endl;
    warmupCache(_inner_text);
    std::cout << "[Info][speakerEngine]Loading Successfully!!" << std::endl;
#endif

}

UsbSpeaker::~UsbSpeaker() {
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

    if (_config.model.vits.data_dir) free((void*)_config.model.vits.data_dir);
}

void UsbSpeaker::start_thread(int core){

    _playbackThread = std::thread(&UsbSpeaker::playbackLoop, this);
    _synthesisThread = std::thread(&UsbSpeaker::synthesisLoop, this);

    pinThreadToCore(_synthesisThread, "TTS",core);
    pinThreadToCore(_playbackThread, "ALSA",core);

}

void UsbSpeaker::stop_thread() {
    if (!_running) return ;
    _running = false;
    
    _textCV.notify_all();
    _audioCV.notify_all();

    if (_playbackThread.joinable()) _playbackThread.join();
    if (_synthesisThread.joinable()) _synthesisThread.join();

}

bool UsbSpeaker::open() {
    // create a connection to the ALSA device
    // handle: space for PCM data
    // _deviceName.c_str(): device name, e.g. "hw:2,0"
    int rc = snd_pcm_open(&_handle, _deviceName.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        std::cerr << "[Error][SpeakerEngine]Speaker Error: " << snd_strerror(rc) << std::endl;
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

    // 从函数调用开始计算时间
#ifdef TESTMODE
    TaskEvent _taskevent;
    Taskdata testdata;
    _taskevent.moduleName = "Speaker";
    _taskevent.taskId = task_id++;
    _taskevent.status = TaskStatus::STARTED;
    _taskevent.taskType = _taskdescribe;
    _taskevent.timestamp = std::chrono::steady_clock::now();
    _taskMonitor->postEvent(_taskevent);
    testdata.id = _taskevent.taskId;
    testdata.speaktext = text;
    _testdata.push(testdata);
#endif
    {
    std::lock_guard<std::mutex> lock(_textMutex);
     _textQueue.push(text);
    }
     _textCV.notify_one();
}

void UsbSpeaker::synthesisTask(const std::string& text) {
    if (!_tts) return;

    if (is_innerText(text)) {
        std::lock_guard<std::mutex> lock(_cacheMutex);
        auto it = _ttsCache.find(text);
        if (it != _ttsCache.end()) {
            playInternal(it->second);
            return;
        }
    }

    std::vector<short> pcmData = generate_pcm(text);
    if (pcmData.empty()) return;
#ifndef PRECACHE    
    if (is_innerText(text))
#endif
    {
        std::lock_guard<std::mutex> lock(_cacheMutex);
        _ttsCache[text] = pcmData;
    }

    playInternal(std::move(pcmData));

    
}


void UsbSpeaker::close() {
    if (_handle) {
        snd_pcm_close(_handle);
        _handle = nullptr;
    }
}

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
        
    }

        if (!buffer.empty() && _handle) {
            snd_pcm_uframes_t totalFrames = buffer.size() / _channels;
            snd_pcm_uframes_t framesWritten = 0;
            short* pData = buffer.data();

#ifdef TESTMODE
            Taskdata testdata;
            TaskEvent _taskevent;
            _testdata.pop(testdata);
            _taskdescribe.Name = testdata.speaktext;
            _taskevent.moduleName = "Speaker";
            _taskevent.taskId = testdata.id;
            _taskevent.status = TaskStatus::FINISHED;
            _taskevent.taskType = _taskdescribe;
            _taskevent.issuccessful = true;
            _taskevent.timestamp = std::chrono::steady_clock::now();
            _taskMonitor->postEvent(_taskevent);
#endif

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

bool UsbSpeaker::is_innerText(const std::string& text){

    return _inner_text.find(text) != _inner_text.end();

}

std::vector<short> UsbSpeaker::generate_pcm(const std::string& text){
    // generate pcmdata
    const SherpaOnnxGeneratedAudio* audio = SherpaOnnxOfflineTtsGenerate(_tts, text.c_str(), 0, 1.0f);

    std::vector<short> pcmData;
    if (audio && audio->n > 0) {
        
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
    }

    if(audio){
        SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
    }

    return pcmData;
}

bool UsbSpeaker::load_innerText(const std::string& filepath) {
    std::ifstream file(filepath);

    if (!file.is_open()) {
        std::cerr << "[Error][SpeakerEngine] Failed to open inner_text file: " << filepath << std::endl;
        return false;
    }

    std::string line;
    int count = 0;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        _inner_text.insert(line);
        count++;
    }

    std::cout << "[Info][SpeakerEngine] Loaded " << count << " cacheable texts." << std::endl;
    return true;
}

void UsbSpeaker::warmupCache(const std::unordered_set<std::string>& texts) {
    if (!_tts) return;

    for (const auto& rawText : texts) {
        if (rawText.empty()) continue;

        if (!is_innerText(rawText)) {
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(_cacheMutex);
            if (_ttsCache.find(rawText) != _ttsCache.end()) {
                continue;
            }
        }

        std::vector<short> pcmData = generate_pcm(rawText);
        if (pcmData.empty()) continue;

        {
            std::lock_guard<std::mutex> lock(_cacheMutex);
            _ttsCache[rawText] = std::move(pcmData);
        }
    }
}
