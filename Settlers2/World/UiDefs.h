#pragma once
#include <stdint.h>
#include <d3d9.h>

#ifndef XINPUT_GAMEPAD_A
#define XINPUT_GAMEPAD_A    0x1000
#define XINPUT_GAMEPAD_B    0x2000
#define XINPUT_GAMEPAD_X    0x4000
#define XINPUT_GAMEPAD_Y    0x8000
#endif

namespace World {
    static const size_t MAX_UI_POPUPS = 4;

    struct PopupUiData {
        char title[32];
        char line1[32];
        char line2[32];
        int tileX;
        int tileY;
        float timer;
        bool isVisible;
    };
}
