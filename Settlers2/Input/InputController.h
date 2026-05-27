#pragma once

#include "../Graphics/Camera.h"
#include "Gamepad.h"

namespace Logic {

class InputController
{
public:
    InputController();
    ~InputController();

    void Initialize(Camera* camera, Input::Gamepad* gamepad);
    void Update(float deltaTime);

    // Get cursor offset from camera center
    void GetCursorOffset(float& offsetX, float& offsetY) const;

    // Check if input is active (gamepad connected or mouse active)
    bool IsActive() const { return m_camera != nullptr && m_gamepad != nullptr; }

    // Button states
    bool IsButtonAPressed() const;
    bool IsButtonBPressed() const;
    bool IsButtonXPressed() const;
    bool IsButtonYPressed() const;

    // Stick positions
    void GetLeftStick(float& x, float& y) const;
    void GetRightStick(float& x, float& y) const;

private:
    Camera* m_camera;
    Input::Gamepad* m_gamepad;

    float m_cursorOffsetX;
    float m_cursorOffsetY;
};

} // namespace Logic
