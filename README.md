# CogniArm: A Learnable Robotic Arm System

[English](#english) | [中文](#中文)

---

## English

## Introduction

As of April 13, 2026, the first version of our project has been officially released.

This project is a real-time embedded robotic system based on C++, designed with a Producer–Decision–Executor architecture.  
It supports voice interaction, visual perception, and motion learning.

For a detailed overview, see [Project Overview](#cogniarm-a-learnable-robotic-arm-system_)

---

## 🚀 Features

- 🎤 Speech recognition and command parsing (Microphone + NLU)
- 👁️ Visual detection and object tracking (OpenCV + DNN)
- 🤖 Motion learning and execution system
- 🧠 Producer–Decision–Executor architecture
- 🖥️ Embedded UI based on LVGL

---

## 🏗️ System Architecture

The system adopts a three-layer architecture:

See details: [System Architecture](#version-update-v20)

- **Producer (Input Layer)**
  - Microphone (speech input)
  - Camera (visual input)
  - Screen (UI input)

- **Brain (Decision Layer)**
  - RobotBrain (state machine + task scheduling)

- **Executor (Execution Layer)**
  - MotionExecutor (motion execution)
  - SpeakerExecutor (speech feedback)
  - CameraExecutor (feature matching)

---

## 📦 Project Structure

```bash
src/
├── brain/                # Decision layer (core logic)
├── driver/               # Hardware abstraction layer
├── engines/              # Low-level hardware drivers
├── executors/            # Task execution layer
├── producers/            # Input layer
├── ui/                   # User interface (LVGL)
├── utils/                # Utility modules

third_party/              # External dependencies
learning_data/            # Learned data (features / motions)
hardware/                 # Hardware design files
docs/                     # Documentation

CMakeLists.txt          
README.md               
main.cpp

```

## ⚙️ Usage

### Notes  
Please make sure all third-party dependencies are installed before running.
link:[Third_party](https://github.com/milars-z/Real-Time-Embedded-Programming/releases/download/1.0/third_party.zip)
Extract the third_party folder to the same level as src.
Due to the large size of the NLU model, reproduction is not recommended at this stage.
Please wait for version 2.0 for optimized support.


### Software Setup
```bash
git clone <https://github.com/milars-z/Real-Time-Embedded-Programming.git>
cd Real-Time-Embedded-Programming
mkdir build
cd build
cmake ..
make -j4
./main
```

### Hardware Requirements
Recommended hardware configuration:

Main Platform: Raspberry Pi 5 (4GB)  
Camera: Raspberry Pi Camera Module 3  
Microphone: VEETOP USB Microphone  
Speaker: USB Speaker  
Servo / PWM Control: PCA9685 + MG996 ×3 + MG90 ×1  
Display (Optional): OSOYOO 3.5-inch DSI Touchscreen  
Robotic Arm: EEZYbotARM Mk2 

## Known Issues

## 📌 Known Issues

The current version still has several limitations:

1. **Large NLU Model Size**  
   The current ONNX-based NLU model is large and resource-intensive,  
   with limited performance in embedded environments.

2. **Speaker Latency**  
   The speech feedback system suffers from noticeable latency.

   **Main reasons:**
   - Long text processing takes significant time  
   - Repeated inference for common phrases  

   **Future improvements:**
   - Introduce caching for frequent phrases  
   - Support streaming-based speech generation  

3. **Limited Camera Capability**  
   The current vision module can only detect simple features.

   **Future improvements:**
   - More robust detection models  
   - Support dynamic environments

4. **Basic UI Functionality**
    The current UI is relatively simple and mainly supports basic interaction.

    **Planned improvements:**
    - Host/robot name display and configuration
    - Language switching
    - Visualization of learned motions and detection results

---

## 📄 License

GPL License

---

## 中文

截至2026/04/13，我们的项目终于迎来了第一版。

一个基于C++的实时嵌入式机器人系统，采用生产者-决策者-执行者架构，支持语音交互、视觉识别与动作学习。

详细介绍请看[项目介绍](#cogniarm-a-learnable-robotic-arm-system_)

## 功能特性

- 🎤 语音识别与指令解析（Microphone + NLU）
- 👁️ 视觉检测与目标跟踪（OpenCV + DNN）
- 🤖 Motion 动作学习与执行系统
- 🧠 Producer-Decision-Executor 架构设计
- 🖥️ 基于 LVGL 的嵌入式 UI

## 系统架构

系统采用三层架构：

详见[System_Architecture](#version-update-v20)

- Producer（输入层）
  - Microphone（语音输入）
  - Camera（视觉输入）
  - Screen（UI输入）

- Brain（决策层）
  - RobotBrain（状态机 + 任务调度）

- Executor（执行层）
  - MotionExecutor (动作执行)
  - SpeakerExecutor (语音反馈)
  - CameraExecutor (图像特征匹配)

---

## 项目结构
```bash
src/
├── brain/                # 决策层（核心大脑）
│   ├── RobotBrain.cpp        # 主控制逻辑 / 状态机
│   ├── NluHandle.cpp         # 语义解析（NLU）
│   └── cogniArm.cpp          # 行为/认知控制（高层逻辑）
│
├── driver/               # 硬件驱动抽象层
│   ├── camera/
│   │   ├── CameraHandle.cpp      # 摄像头接口封装
│   │   ├── AttentionDetector.cpp # 注意力检测
│   │   └── FeatureManager.cpp    # 特征管理
│   └── motion/
│       └── MotionManager.cpp     # 运动控制（关节/动作）
│
├── engines/              # 底层执行引擎（硬件控制）
│   ├── camera/
│   │   └── CameraEngine.cpp      # 摄像头底层驱动
│   ├── microphone/
│   │   └── MicrophoneEngine.cpp  # 麦克风输入
│   ├── motion/
│   │   └── PwmBoardController.cpp # PWM 控制（舵机）
│   └── speaker/
│       └── SpeakerEngine.cpp     # 音频输出
│
├── executors/            # 执行层（任务执行器）
│   ├── camera/
│   │   └── CameraApp.cpp
│   ├── motion/
│   │   └── MotionApp.cpp
│   └── speaker/
│       └── SpeakerApp.cpp
│
├── producers/            # 输入层（数据生产者）
│   ├── microphone/
│   │   └── MicrophoneApp.cpp     # 语音输入
│   └── ui/
│       └── ScreenApp.cpp         # UI 输入
│
├── ui/                   # 用户界面
│   └── Screen_ui.cpp         # LVGL界面实现
│
├── utils/                # 工具模块
└──  └── VisionTools.cpp       # 视觉工具函数

third_party/         # 第三方依赖库
├── camera/          # mobilenet_v2_slice.onnx模型
├── lvgl/            # LVGL 图形库
├── lvgl_lib/        # LVGL 静态库（liblvgl.a）
├── microphone/      # 麦克风相关依赖（vosk）
├── model_nlu/       # 本地语义理解模型（NLU.onnx）
├── onnxruntime/     # ONNX  推理引擎
└── speaker/         # 音频输出相关依赖 (sherpa/vits)

learning_data/
├── camera/
│    └── features/ 
│        └──my_features.yml #(图像特征信息)
├── motion/
│    └── motion_set/
│           └──hello.json #(动作组记录)
└──         └── 

hardware/
├── cad/
└── list/

docs/
├── hardware_config/
└── Project_Planning/

CMakeLists.txt          
README.md               
main.cpp

```

---

## 如何使用？

### 注意！
- 使用前请务必确定安装好三方依赖，可直接点击一下链接进行下载并确保third_party文件夹与src文件夹在同一级目录
- link:[Third_party](https://github.com/milars-z/Real-Time-Embedded-Programming/releases/download/1.0/third_party.zip)
- NLU模型过大，暂时不推荐复现，请耐心等到2.0版本后，谢谢！

### 软件
```bash
git clone <https://github.com/milars-z/Real-Time-Embedded-Programming.git>
cd Real-Time-Embedded-Programming
mkdir build
cd build
cmake ..
make -j4
./main
```

### 硬件

本项目基于嵌入式平台开发，推荐硬件配置如下：

- **主控平台**：Raspberry Pi 5 4GB 
- **摄像头模块**：Raspberry Pi Camera Module 3
- **麦克风**：VEETOP USB Microphone
- **扬声器**：USB Speaker
- **舵机 / PWM控制板**：PCA9685 MG996(x3) MG90(x1)
- **显示屏（可选）**: OSOYOO 3.5Inch DSI touchscreen
- **机械臂**：EEZYbotARM-Mk2

---

## 已知问题

当前版本仍存在以下问题：

### 1. NLU模型体积较大
当前使用的ONNX语义理解模型体积较大，效果也不好。
急需优化解决

### 2. Speaker语音反馈存在延迟
当前语音反馈模块在生成语音时存在一定延迟，
对于人机交互场景而言响应速度仍不足。
主要原因包括两个：

1、长text单次处理事件较长，需进行拆分持续输出  
2、常用text反复推理，需保存常用text的音频数据  

后续可通过引入常用语缓存或流式生成方式进行优化。

### 3. Camera模块能力不足
现在的Cam模块仅能检测浅显的特征，对于复杂图像的判断能力不足，且需要固定背景
后续更新会追加对移动式背景的检测与特征记录，增加追踪功能

### 4. Screen
当前 UI 界面功能较为简单，主要用于基础交互展示，
在信息呈现与交互体验方面仍有提升空间。
后续会更新：
1、host/robot名称显示与配置
2、语言切换
3、detect/Do模块会显示现在已经保存的动作

---

## License

GPL License

---


# CogniArm: A Learnable Robotic Arm System_

> **A cross-disciplinary embedded system project integrating Computer Vision, Speech Recognition, and Robotic Kinematics.**

## 📖 Project Overview
CogniArm is an intelligent robotic arm system designed with **learning capabilities**. Unlike traditional robots with fixed routines, CogniArm can be "taught" to recognize new objects through user interaction, storing visual features for future retrieval and autonomous grasping.

---

## 🏗 System Architecture
The system is built on five modular pillars:
*   **Vision Module**: Handles object detection and feature extraction. Includes *Learning Mode* (saving central object features) and *Recognition Mode* (vector matching).
*   **Speech Recognition Module**: Processes user voice commands and extracts keywords for instruction generation or visual search.
*   **AI Module**: Acts as the **Feature Extraction Engine**, supporting both visual and auditory intelligence.
*   **File Management Module**: Manages the persistence of pre-stored information groups and feature vectors.
*   **Control Module**: Executes motion planning and hardware-level control.

---

## 📅 Development Roadmap

### Stage 1: Hardware Foundation & Simulation (01/28 – 02/15)
- [x] **Physical Assembly**: 3D print and assemble the **EEZYbotARM Mk2**.
    - *Ref:* [Instructables Guide](https://www.instructables.com/EEZYbotARM-Mk2-3D-Printed-Robot/) | [Thingiverse Files](https://www.thingiverse.com/thing:1454048)
- [x] ~~**Digital Twin**: Implement Python-based simulation (PyQt/PySide) for synchronized movement.(This module will be developed in the middle of the project)~~
- [x] **Low-level Control**: Establish Raspberry Pi GPIO control with multi-threading.
- [x] **Speech/Vision Baseline**: Implement basic keyword extraction and image matching benchmarks (accuracy & blurriness analysis).

### Stage 2: Embedded Deployment & Kinematics (02/23 – 03/09)
- [x] **Embedded Migration**: Transition control from PC to standalone embedded deployment.
- [ ] ~~**Mathematics**: Implement **Inverse Kinematics (IK)** and coordinate system transformations.~~
- [x] **System Integration**: Synchronous display between the physical arm and simulation.

### Stage 3: Intelligent Perception (03/10 – 04/03)
- [x] **Voice Control**: Deploy full voice command chain for directional movement and grasping.
- [x] **Object Matching**: Real-time identification of desktop objects against existing image datasets.
- [ ] **Hardware Iteration**: (Optional) Refine 3D printed components for better grip/accuracy.

### Stage 4: Advanced Learning (Independent Track)
- [ ] **Learning Workflow**: Expand file system for keyword-based image vector storage and retrieval.

---

## 🚩 Key Milestones
- [ √ ] **Lunar New Year (Feb 15)**: Prototype verification (Physical arm is fully mobile).
- [ √ ] **Early March**: Standalone embedded control (PC-independent).
- [ ] **Early April**: Full deployment of Speech and Image Recognition systems.

---

## 🛠 Final Phase & Maintenance
The final weeks will be dedicated to:
*   **Optimization**: Bug fixes and accuracy calibration.
*   **Documentation**: Code standardization, GitHub repository refinement, and **Thesis writing**.
*   **DevLogs**: Multimedia video documentation will be synchronized for each stage to showcase progress.

---

### 📝 Note
*Stage 4 development is handled as an independent advanced track.*


## 📌 Progress Summary (as of 02/04)  V 1.5

As of April 2th, the basic system framework has been successfully established.  
The intended architecture is shown in the diagram below.
![Architecture](docs/pic/architecture.png)

### ✅ Current Capabilities

- Robotic arm can learn and execute predefined motion sets  
- Object detection and recognition are functional  
- Basic speech recognition and interaction are implemented  

---

## ⚠️ Current Limitations

- Motion recording process is cumbersome and inefficient  
- NLU accuracy is limited; text output from the speaker module is not always reliable  
- Camera module can only distinguish objects that are relatively far apart  
- NO IK！

---

## 🚀 Next Steps

### 1. Interface & Usability
- Introduce a screen interface  
- Enable manual motion recording  
- Support remote visualization of camera detection results  
- Goal: Fully operate without reliance on a PC  

### 2. NLU Optimization
- Improve model accuracy and intent recognition  
- Enhance robustness of speech-to-text pipeline  

### 3. Camera Module Enhancement
- Improve detection accuracy  
- Allocate more memory for better performance  
- Optimize usage since the camera is only activated during detection/recognition tasks  

### Note
- The current 3D-printed model is sufficient for now, so no reprinting was required.

---

## 📊 Notes

- Current system focuses on modular integration  
- Next phase emphasizes optimization, usability, and system independence  


## chinese  V 1.5 

截至4月2日，已完成基本框架的搭建，理想框架如下图 机械臂可以实现motion动作组的学习，物品的识别与检测，语音识别交互功能 

现版本的3D打印模型暂时够用，因此没有再次打印 

仍存在问题如下： 
- 1 motion动作录入繁琐 
- 2 nlu识别不够精准，speaker模块获取到的text不够精准 
- 3 cam模块只能比较两个距离较远的物体 
- 4 没有做IK，应该得等到4.10之后研究了

下一阶段将对这几个模块进行升级优化 
- 1 尝试加入screen，手动操作录入motion，并且可以远程查看cam的检测结果，完全脱离pc 
- 2 优化nlu模型 
- 3 优化cam模型，现阶段仅在调用cam进行检测/识别的时候使用cam模块，因此可以考虑多使用一部分内存来换取更高的精度


## Version Update V2.0

This release introduces a major refactor of the system architecture, transitioning to a structured **Producer–Brain–Executor** model.

The overall system workflow is illustrated in the diagram above.

## System_Architecture

![Architecture_02](docs/pic/architecture_02.png)

### Key Changes

- Introduced a Screen module
- Unified input handling by treating both Screen and Microphone as producer modules
- Part of the Screen module was designed with AI assistance
- Updated camera parameters for improved performance

### Improvements

- Conducted a full system-level review to ensure proper thread lifecycle management
- For more details, please refer to docs/Project_Planning/overall.xlsx

![Thread_check](docs/pic/thread_check.png)

### Known Issues

- Thread start/stop behaviors are not yet fully unified
- This will be addressed in future updates

## 版本更新 V2.0

4月9日

该版本重构了原代码解构，完善了生产者决策者执行者的模型设计  
具体逻辑构造如图（英文版中）
### 修改点如下  
- 增加了screen  
- screen和microphone模块都作为生产者输入指令 
- screen模块部分设计由AI辅助完成  

- 修改了camera的参数

### 优化
- 同时对设计进行了整体检查，确保了线程生命周期完整
- 同时也暴露出了一些问题，线程的开始与结束事件并不统一，将会在后续版本中修改完善




