#pragma once
#include <d3d9.h>

namespace Input {

// Кнопки геймпада
enum GamepadButton
{
    GP_A = 0,
    GP_B,
    GP_X,
    GP_Y,
    GP_LB,
    GP_RB,
    GP_LT,
    GP_RT,
    GP_LeftStick,
    GP_RightStick,
    GP_DPadUp,
    GP_DPadDown,
    GP_DPadLeft,
    GP_DPadRight,
    GP_Start,
    GP_Back,
    GP_ButtonCount
};

// Геймпад (работает на Xbox 360 и Windows)
class Gamepad
{
public:
    Gamepad();
    ~Gamepad();

    // Инициализация
    bool Initialize(int controllerIndex);
    void Shutdown();

    // Обновление состояния
    void Update();

    // Проверка состояния кнопок
    bool IsButtonDown(GamepadButton button) const;
    bool IsButtonPressed(GamepadButton button) const;
    bool IsButtonReleased(GamepadButton button) const;

    // Получить положение стиков
    void GetLeftStick(float& x, float& y) const;
    void GetRightStick(float& x, float& y) const;

    // Получить положение триггеров
    float GetLeftTrigger() const;
    float GetRightTrigger() const;

    // Проверка подключения
    bool IsConnected() const;

    // Очистка состояния
    void Clear();

private:
    int m_controllerIndex;
    bool m_connected;

    bool m_currentState[GP_ButtonCount];
    bool m_previousState[GP_ButtonCount];

    float m_leftStickX;
    float m_leftStickY;
    float m_rightStickX;
    float m_rightStickY;
    float m_leftTrigger;
    float m_rightTrigger;

    // Порог мертвой зоны для стиков
    static const float DEADZONE;
};

} // namespace Input
