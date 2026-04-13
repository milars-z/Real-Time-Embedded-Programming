// 该函数在VoiceInteraction中直接与Motor中的代码交流
// 主要完成接受到text后控制电机转动相关的部分
#ifndef MOTIONHANDLE_HPP
#define MOTIONHANDLE_HPP

#include "MotionManager.hpp"
#include <iostream>
#include <string>
#include <regex>
#include <algorithm>

class MotionHandle {

public:

// 语音转motion相关，用来判断text是否为motion的指令
// 如果是则调用内部sendmotion函数,修改cmd中的内容
bool parseMotionCommand(std::string text, MotionTask& cmd);





// 在nlu识别到做指定motion时调用，向Motor模块发送motion set的名字

// 在nlu识别到学习指定motion时调用，改变nlu识别指令，开始以特定格式读取text，主要关注动作与结束标识，而忽略其他指令。


private:

// 如果检测到语音模块为motion,则将提取到的joint和angle发送到Motor侧，创建Motion指令并执行
// 用户输入的指令默认速度为100，运动模式为相对，后续nlu识别更精准在追加
// 这里设计不是很优雅，只在robotcore中构建了motionmanager实例，因此只能生成一个MotionTask然后在voiceinteraction中执行，后续应该会修改吧
// void sendmotion(MotionTask cmd);
std::string toLower(std::string text);


};


// motion相关，用来将



#endif  // MOTIONHANDLE_HPP