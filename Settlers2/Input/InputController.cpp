#include "stdafx.h"
#include "InputController.h"
#include "../Graphics/Camera.h"
#include "Gamepad.h"

namespace Logic {

InputController::InputController()
    : m_camera(nullptr)
    , m_gamepad(nullptr)
    , m_cursorOffsetX(0.0f)
    , m_cursorOffsetY(0.0f)
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

void InputController::Update(float deltaTime)
{
    (void)deltaTime;
    // Cursor always at camera center — offset removed from left stick
    // (left stick exclusively moves the camera in EditorScene::UpdateCamera)
    m_cursorOffsetX = 0.0f;
    m_cursorOffsetY = 0.0f;
}

void InputController::GetCursorOffset(float& offsetX, float& offsetY) const
{
    offsetX = m_cursorOffsetX;
    offsetY = m_cursorOffsetY;
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
        x = 0.0f; y = 0.0f;
    }
}

void InputController::GetRightStick(float& x, float& y) const
{
    if (m_gamepad) {
        m_gamepad->GetRightStick(x, y);
    } else {
        x = 0.0f; y = 0.0f;
    }
}

} // namespace Logic
