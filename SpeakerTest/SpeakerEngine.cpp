#include "SpeakerEngine.hpp"

//espeak need a global pointer to the instance for callback access
UsbSpeaker* UsbSpeaker::_instance = nullptr;

//set the callback function for espeak
//_instance is a pointer to the class
UsbSpeaker::UsbSpeaker(const std::string& deviceName, unsigned int sampleRate, int channels)
    : _deviceName(deviceName), _sampleRate(sampleRate), _channels(channels) {
    _instance = this;

    // init eSpeak
    //AUDIO_OUTPUT_RETRIEVAL:directly retrieve audio data via callback, no internal playback
    espeak_Initialize(AUDIO_OUTPUT_RETRIEVAL, 0, nullptr, 0);
    
    //set the callback function for eSpeak
    espeak_SetSynthCallback(espeakCallback); 

    //set speak rate and volume
    espeak_SetParameter(espeakRATE, 80, 0); 
    espeak_SetParameter(espeakVOLUME, 200, 0);
    
    //set English voice
    espeak_SetVoiceByName("en"); 
}

UsbSpeaker::~UsbSpeaker() {
    stop();
    close();
    espeak_Terminate();
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
    return true;
}

//used by espeakCallback to play PCM data
void UsbSpeaker::play(const std::vector<short>& data) {
    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        _dataQueue.push(data);
    }
    // notify the playback thread that new data is available
    _cv.notify_one(); 
}

//use by main.cpp to play text
void UsbSpeaker::play(const std::string& text) {
    // after calling this function , espeak will start a thread to process the text
    // after processing,it will call the callback function as we set before
    // espeak_SetSynthCallback(espeakCallback); 
    espeak_Synth(text.c_str(), text.size() + 1, 0, POS_CHARACTER, 0, espeakCHARS_AUTO, nullptr, nullptr);
}

// eSpeak callback implementation
// input from eSpeak, output to ALSA
// wav: pointer to PCM data, numsamples: number of samples, events: eSpeak events (not used here)
int UsbSpeaker::espeakCallback(short* wav, int numsamples, espeak_EVENT* events) {
    //task check
    if (wav == nullptr || numsamples <= 0) 
        return 0;

    // change mono to stereo 
    // buffer size = numsamples * channels
    std::vector<short> buffer;
    buffer.reserve(numsamples * _instance->_channels);
    for (int i = 0; i < numsamples; ++i) {
        for (int c = 0; c < _instance->_channels; ++c) {
            buffer.push_back(wav[i]);
        }
    }
    _instance->play(buffer);
    return 0;
}

void UsbSpeaker::stop() {
    if (!_running) return;
    _running = false;
    // notify the playback thread to exit
    _cv.notify_all();
    if (_playbackThread.joinable()) _playbackThread.join();
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
            // thread will wait here until there is data to play or stop signal
            _cv.wait(lock, [this] { return !_dataQueue.empty() || !_running; });

            if (!_running && _dataQueue.empty()) break;
            if (!_dataQueue.empty()) {
                buffer = std::move(_dataQueue.front());
                _dataQueue.pop();
            }
            // end of espeak callback,release the lock for ALSA playback
        }

        // play the buffer using ALSA
        // copy the buffer to _handle and write to ALSA
        if (!buffer.empty() && _handle) {
            int rc = snd_pcm_writei(_handle, buffer.data(), buffer.size() / _channels);
            // handle underrun and other errors
            if (rc == -EPIPE) {
                snd_pcm_prepare(_handle);
            }
        }
    }
}