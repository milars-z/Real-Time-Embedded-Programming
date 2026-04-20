# Test

本文件详细说明了各个核心模块的测试方法，并记录了通过单元测试所得出的自检结果。

This document outlines the testing methodology for each core module and provides a detailed record of the unit testing results obtained during our self-inspection process.

## Table Of Contents

- [Module Testing](#module-testing)
- [Latency Testing](#latency-testing)

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

Tips for voice interaction can be found in: [Tips for Voice Interaction](../../README.md#tips-for-voice-interaction)

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

[back to Main Contents](../../README.md#-table-of-contents)

[back to Contents](#table-of-contents)


## Latency Testing
![Whole system test and Seperate system test](docs/pic/System_Latency_test.png)

The Camera, STT, and Screen-motion modules maintain consistent execution times across both integrated system tests and standalone module tests, indicating that these components operate independently without cross-module interference. 

The TTS and Nlu modules—which lacked caching mechanisms in early testing—are experiencing severe delays caused by significant queue latency.

![Caching mechanism TTS test](docs/pic/Caching_TTS_Latency.png)

Sentence "i fine the tissue" only uses 0.131ms in second generation, while in first generation it takes about 800ms.Using cache instead of generating entire sentence can significantly reduce time consumption,cause an obvious drop on average time cost.




[back to Main Contents](../../README.md#-table-of-contents)

[back to Contents](#table-of-contents)


