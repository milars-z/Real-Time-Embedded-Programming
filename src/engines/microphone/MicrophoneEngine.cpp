#include "MicrophoneEngine.hpp"
#include "Tools.hpp"


UsbMicrophone::UsbMicrophone(const std::string& deviceName,
                             unsigned int sampleRate, 
                             int channels)
    : _deviceName(deviceName), _sampleRate(sampleRate), _channels(channels) {}

// close microphone device
UsbMicrophone::~UsbMicrophone() {
    stop();
    close();
}

// open microphone device
// snd_pcm_open
//                                Input
// &_handle: output vector
// _deviceName.c_str(): input device
// SND_PCM_STREAM_CAPTURE: CAPTURE or PLAYBACK
// 0 : input method
//                                Output
// if out put == 0 : success 
bool UsbMicrophone::open() {
    int rc = snd_pcm_open(&_handle, _deviceName.c_str(), SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        std::cerr << "[Error][MicrophoneEngine]cant open audio devices: " << snd_strerror(rc) << std::endl;
        return false;
    }

    snd_pcm_hw_params_t* params;
    snd_pcm_hw_params_alloca(&params);
    // init params with device's information
    snd_pcm_hw_params_any(_handle, params);
    // Access Mode
    snd_pcm_hw_params_set_access(_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    // Format
    snd_pcm_hw_params_set_format(_handle, params, SND_PCM_FORMAT_S16_LE); 
    // Channels
    snd_pcm_hw_params_set_channels(_handle, params, _channels);
    // Sample Rate :
    snd_pcm_hw_params_set_rate_near(_handle, params, &_sampleRate, 0);
    // Period Size :
    snd_pcm_hw_params_set_period_size_near(_handle, params, &_frames, 0);

    // params check
    rc = snd_pcm_hw_params(_handle, params);
    if (rc < 0) {
        std::cerr << "[Error][MicrophoneEngine]can't set suitable params for device: " << snd_strerror(rc) << std::endl;
        return false;
    }
    return true;
}

void UsbMicrophone::close() {
    if (_handle) {
        snd_pcm_close(_handle);
        _handle = nullptr;
    }
}

// thread start
// callback is a vector which store audio data
bool UsbMicrophone::start(AudioCallback callback) {
    if (_running) return true;
    _callback = callback;
    
    return true;
}

void UsbMicrophone::start_thread(int core){
    _running = true;
    captureThread = std::thread(&UsbMicrophone::captureLoop, this);
    pinThreadToCore(captureThread, "Mic" ,core);
}


void UsbMicrophone::stop() {
    _running = false;
    if (captureThread.joinable()) {
        captureThread.join();
        std::cout << "[End][UsbMicphone]:captureThread closed" << std::endl; 
    }
}


void UsbMicrophone::captureLoop() {
    int size = _frames * _channels;
    std::vector<short> buffer(size);

    // get all audio information to buffer and retuen buffer
    // buffer is a std::vector<short>
    while (_running) {
        // get data from _handle to buffer
        int rc = snd_pcm_readi(_handle, buffer.data(), _frames);
        
        if (rc == -EPIPE) {
            // Overrun 
            snd_pcm_prepare(_handle);
        } 
        if (rc == -ENODEV) {
            std::cerr << "[Error][MicphoneEngine] device lost: " << snd_strerror(rc) << std::endl;
            _running = false;
            break;
        }

        if (rc < 0) {
            std::cerr << "[Error][MicphoneEngine] can't read: " << snd_strerror(rc) << std::endl;

            if (snd_pcm_prepare(_handle) < 0) {
                std::cerr << "[Error][MicphoneEngine] recovery failed, exiting capture loop." << std::endl;
                _running = false;
                break;
            }
            continue;
        }
        
         else if (rc > 0) {
            // read successfully
            if (_callback) {
                _callback(buffer);
            }
        }
    }
}