#pragma once

// 用来放一些Bug提示

// Motion_APP层的一些bug
enum class BugCode_M {
    Success = 0,
    NoMotion = -1,
    CannotOpenMotionFile = -2,
    ReadInvalidSet = -3,
    MotionSaveWrong = -4,
    WriteInvalidSet = -5,
    Init = -6
};

