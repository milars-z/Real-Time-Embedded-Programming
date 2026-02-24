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