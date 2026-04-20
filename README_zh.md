# CogniArm: 一款可学习的机器人系统

## Note

该板块正在维护中！请先浏览README.md，谢谢！

---

## 介绍

截至2026/04/13，我们的项目终于迎来了第一版。

一个基于C++的实时嵌入式机器人系统，采用生产者-决策者-执行者架构，支持语音交互、视觉识别与动作学习。

~~详细介绍请看[项目介绍](#cogniarm-a-learnable-robotic-arm-system_)~~

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