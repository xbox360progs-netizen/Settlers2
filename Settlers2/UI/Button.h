#pragma once
#include "Widget.h"
#include <string>

namespace UI {

// Callback для нажатия кнопки
typedef void (*ButtonCallback)(void* userData);

// Кнопка
class Button : public Widget
{
public:
    Button();
    virtual ~Button();

    // Инициализация
    virtual bool Initialize(IDirect3DDevice9* device);
    virtual void Shutdown();

    // Обновление
    virtual void Update(float deltaTime);

    // Рендеринг
    virtual void Render(IDirect3DDevice9* device);

    // Обработка ввода
    virtual void OnMouseDown(int x, int y);
    virtual void OnMouseUp(int x, int y);
    virtual void OnMouseMove(int x, int y);

    // Текст кнопки
    void SetText(const std::string& text);
    const std::string& GetText() const { return m_text; }

    // Callback
    void SetCallback(ButtonCallback callback, void* userData = NULL);

    // Состояние
    bool IsClicked() const { return m_clicked; }
    void ResetClicked() { m_clicked = false; }

    // Стиль
    void SetNormalColor(D3DCOLOR color) { m_normalColor = color; }
    void SetHoverColor(D3DCOLOR color) { m_hoverColor = color; }
    void SetPressedColor(D3DCOLOR color) { m_pressedColor = color; }
    void SetDisabledColor(D3DCOLOR color) { m_disabledColor = color; }

private:
    std::string m_text;
    ButtonCallback m_callback;
    void* m_callbackData;

    bool m_clicked;

    D3DCOLOR m_normalColor;
    D3DCOLOR m_hoverColor;
    D3DCOLOR m_pressedColor;
    D3DCOLOR m_disabledColor;

    // Рендеринг кнопки
    void RenderButton(IDirect3DDevice9* device);
    void RenderText(IDirect3DDevice9* device);
};

} // namespace UI
