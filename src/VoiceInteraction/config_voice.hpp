#pragma once
#include <string>
#include <vector>

#include "SpeakerEngine.hpp"

namespace Config {

    namespace Hardware {
        const std::string SPEAKER_NAME = "UACDemo";
        const std::string MIC_NAME     = "USB PnP";
        const int MIC_SAMPLE_RATE      = 16000;
    }

    namespace Path {
        // PRO
        inline const std::string SRC_ROOT = PROJECT_ROOT_DIR;

        // NLU 
        const std::string NLU_MODEL_DIR = SRC_ROOT + "VoiceInteraction/model_nlu"; 

        // Vosk 
        const std::string VOSK_MODEL_DIR = SRC_ROOT + "Microphone/model/model"; 

        // Speaker  (Sherpa/VITS)
        const ModelPaths SPEAKER_MODELS = {
            SRC_ROOT + "Speaker/model/vits-piper-en_GB-cori-medium-int8",
            SRC_ROOT + "Speaker/model/vits-piper-zh_CN-huayan-medium"
        };
    }

    namespace Camera {
        const std::string CAMERA_MODEL = "../../Camera/model/mobilenet_v2_slice.onnx";
        const std::string CAMERA_FEATURE = "../../Camera/feature/my_features.yml";
    }

    namespace Motion {
        const std::string MOTION_CONFIG = "../../Motor/servo_config.txt";
    }

}