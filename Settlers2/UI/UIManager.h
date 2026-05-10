#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include "Widget.h"
#include "../Input/InputManager.h"

namespace UI {

// Менеджер UI
// Управляет всеми виджетами и обработкой ввода
class UIManager
{
public:
    UIManager();
    ~UIManager();

    // Инициализация
    bool Initialize(IDirect3DDevice9* device, Input::InputManager* inputManager);
    void Shutdown();

    // Обновление
    void Update(float deltaTime);

    // Рендеринг
    void Render(IDirect3DDevice9* device);

    // Управление виджетами
    void AddWidget(Widget* widget);
    void RemoveWidget(Widget* widget);
    Widget* GetWidget(size_t index) const;
    size_t GetWidgetCount() const { return m_widgets.size(); }

    // Корневой виджет
    void SetRootWidget(Widget* root) { m_rootWidget = root; }
    Widget* GetRootWidget() const { return m_rootWidget; }

    // Фокус
    void SetFocusedWidget(Widget* widget) { m_focusedWidget = widget; }
    Widget* GetFocusedWidget() const { return m_focusedWidget; }

    // Обработка ввода
    void OnMouseDown(int x, int y);
    void OnMouseUp(int x, int y);
    void OnMouseMove(int x, int y);
    void OnKeyDown(int key);
    void OnKeyUp(int key);

    // Размер экрана
    void SetScreenSize(int width, int height);
    void GetScreenSize(int& width, int& height) const { width = m_screenWidth; height = m_screenHeight; }

private:
    IDirect3DDevice9* m_device;
    Input::InputManager* m_inputManager;

    std::vector<Widget*> m_widgets;
    Widget* m_rootWidget;
    Widget* m_focusedWidget;

    int m_screenWidth;
    int m_screenHeight;

    // Поиск виджета по позиции
    Widget* FindWidgetAt(int x, int y) const;
};

} // namespace UI
