# Version1.0
## before run this code  
```bash
sudo apt-get install libasound2-dev
sudo apt-get install libespeak-ng-devq
```

```bash
mkdir build
cd build
cmake ..
make
./SpeakerTest
```

# Version2.0
## tips
Update Espeak to Sherpa-Onnx to get a better experience
before use the speaker please check the structure of file
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
```

## how to download model
### Inference Engine
```bash
mkdir model
cd model
# download sherpa
wget https://github.com/k2-fsa/sherpa-onnx/releases/download/v1.12.25/sherpa-onnx-v1.12.25-linux-aarch64-shared-cpu.tar.bz2
# unzip
tar xvf sherpa-onnx-v1.12.25-linux-aarch64-shared-cpu.tar.bz2
# delete tar
rm sherpa-onnx-v1.12.25-linux-aarch64-shared-cpu.tar.bz2
# V1.12.25 dont have .h file in tar
cd sherpa-onnx-v1.12.25-linux-aarch64-shared-cpu
mkdir include
wget https://raw.githubusercontent.com/k2-fsa/sherpa-onnx/v1.12.25/sherpa-onnx/c-api/c-api.h
```

### Language Model
```bash
mkdir model
cd model
# download chinese model
wget https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-piper-zh_CN-huayan-medium.tar.bz2
# unzip
tar xvf vits-piper-zh_CN-huayan-medium.tar.bz2
# delete tar
rm vits-piper-zh_CN-huayan-medium.tar.bz2
```

```bash
mkdir model
cd model
# download english model
wget https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-piper-en_GB-cori-medium-int8.tar.bz2
# unzip
tar xvf vits-piper-en_GB-cori-medium-int8.tar.bz2
# delete tar
rm vits-piper-en_GB-cori-medium-int8.tar.bz2
```



