#include "StdAfx.h"
#include "Gamepad.h"
#include <cmath>

namespace Input {

const float Gamepad::DEADZONE = 0.2f;

Gamepad::Gamepad()
    : m_controllerIndex(0)
    , m_connected(false)
    , m_leftStickX(0.0f)
    , m_leftStickY(0.0f)
    , m_rightStickX(0.0f)
    , m_rightStickY(0.0f)
    , m_leftTrigger(0.0f)
    , m_rightTrigger(0.0f)
{
    for (int i = 0; i < GP_ButtonCount; i++)
    {
        m_currentState[i] = false;
        m_previousState[i] = false;
    }
}

Gamepad::~Gamepad()
{
    Shutdown();
}

bool Gamepad::Initialize(int controllerIndex)
{
    m_controllerIndex = controllerIndex;
    m_connected = true;
    return true;
}

void Gamepad::Shutdown()
{
    m_connected = false;
    Clear();
}

void Gamepad::Update()
{
    // Сохраняем предыдущее состояние
    for (int i = 0; i < GP_ButtonCount; i++)
    {
        m_previousState[i] = m_currentState[i];
    }

#ifdef _XBOX
    // Xbox 360 XInput
    XINPUT_STATE state;
    DWORD result = XInputGetState(m_controllerIndex, &state);
    m_connected = (result == ERROR_SUCCESS);

    if (m_connected)
    {
        // Кнопки
        m_currentState[GP_A] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
        m_currentState[GP_B] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0;
        m_currentState[GP_X] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0;
        m_currentState[GP_Y] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0;
        m_currentState[GP_LB] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
        m_currentState[GP_RB] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
        m_currentState[GP_LeftStick] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
        m_currentState[GP_RightStick] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
        m_currentState[GP_DPadUp] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
        m_currentState[GP_DPadDown] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
        m_currentState[GP_DPadLeft] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
        m_currentState[GP_DPadRight] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
        m_currentState[GP_Start] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
        m_currentState[GP_Back] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0;

        // Триггеры
        m_leftTrigger = state.Gamepad.bLeftTrigger / 255.0f;
        m_rightTrigger = state.Gamepad.bRightTrigger / 255.0f;
        m_currentState[GP_LT] = m_leftTrigger > 0.5f;
        m_currentState[GP_RT] = m_rightTrigger > 0.5f;

        // Стики
        float lx = state.Gamepad.sThumbLX / 32767.0f;
        float ly = state.Gamepad.sThumbLY / 32767.0f;
        float rx = state.Gamepad.sThumbRX / 32767.0f;
        float ry = state.Gamepad.sThumbRY / 32767.0f;

        // Применяем мертвую зону
        float lMag = sqrtf(lx * lx + ly * ly);
        if (lMag > DEADZONE)
        {
            float scale = (lMag - DEADZONE) / (1.0f - DEADZONE);
            m_leftStickX = lx * scale;
            m_leftStickY = -ly * scale;  // Инвертируем ось Y
        }
        else
        {
            m_leftStickX = 0.0f;
            m_leftStickY = 0.0f;
        }

        float rMag = sqrtf(rx * rx + ry * ry);
        if (rMag > DEADZONE)
        {
            float scale = (rMag - DEADZONE) / (1.0f - DEADZONE);
            m_rightStickX = rx * scale;
            m_rightStickY = -ry * scale;  // Инвертируем ось Y
        }
        else
        {
            m_rightStickX = 0.0f;
            m_rightStickY = 0.0f;
        }
    }
#else
    // Windows XInput
    // Для Windows нужно подключить XInput библиотеку
    // Пока заглушка
    m_connected = false;
#endif
}

bool Gamepad::IsButtonDown(GamepadButton button) const
{
    if (button >= 0 && button < GP_ButtonCount)
        return m_currentState[button];
    return false;
}

bool Gamepad::IsButtonPressed(GamepadButton button) const
{
    if (button >= 0 && button < GP_ButtonCount)
        return m_currentState[button] && !m_previousState[button];
    return false;
}

bool Gamepad::IsButtonReleased(GamepadButton button) const
{
    if (button >= 0 && button < GP_ButtonCount)
        return !m_currentState[button] && m_previousState[button];
    return false;
}

void Gamepad::GetLeftStick(float& x, float& y) const
{
    x = m_leftStickX;
    y = m_leftStickY;
}

void Gamepad::GetRightStick(float& x, float& y) const
{
    x = m_rightStickX;
    y = m_rightStickY;
}

float Gamepad::GetLeftTrigger() const
{
    return m_leftTrigger;
}

float Gamepad::GetRightTrigger() const
{
    return m_rightTrigger;
}

bool Gamepad::IsConnected() const
{
    return m_connected;
}

void Gamepad::Clear()
{
    for (int i = 0; i < GP_ButtonCount; i++)
    {
        m_currentState[i] = false;
        m_previousState[i] = false;
    }
    m_leftStickX = 0.0f;
    m_leftStickY = 0.0f;
    m_rightStickX = 0.0f;
    m_rightStickY = 0.0f;
    m_leftTrigger = 0.0f;
    m_rightTrigger = 0.0f;
}

} // namespace Input
