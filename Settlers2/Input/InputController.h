#pragma once

#include "../Graphics/Camera.h"
#include "Gamepad.h"
#include <d3dx9math.h>

namespace Logic {

// InputController translates input (mouse/gamepad) to world coordinates
// and handles basic input events for game logic
class InputController
{
public:
    InputController();
    ~InputController();

    void Initialize(Camera* camera, Input::Gamepad* gamepad);
    void Update();

    // Get world position from screen position
    void ScreenToWorld(float screenX, float screenY, float& worldX, float& worldY) const;

    // Get current world cursor position
    void GetWorldCursor(float& worldX, float& worldY) const;

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

    float m_cursorWorldX;
    float m_cursorWorldY;
};

} // namespace Logic
