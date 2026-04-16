#pragma once

enum ErrorCode {
    ERR_NONE        = 0,
    ERR_MIC_INIT    = 1 << 0,
    ERR_SPEAKER_INIT= 1 << 1,
    ERR_CAMERA_INIT = 1 << 2,
    ERR_MOTION_INIT = 1 << 3,
    ERR_NLU_INIT    = 1 << 4,
    ERR_MICMODE_INIT = 1 << 5,
    ERR_SCREEN_INIT = 1 << 6,
};