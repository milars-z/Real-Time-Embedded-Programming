# Version 1.0
Integrated speaker and microphone, and added specified replies  
Subsequent versions will introduce a simple NLU for interaction and instruction recognition  
Please check if the folder contents are complete before running  
At present, the Speaker and Microphone modules have not been fully integrated together, so please check if the models in these two modules are complete before use  

## check
```bash
SpeakerTest/
├── build/                  
├── model/                  
│   ├── sherpa-onnx-v1.12.25-linux-aarch64-shared-cpu
│   ├── vits-piper-en_GB-cori-medium-int8
│   └── vits-piper-zh_CN-huayan-medium
│   └── ...
├── Main.cpp               
├── SpeakerEngine.cpp
├── SpeakerEngine.hpp
└── CMakeLists.txt

MicrophoneTest/
├── build/                  
├── model/ 
│   ├── vosk/  
│   │   ├── libvosk.so               
│   │   └── vosk_api.h
│   ├── model/
│   │   ├── am/          
│   │   ├── conf/
│   └── └── ...
├── Main.cpp               
├── MicrophoneEngine.cpp
├── MicrophoneEngine.h
└── CMakeLists.txt

Tools/
├── VisonTools.cpp                  
└── VisonTools.hpp

VoiceInteraction/
├── build/                  
├── CMakeLists.txt
└── main.cpp 

```

## tips
Some of the names are not reasonable and will be improved after integration in the later stage of the project

# Version 2.0
## Chinese
重构了代码结构  
将辅助函数统一移动至VisonTool中管理  
新增了Nlu模块，用来做粗略的语意识别，模型通过python训练获得并通过onnx加载  
新建了TheardSafeQueue用来管理生产消费者模型，解决Microphone-Nlu-Speaker三者在运行过程中堵塞的问题  
文件路径统一写入config_voice.hpp中  

Microphone内部线程：captureThread  
Nlu内部线程：nluThread  
Speaker内部线程：speakerThread//_playbackThread//_synthesisThread  

该模块绑定在core_3中  
Microphone-Nlu-Speaker三者的启动由RobotCore类同意管理，后续调用类初始化并启动即可  
该模块过去的主函数改名为last_main.cpp以防后续需要
下一个版本会将speaker中的双线程通过TheardSafeQueue管理

## English
Refactored Code Structure:

Moved all utility/helper functions to the VisionTool class for unified management.  
Added a new Nlu module for basic intent recognition. The model is trained using Python and loaded via ONNX.  
Created a ThreadSafeQueue to implement the producer-consumer pattern, solving blocking issues between Microphone, Nlu, and Speaker during operation.  
Standardized file paths are now managed in config_voice.hpp.  
Internal Threads:

Microphone: captureThread  
Nlu: nluThread  
Speaker: speakerThread, _playbackThread, _synthesisThread  
Integration:  

This module is bound to core_3.  
The startup of Microphone, Nlu, and Speaker is now centrally managed by the RobotCore class. Future usage only requires calling the class to initialize and start these components.
the main function in past Version was change to last_main.cpp for further use.
The multi thread in speaker will use TheardSafeQueue in the next Version

## check
```bash
VoiceInteraction/
├── build/                 # Build output directory
├── model_nlu/             # NLU model and related resources
│   ├── meta.json          # NLU label definitions (intents & slot types)
│   ├── nlu_model.onnx     # Trained NLU ONNX model
│   └── vocab.txt          # Tokenizer vocabulary file
├── onnxruntime/           # ONNX Runtime dependency (includes headers & libs)
│   ├── include/           # ONNX Runtime header files
│   └── lib/               # ONNX Runtime binary libraries
├── Test_pic/              # Example pictures for testing 
├── CMakeLists.txt         # CMake build configuration file
├── config_voice.hpp       # Global configuration (e.g. path variables, params)
├── last_main.cpp          # Previous version main program (backup/legacy)
├── main.cpp               # Entry point of the main program
├── nlu_test.cpp           # NLU standalone test program
├── nlu_test.hpp           # Header for NLU tests
├── ThreadSafeQueue.hpp    # Thread-safe queue implementation 
├── voiceInteraction.cpp   # Core project logic implementation
├── voiceInteraction.hpp   # Core project header file
└── README.md              # Project documentation
```
# Version 3.0
该模块已合并进micro_motord分支  
子模块运行请进入microTest模块进行单元测试  
cmakelist不沿用  
This module has been integrated into the `micro_motor` branch.  
To test this component, please run the unit tests in the `MicrophoneTest` module.  
The original CMakeLists configuration is deprecated and no longer maintained.  
get nlu model :wget https://huggingface.co/milars/CogniArm_nlu/resolve/main/nlu_model.onnx

