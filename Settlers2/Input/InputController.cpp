#include "stdafx.h"
#include "InputController.h"
#include "../Graphics/Camera.h"
#include "Gamepad.h"

namespace Logic {

InputController::InputController()
: m_camera(nullptr), m_gamepad(nullptr), m_cursorWorldX(0.0f), m_cursorWorldY(0.0f)
{
}

InputController::~InputController()
{
}

void InputController::Initialize(Camera* camera, Input::Gamepad* gamepad)
{
    m_camera = camera;
    m_gamepad = gamepad;
}

void InputController::Update()
{
    if (!m_camera || !m_gamepad) return;

    // Get left stick position
    float stickX, stickY;
    m_gamepad->GetLeftStick(stickX, stickY);

    // Convert stick position to screen cursor position
    // Stick range: -1.0 to 1.0
    // Screen position: 0 to screenWidth/Height
    float screenX = (stickX + 1.0f) * 0.5f * m_camera->GetScreenWidth();
    float screenY = (1.0f - stickY) * 0.5f * m_camera->GetScreenHeight();

    // Convert to world coordinates
    ScreenToWorld(screenX, screenY, m_cursorWorldX, m_cursorWorldY);
}

void InputController::ScreenToWorld(float screenX, float screenY, float& worldX, float& worldY) const
{
    if (m_camera) {
        m_camera->ScreenToWorld(screenX, screenY, worldX, worldY);
    } else {
        worldX = screenX;
        worldY = screenY;
    }
}

void InputController::GetWorldCursor(float& worldX, float& worldY) const
{
    worldX = m_cursorWorldX;
    worldY = m_cursorWorldY;
}

bool InputController::IsButtonAPressed() const
{
    return m_gamepad ? m_gamepad->IsButtonPressed(Input::GP_A) : false;
}

bool InputController::IsButtonBPressed() const
{
    return m_gamepad ? m_gamepad->IsButtonPressed(Input::GP_B) : false;
}

bool InputController::IsButtonXPressed() const
{
    return m_gamepad ? m_gamepad->IsButtonPressed(Input::GP_X) : false;
}

bool InputController::IsButtonYPressed() const
{
    return m_gamepad ? m_gamepad->IsButtonPressed(Input::GP_Y) : false;
}

void InputController::GetLeftStick(float& x, float& y) const
{
    if (m_gamepad) {
        m_gamepad->GetLeftStick(x, y);
    } else {
        x = 0.0f;
        y = 0.0f;
    }
}

void InputController::GetRightStick(float& x, float& y) const
{
    if (m_gamepad) {
        m_gamepad->GetRightStick(x, y);
    } else {
        x = 0.0f;
        y = 0.0f;
    }
}

} // namespace Logic
