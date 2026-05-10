#include "StdAfx.h"
#include "Button.h"
#include <d3dx9.h>

namespace UI {

Button::Button()
    : Widget()
    , m_callback(NULL)
    , m_callbackData(NULL)
    , m_clicked(false)
    , m_normalColor(0xFF404040)
    , m_hoverColor(0xFF606060)
    , m_pressedColor(0xFF808080)
    , m_disabledColor(0xFF404040)
{
}

Button::~Button()
{
    Shutdown();
}

bool Button::Initialize(IDirect3DDevice9* device)
{
    if (!Widget::Initialize(device))
        return false;

    return true;
}

void Button::Shutdown()
{
    Widget::Shutdown();
}

void Button::Update(float deltaTime)
{
    Widget::Update(deltaTime);
}

void Button::Render(IDirect3DDevice9* device)
{
    if (!m_visible)
        return;

    RenderButton(device);
    RenderText(device);

    Widget::Render(device);
}

void Button::OnMouseDown(int x, int y)
{
    if (!m_enabled)
        return;

    Widget::OnMouseDown(x, y);

    if (ContainsPoint(x, y))
    {
        m_pressed = true;
    }
}

void Button::OnMouseUp(int x, int y)
{
    if (!m_enabled)
        return;

    Widget::OnMouseUp(x, y);

    if (m_pressed && ContainsPoint(x, y))
    {
        m_clicked = true;
        if (m_callback)
        {
            m_callback(m_callbackData);
        }
    }

    m_pressed = false;
}

void Button::OnMouseMove(int x, int y)
{
    if (!m_enabled)
        return;

    Widget::OnMouseMove(x, y);
}

void Button::SetText(const std::string& text)
{
    m_text = text;
}

void Button::SetCallback(ButtonCallback callback, void* userData)
{
    m_callback = callback;
    m_callbackData = userData;
}

void Button::RenderButton(IDirect3DDevice9* device)
{
    // Определяем цвет кнопки
    D3DCOLOR color = m_normalColor;
    if (!m_enabled)
        color = m_disabledColor;
    else if (m_pressed)
        color = m_pressedColor;
    else if (m_hovered)
        color = m_hoverColor;

    // Получаем абсолютную позицию
    float absX, absY;
    GetAbsolutePosition(absX, absY);

    // TODO: Реализовать рендеринг прямоугольника кнопки
    // Можно использовать спрайт или простой батчинг вершин
    // Для простоты пока заглушка
}

void Button::RenderText(IDirect3DDevice9* device)
{
    if (m_text.empty())
        return;

    // TODO: Реализовать рендеринг текста кнопки
    // Использовать TextRenderer из Renderer/2D/
}

} // namespace UI
