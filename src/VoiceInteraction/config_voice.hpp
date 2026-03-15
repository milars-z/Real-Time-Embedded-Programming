#pragma once
#include <string>
#include <vector>
#include "SpeakerEngine.hpp" 
#include "voiceInteraction.hpp"
#include "VisonTools.hpp"

namespace Config {

    const std::string PROJECT_ROOT = "/home/milars/Real-Time-Embedded-Programming/VoiceInteraction";

    namespace Hardware {
        const std::string SPEAKER_NAME = "UACDemo";
        const std::string MIC_NAME     = "USB PnP";
        const int MIC_SAMPLE_RATE      = 16000;
    }

    namespace Path {
        // NLU 
        const std::string NLU_MODEL_DIR = PROJECT_ROOT + "/model_nlu"; 

        // Vosk 
        const std::string VOSK_MODEL_DIR = PROJECT_ROOT + "/../MicrophoneTest/model/model"; 

        // Speaker  (Sherpa/VITS)
        const ModelPaths SPEAKER_MODELS = {
            PROJECT_ROOT + "/../SpeakerTest/model/vits-piper-en_GB-cori-medium-int8",
            PROJECT_ROOT + "/../SpeakerTest/model/vits-piper-zh_CN-huayan-medium"
        };
    }
}