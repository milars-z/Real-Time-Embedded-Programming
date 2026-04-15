#pragma once
#include <string>
#include <vector>


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
        const std::string NLU_MODEL_DIR = SRC_ROOT + "/third_party/model_nlu_2"; 

        // Vosk 
        const std::string VOSK_MODEL_DIR = SRC_ROOT + "/third_party/microphone/model/model"; 

        const std::string SPEAKER_MODELS_EN = SRC_ROOT + "/third_party/speaker/model/vits-piper-en_GB-cori-medium-int8";

        const std::string SPEAKER_MODELS_ZH = SRC_ROOT + "/third_party/speaker/model/vits-piper-zh_CN-huayan-medium";

    }

    namespace Camera {
        inline const std::string SRC_ROOT = PROJECT_ROOT_DIR;

        const std::string CAMERA_MODEL = SRC_ROOT + "/third_party/camera/model/mobilenet_v2_slice.onnx";
        const std::string CAMERA_FEATURE = SRC_ROOT + "/learning_data/camera/feature/my_features.yml";
    }

    namespace Motion {
        inline const std::string SRC_ROOT = PROJECT_ROOT_DIR;

        const std::string MOTION_CONFIG = SRC_ROOT + "/include/common/servo_config.txt";
        const std::string MOTION_SET = SRC_ROOT + "/learning_data/motion/motion_set";
        const std::string INNER_MOTION_SET = SRC_ROOT + "/learning_data/motion/inner_motion_set";
    }

    namespace Test {
        inline const std::string SRC_ROOT = PROJECT_ROOT_DIR;

        const std::string TEST_FILE = SRC_ROOT + "/test/log.csv";

    }

}