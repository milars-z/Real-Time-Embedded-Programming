# CogniArm: A Learnable Robotic Arm System

[English](#english) | [中文](#中文)

---

## English

## Introduction

As of April 13, 2026, the first version of our project has been officially released.

**Latest Update (CogniArm 2.0 – 20 April 2026):**
- Improved the Quick Start section [Quick Start](#quick-start)
- Added hardware connection documentation [Hardware](#hardware-requirements)
- Added an individual module testing section[Module Test](#module-testing)
- Added voice interaction guidance [Voice Interaction](#tips-for-voice-interaction)
- Introduced CogniArm v2.0 highlights with detailed description of updates [v2.0 Highlights](#v20-highlights)

This project is a real-time embedded robotic system based on C++, designed with a Producer–Decision–Executor architecture.  
It supports voice interaction, visual perception, and motion learning.

For a detailed overview, see [Project Overview](#cogniarm-a-learnable-robotic-arm-system_)

---

## 🚀 Features

- 🎤 Speech recognition and command parsing (Microphone + NLU)
- 👁️ Visual detection (OpenCV)
- 🤖 Motion learning and execution system
- 🧠 Producer–Decision–Executor-Supervisor architecture (enhanced in v2.0)
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

- **Supervisor (Feedback Layer)**
  - Task feedback (provides real-time execution status to the Brain)
  - Task latency tracking (records execution time for performance analysis)


---

## 📦 Project Structure

```bash
src/
├── brain/                # Decision layer (core logic)
├── driver/               # Hardware abstraction layer
├── engines/              # Low-level hardware drivers
├── executors/            # Task execution layer
├── producers/            # Input layer
├── system/               # system mode setting(enhanced in v2.0)
├── supervisor/           # Task feedback and task latency record(enhanced in v2.0)
├── kinematics/           # Object grasping(enhanced in v2.0)
├── ui/                   # User interface (LVGL)
├── utils/                # Utility modules


third_party/              # External dependencies
learning_data/            # Learned data (features / motions)
hardware/                 # Hardware design files
docs/                     # Documentation
test/                     # task latency record(enhanced in v2.0)

CMakeLists.txt          
README.md               
main.cpp
setup.sh                  # Automated dependency setup(enhanced in v2.0)

```
## Quick Start

Follow these steps to set up the environment and verify your hardware before running the project.

### step 1: Hardware connection

Please connect the Raspberry Pi 5 to the hardware according to the diagram or table below.


<p align="center">
  <img src="docs/pic/Hardware_connection.png" width="1000">
</p>


#### Raspberry Pi to PCA9685 Connection
| Raspberry Pi Pin | PCA9685 | Function |
|------------------|--------|----------|
| Pin 1            | VCC    | Power    |
| Pin 3            | SDA    | Data     |
| Pin 5            | SCL    | Clock    |
| Pin 6            | GND    | Ground   |



#### Servo Wiring
| Wire Color | Function |
|-----------|----------|
| Brown     | GND      |
| Red       | VCC (+5V)|
| Yellow    | PWM      |

#### Note
> The screen is not required. You can use VNC to connect to and control the Raspberry Pi remotely.
>
> **Do NOT** connect the MG996R servo directly to the Raspberry Pi 5. During operation, please use an external power supply to power the PCA9685 board.

**If you have already assembled the robotic arm**, it may exhibit large or unexpected movements during the initial startup, possibly even exceeding its mechanical limits. This is because the initial servo values may have been modified.

Please interrupt the program during execution using Ctrl + C, and then adjust the initial configuration of each servo in the file *include/common/servo_config.hpp*.

**If you have not yet assembled the robotic arm**, please connect the wiring (servo → PCA9685 → Raspberry Pi 5) before mounting the servos onto the arm, and run the program once.

After that, please do not manually adjust the servo angles when the program is stopped. Thank you.

Minor angle adjustments are fine. The initial angles and angle limits can be modified at any time during later use in *include/common/servo_config.hpp*.

---

### step 2: Clone

After connecting the hardware, please connect the Raspberry Pi 5 to your computer.   
If you encounter any issues at this step, refer to [Setup Guide](docs/helper/setup_guide.md), which contains relevant instructions for connection as well as some Raspberry Pi commands.  

After ensuring that your computer is properly connected to the Raspberry Pi 5, please run the following command to clone our repository.

```bash
git clone https://github.com/milars-z/Real-Time-Embedded-Programming.git
```

---

### Step 3: Environment Setup

libasound2-dev         -- ues for microphone and speaker
nlohmann-json3-dev     -- json file process
libopencv-dev          -- image processing
libsdl2-dev            -- use for screen
libcamera-apps         -- use for camera
gstreamer1.0-libcamera -- use for camera
libcamera-v4l2         -- use for camera
libonnxruntime-dev     -- use for inference engine
cmake                  -- general package
build-essential        -- general package

You can choose either of the following methods to install the required dependencies.

- 1. command(You can manually install any packages you are missing)

```bash
sudo apt update
sudo apt install -y build-essential libasound2-dev nlohmann-json3-dev libopencv-dev
sudo apt install -y libsdl2-dev libcamera-apps gstreamer1.0-libcamera libcamera-v4l2 libonnxruntime-dev
```

- 2. bash

```bash
cd Real-Time-Embedded-Programming
bash setup.sh
```

This step may take a considerable amount of time and could require **more than 10 minutes**. Please be patient.  

---

### Step 4: Camera Verification 
Ensure your camera is correctly connected and recognized by the system.  

```bash
rpicam-hello --list-cameras
```

Check: If no camera is detected, please check the ribbon cable connection.

---

### Step 5: Enable I2C Interface

The robot arm's servo controller requires I2C communication.
Open and edit the configuration file:
```bash
sudo nano /boot/firmware/config.txt
```

Ensure the following line is present and not commented out:
```bash
dtparam=i2c_arm=on
```
Save and Exit (*Ctrl+O*, *Enter*, *Ctrl+X*), then **reboot** your Raspberry Pi.

---

### Step 6: Audio Hardware Configuration

Audio devices often vary. You must verify your speaker and microphone names and update the project configuration if necessary.

- 1.Check Recording Device (Microphone)

```bash
arecord -l
```
Look for the name inside the [].
Requirement: The name should contain UACDemo.
Example: card 2: UACDemoV10 [UACDemoV1.0], device 0...

If it does not contain UACDemo, you must manually update the device string in include/commom/config.h
edit SPEAKER_NAME

- 2.Check Playback Device (Speaker)
```bash
aplay -l
```
Look for the name inside the [].
Requirement: The name should contain USB PnP.
Example: card 2: Device [USB PnP Sound Device], device 0...

If it does not contain USB PnP, you must manually update the device string in include/commom/config.h
edit MIC_NAME

---

### Step 7:build and start!

```bash
cd Real-Time-Embedded-Programming
mkdir build
cd build
cmake ..
make
./main
```

### Note:

The screen is not required for this project, but it is recommended to use one for a better and easier experience (as the NLU and voice control are not very stable at this stage).

If no screen is available, it is recommended to use VNC for viewing and control. The configuration requirements are as follows:

```bash
sudo raspi-config
``` 

choose 3-Interface Options  
choose I3 - VNC  
Yes  

Then you can use VNC to connect to the Raspberry Pi and view the screen in real time.

### After runing

The system should print initialization logs:

<p align="center">
  <img src="docs/pic/Normal_start.png" width="600">
</p>

Or it may directly display the initial interface, as shown in the figure below.

<p align="center">
  <img src="docs/pic/Normal_start_screen.png" width="600">
</p>

You can also check the command output window again to determine whether the program has started successfully. During the startup phase, under normal conditions, all outputs should be in the format **[Init] xxxx**  

If everything is working properly, feel free to enjoy interacting with the robot. You can try clicking motion to teach it a new motion, or camera to let it learn a new object feature. You can also change its name or how it addresses you.
Here are some tips for voice interaction [Tips for Voice Interaction](#tips-for-voice-interaction)



## Hardware Requirements

| Component        | Model / Specification                | Description                          |
|------------------|-------------------------------------|--------------------------------------|
| Main Controller  | Raspberry Pi 5 (4GB)               | Core processing unit                 |
| Camera Module    | Raspberry Pi Camera Module 3       | Vision input                         |
| Microphone       | VEETOP USB Microphone              | Audio input                          |
| Speaker          | USB Speaker                        | Audio output                         |
| Servo Driver     | PCA9685                            | PWM controller (I2C)                 |
| Servos           | MG996R (x3), MG90 (x1)             | Robotic arm actuators                |
| Display (Optional)| OSOYOO 3.5-inch DSI touchscreen   | User interface                       |
| Robotic Arm      | EEZYbotARM Mk2                     | Mechanical structure                 |
| Power Supply     | External 5V Power Supply (for PCA9685) | Required for stable servo operation |

### Note
> It is strongly recommended to use an external power supply for the PCA9685 board instead of powering servos directly from the Raspberry Pi.

## Module Testing

If you encounter persistent errors during the full system test, or if you do not have all the required hardware, you can use individual module testing to experience specific functionalities of the system.

In this project, you can use the following commands to test each module independently:

```bash
./main --test speaker
./main --test camera
./main --test motion
./main --test microphone
```

### Speaker

Used to test the speaker module’s ability to continuously output audio and evaluate the speech processing speed.

We have implemented full text caching in the system. For longer sentences, only the first playback requires significant time to generate PCM data. Subsequent playback will be much faster.

Test data can be found in:
`test/speaker_test.txt`

---

### Camera

This module requires a screen or VNC to view the results.  
You may also try voice control (not recommended).  

Tips for voice interaction can be found in: [Tips for Voice Interaction](#tips-for-voice-interaction)

This test allows you to measure the time required for camera detection.

---

### Motion

This module requires a screen or VNC.  

Since this is an isolated module test, only screen-based interaction is enabled. It is used to measure the response time from when the user presses a button to when the robotic arm begins executing the motion.

---

### Microphone

This module is used to test the microphone’s speech recognition capability as well as the NLU processing performance.

Results can be observed directly in the console output.

---

All test logs can be found in the `test` directory.

We also conducted a series of tests during the development of the project. Detailed test results can be found in docs/test_data

**You may also modify the underlying code to test your own modules or extend the system functionality.**

## Tips for voice interaction

### basic commond

Due to limitations in the current NLU implementation and microphone performance, speech recognition may not be highly accurate. The following commands are reliably recognized and can help you quickly interact with the system using voice:

| Command                            | Intent        | Value  |
|-----------------------------------|--------------|--------|
| `hello`                           | greet                | —      |
| `what's my name`                  | check_host           | —      |
| `what's your name`                | check_robot          | —      |
| `i teach you how to dance`        | learn_motion         | dance  |
| `do motion dance`                 | do_motion            | dance  |
| `this is apple`                   | learn_object         | apple  |
| `find the apple`                  | find_object          | apple  |
| `reset`                           | reset                | —      |
| `update`                          | update_background    | —      |

### Motion Learning Command Mapping

During the motion learning phase, voice commands are mapped into joint control actions as follows:

#### Joint Selection

| Command    | Target Joint |
|------------|-------------|
| `base`     | Base        |
| `shoulder` | Shoulder    |
| `elbow`    | Elbow       |
| `hand`     | Hand        |

---

#### Direction Control

| Command      | Action Direction |
|--------------|------------------|
| `left`       | Move left        |
| `right`      | Move right       |
| `up`         | Move up          |
| `down`       | Move down        |
| `forward`    | Move forward     |
| `back`       | Move backward    |

---

#### Control Commands

| Command            | Function            |
|--------------------|---------------------|
| `finish` / `stop` / `done` | Exit learning mode |

---

> **Note**
> Direction commands are shared across all joints.
>
> It is recommended to avoid using `down`, as it may be confused with `done`.  
> Instead, use `left` or `back` as alternative commands when possible.


### Usage Notes

Please keep the following points in mind during usage. Although the robot provides feedback, these tips will help ensure better performance:

1. Before performing object learning or detection, please update the background and ensure that the scene remains relatively stable. After the robotic arm locates an object, it may block the camera and affect detection. For more reliable results, it is recommended to perform a `reset` beforehand.

2. The robotic arm may occasionally enter motion learning mode unintentionally. In this mode, only motion-related commands will be processed correctly. All other inputs will receive the response: `what's do you mean`.  
You can exit this mode by using the `finish` command or by clicking **confirm** on the screen.

## v2.0 Highlights

### 1. Supervisor Node

In this update, a **Supervisor module** has been introduced. This module feeds the results of motion and camera tasks back to the Brain and provides real-time feedback based on task execution. For example, it can indicate when an object is not found, guide the robotic arm to point to a detected object, and confirm whether a motion has been successfully learned, making the system more intuitive and user-friendly.

With the introduction of the Supervisor, we have also added **task latency measurement**. The execution time of each task is recorded in logs, helping analyze system performance. Detailed test results can be found here: [link]

---

### 2. Module Test

In addition, we have decoupled the system modules and introduced individual module testing. Detailed instructions can be found here: [Module Test](#module-testing).

These tests are lightweight and allow you to observe the true latency of each module without interference from others. This demonstrates that the system is driven by a thread- and callback-based architecture, where tasks are executed independently without blocking each other.

---

### 3. Speaker Optimization

Based on our profiling results, we found that the speaker module introduced significant latency. To address this, we implemented a **speech caching mechanism** in this update.

The system preloads and caches PCM data for commonly used text during startup. In addition, newly encountered text is cached dynamically after first processing. This approach significantly reduces the runtime latency of the speaker module.

Detailed performance data can be found here: [link]

This optimization highlights the trade-off between memory usage and response time, favoring lower latency for interactive performance.

---

### 4. Microphone Pipeline Improvement

Similarly, we improved the reliability of speech recognition. Although a more advanced STT model has not yet been adopted, we optimized the data pipeline by introducing a **buffer queue** between the audio callback and the STT processing stage.

This reduces the workload of the callback function and ensures stable data ingestion during STT processing. As a result, the system becomes more robust under continuous input and is better prepared for integrating more computationally intensive STT models in the future.

---

### 5. Settings Interface

A **Settings interface** has been introduced in this version.

Currently, this page is used to modify the `hostname` and `robotname`. More importantly, it serves as a foundation for future extensions, such as supporting multi-language modes.

In addition, commonly used fixed text has been centralized into a dedicated text configuration, preparing the system for easier expansion and integration of new features in future updates.


## Known Issues

## 📌 Known Issues

The current version still has several limitations:

1. **Large NLU Model Size**  
   The current ONNX-based NLU model is large and resource-intensive,  
   with limited performance in embedded environments.
   The NLU model size has been optimized in v2.0. However, recognition accuracy is still limited and requires further training and data refinement.


2. **~~Speaker Latency~~**  
   ~~The speech feedback system suffers from noticeable latency.~~

   **~~Main reasons:~~**
   ~~- Long text processing takes significant time  ~~
   ~~- Repeated inference for common phrases  ~~

   **~~Future improvements:~~**
   ~~- Introduce caching for frequent phrases  ~~
  ~~ - Support streaming-based speech generation ~~
  This issue has been improved in v2.0. See details in [v2.0 Highlights](#v20-highlights).


3. **Limited Camera Capability**  
   The current vision module can only detect simple features.

   **Future improvements:**
   - More robust detection models  
   - Support dynamic environments

   Camera detection accuracy remains a limitation in v2.0. While larger models could improve accuracy, they introduce higher computational overhead, making it difficult to maintain stable, low-latency performance when multiple modules run concurrently.

   Future improvements will explore deploying more advanced models on higher-performance platforms.

4. **Basic UI Functionality**
    The current UI is relatively simple and mainly supports basic interaction.

    **Planned improvements:**
    ~~- Host/robot name display and configuration~~
    - Language switching (maybe in ver2.5)
    - Visualization of learned motions and detection results (no need now)

5. **Object grasping and precise recognition are not currently supported**

    The current hardware setup does not support object grasping. If the project is further developed in the future, more advanced robotic arms or robotic platforms may be used to enable grasping capabilities.

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
- 👁️ 视觉检测（OpenCV + DNN）
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
- [x] **Kinematics Foundation**: Implemented 3D coordinate system transformations (Homogeneous Transformation, Pixel-to-Base) with dynamic configuration and workspace safety boundaries.
- [x] **System Integration**: Synchronous display between the physical arm and simulation.

### Stage 3: Intelligent Perception (03/10 – 04/03)
- [x] **Voice Control**: Deploy full voice command chain for directional movement and grasping.
- [x] **Object Matching**: Real-time identification of desktop objects against existing image datasets.
- [ ] **Hardware Iteration**: (Optional) Refine 3D printed components for better grip/accuracy.

### Stage 4: Advanced Learning (Independent Track)
- [x] **Learning Workflow**: Expand file system for keyword-based image vector storage and retrieval.

---

## 🚩 Key Milestones
- [ √ ] **Lunar New Year (Feb 15)**: Prototype verification (Physical arm is fully mobile).
- [ √ ] **Early March**: Standalone embedded control (PC-independent).
- [ √ ] **Early April**: Full deployment of Speech and Image Recognition systems.

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
- Coordinate Transformation: Successfully mapping 2D camera pixels to 3D physical base coordinates with safety validations.
---

## ⚠️ Current Limitations

- Motion recording process is cumbersome and inefficient  
- NLU accuracy is limited; text output from the speaker module is not always reliable  
- Camera module can only distinguish objects that are relatively far apart  
- Full Inverse Kinematics (IK) is not yet implemented (though the coordinate transformation baseline is now complete).
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
- 4 完整的 IK (逆运动学) 尚未完全实现（但坐标转换的基础已经打好，预计 4.10 之后继续深入研究）。

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




