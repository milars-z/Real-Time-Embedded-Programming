# Architecture

本文件包含了系统的整体架构设计、模块间的交互逻辑以及核心设计模式的实现方案。它旨在帮助开发者深入理解项目的底层逻辑与 SOLID 设计原则的应用

This document covers the high-level system architecture, inter-module interactions, and the implementation of core design patterns. It is intended to help developers gain a deep understanding of the project's underlying logic and the application of SOLID principles.

## Table Of Contents

- [System Architecture](#system-architecture)
- [Technical Documentation](#technical-documentation)
- [Project Structure](#project-structure)
- [Known Issues](#known-issues)

##  System Architecture

The system adopts a three-layer architecture:

~~See details: [System Architecture](./HISTORY.md#version-update-v20)~~

(update in version 2.0 -- CogniArm structure 3.0)

<p align="center">
  <img src="pic/System_Architecture_30.png" width="1000">
</p>

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

[back to Contents](#table-of-contents)
[back to Main Contents](../README.md#-table-of-contents)

---

## Technical Documentation

During the version update process, a number of technical documents were designed and organized under the path:
 ***docs/Technical_Documentation***

### Memory_Management

To ensure orderly system startup and shutdown during runtime, and to prevent memory leaks, a System Lifecycle & Execution Flow was designed.
This guarantees that thread initialization follows the correct order relative to object instantiation, and that all threads are safely stopped and properly released.

***docs/Technical_Documentation/Memory_Management.xlsx***

### SOLID_Analyze

In accordance with course requirements, a SOLID design analysis was conducted on all classes and functions within the system.  
Although some parts of the design do not fully comply with SOLID principles, the analysis provides a clear evaluation of the current structure. 

***docs/Technical_Documentation/SOLID_Analyze.xlsx***

### System_Architecture

The system architecture diagram has been redesigned for the latest version, providing a clear overview of the overall system structure. 

***docs/Technical_Documentation/System_Architecture.xlsx***

### Thread_Analyze

A comprehensive analysis of all thread lifecycles was conducted to ensure that threads are initialized within a unified function.  
All threads are explicitly bound to designated CPU cores to guarantee controlled execution and consistency.

***docs/Technical_Documentation/Thread_Analyze.xlsx***

### Sleep_Analyze

we conducted a thorough review of all sleep/usleep usages within the project.

We acknowledge that a small number of such calls still exist in the current version.   
However, these are not used to implement core task scheduling logic, but are instead the result of deliberate engineering trade-offs based on the following considerations

***docs/Technical_Documentation/Sleep_Analyze.xlsx***

### Latency_Assessment

We conducted latency evaluations on the main system modules,

From the data, it is evident that, following our optimizations, the execution time of the speaker module has been significantly reduced.

detailed results provided in 

***docs/test_data/Analysis.xlsx***.

[back to Contents](#table-of-contents)
[back to Main Contents](../README.md#-table-of-contents)

---

## Project Structure

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
[back to Contents](#table-of-contents)
[back to Main Contents](../README.md#-table-of-contents)

---

## Known Issues

The current version still has several limitations:

1. **Large NLU Model Size**  
   The current ONNX-based NLU model is large and resource-intensive,  
   with limited performance in embedded environments.
   The NLU model size has been optimized in v2.0. However, recognition accuracy is still limited and requires further training and data refinement.


2. **~~Speaker Latency~~**  
   ~~The speech feedback system suffers from noticeable latency.~~

   This issue has been improved in v2.0. See details in [v2.0 Highlights](./ROADMAP.md#v20-highlights).


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

6. **Code Standards**
    Current code lacks proper comments following the Doxygen standard, which increases the difficulty for future maintenance.

    solve:Complete and translate comments for core modules according to Doxygen specifications.

7. **SOLID specification**
    Some structures (e.g., pwmboardcontroller, motionManager) do not comply with SOLID principles  
    leading to high coupling and poor scalability.

    solve: To ensure long-term development, we have re-architected these modules.[Future Work](./ROADMAP.md#future-work)

8. **Develop commit**
    During the transition from module-based development to the Producer–Brain–Executor architecture, a major merge was performed from the `integration_micro_motor` branch into `main`.

    This merge consolidated previous development history, which may make the commit timeline appear partially non-continuous.  

    If any development records around April seem missing, please refer to the `integration_micro_motor` branch for the complete history.

[back to Contents](#table-of-contents)
[back to Main Contents](../README.md#-table-of-contents)

---