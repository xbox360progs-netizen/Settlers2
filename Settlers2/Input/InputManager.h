#pragma once
#include <d3d9.h>
#include <string>
#include "Gamepad.h"

namespace Input {

// Менеджер ввода
// Централизует управление всеми устройствами ввода
class InputManager
{
public:
    InputManager();
    ~InputManager();

    // Инициализация
    bool Initialize(HWND hWnd, IDirect3DDevice9* device);
    void Shutdown();

    // Обновление состояния
    void Update();

    // Получить устройства
    Gamepad* GetGamepad() { return m_gamepad; }


    // Очистка состояния
    void Clear();

private:
    HWND m_hWnd;
    IDirect3DDevice9* m_device;

    Gamepad* m_gamepad;
};

} // namespace Input
