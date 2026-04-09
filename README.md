# CogniArm: A Learnable Robotic Arm System

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




