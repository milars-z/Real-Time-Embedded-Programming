#pragma once
#include <string>
#include <vector>

namespace Config {

    inline const std::string SRC_ROOT = PROJECT_ROOT_DIR;

    namespace Hardware {
        // key name to find hardware
        inline const std::string SPEAKER_NAME = "UACDemo";
        inline const std::string MIC_NAME     = "USB PnP";
        inline const int MIC_SAMPLE_RATE      = 16000;
    }

    namespace Nlu{

        // Nlu model
        inline const std::string NLU_MODEL_DIR = SRC_ROOT + "/third_party/model_nlu_2"; 

    }

    namespace Microphone{

        // STT model
        inline const std::string VOSK_MODEL_DIR = SRC_ROOT + "/third_party/microphone/model/model"; 

    }

    namespace Speaker {

        // TTS model
        // model for en
        inline const std::string SPEAKER_MODELS_EN = SRC_ROOT + "/third_party/speaker/model/vits-piper-en_GB-cori-medium-int8";
        // model for zh
        inline const std::string SPEAKER_MODELS_ZH = SRC_ROOT + "/third_party/speaker/model/vits-piper-zh_CN-huayan-medium";

        // speaker map "hello"->"hello,{host_name}"
        inline const std::string SPEAKER_TEXT = SRC_ROOT + "/include/common/speaker_text.json";
        // setting for host_name,robot_name and lang 
        inline const std::string VARIABLE_NAME = SRC_ROOT + "/include/common/varible_name.txt";
        // precache text
        inline const std::string INNER_TEXT = SRC_ROOT + "/include/common/inner_text.txt";

    }

    namespace Camera {

        // inference Model
        inline const std::string CAMERA_MODEL = SRC_ROOT + "/third_party/camera/model/mobilenet_v2_slice.onnx";
        // learning features
        inline const std::string CAMERA_FEATURE = SRC_ROOT + "/learning_data/camera/feature/my_features.yml";
        // config for 2D-3D
        inline const std::string CAMERA_CONFIG = SRC_ROOT + "/include/common/camera_config.txt";

    }

    namespace Motion {

        // servo config
        inline const std::string MOTION_CONFIG = SRC_ROOT + "/include/common/servo_config.txt";
        // ex motion_set
        inline const std::string MOTION_SET = SRC_ROOT + "/learning_data/motion/motion_set";
        // inner motion_set
        inline const std::string INNER_MOTION_SET = SRC_ROOT + "/learning_data/motion/inner_motion_set";
        
    }

    namespace Test {

        // all_module
        inline const std::string TEST_FILE = SRC_ROOT + "/test/log.csv";

        // signal module
        inline const std::string SPEAKER_TEST_FILE = SRC_ROOT + "/test/log_speakertest.csv";
        inline const std::string MOTION_TEST_FILE = SRC_ROOT + "/test/log_motiontest.csv";
        inline const std::string CAMERA_TEST_FILE = SRC_ROOT + "/test/log_cameratest.csv";
        inline const std::string MICROPHONE_TEST_FILE = SRC_ROOT + "/test/log_microphonetest.csv";

        // text for speaker test
        inline const std::string SPEAKER_TEST = SRC_ROOT + "/test/speaker_test.txt";

    }

}