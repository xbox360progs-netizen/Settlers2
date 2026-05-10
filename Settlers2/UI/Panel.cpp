#include "StdAfx.h"
#include "Panel.h"

namespace UI {

Panel::Panel()
    : Widget()
    , m_backgroundColor(0xFF303030)
    , m_borderColor(0xFF606060)
    , m_borderWidth(2.0f)
    , m_titleBarHeight(24.0f)
    , m_closable(false)
    , m_closed(false)
    , m_draggable(false)
    , m_dragging(false)
    , m_dragOffsetX(0.0f)
    , m_dragOffsetY(0.0f)
{
}

Panel::~Panel()
{
    Shutdown();
}

bool Panel::Initialize(IDirect3DDevice9* device)
{
    if (!Widget::Initialize(device))
        return false;

    return true;
}

void Panel::Shutdown()
{
    Widget::Shutdown();
}

void Panel::Update(float deltaTime)
{
    if (m_closed)
        return;

    Widget::Update(deltaTime);
}

void Panel::Render(IDirect3DDevice9* device)
{
    if (!m_visible || m_closed)
        return;

    RenderBackground(device);
    RenderBorder(device);
    RenderTitleBar(device);

    Widget::Render(device);
}

void Panel::SetBackgroundColor(D3DCOLOR color)
{
    m_backgroundColor = color;
}

void Panel::SetBorderColor(D3DCOLOR color)
{
    m_borderColor = color;
}

void Panel::SetBorderWidth(float width)
{
    m_borderWidth = width;
}

void Panel::SetTitle(const std::string& title)
{
    m_title = title;
}

void Panel::SetTitleBarHeight(float height)
{
    m_titleBarHeight = height;
}

void Panel::SetClosable(bool closable)
{
    m_closable = closable;
}

void Panel::SetClosed(bool closed)
{
    m_closed = closed;
}

void Panel::SetDraggable(bool draggable)
{
    m_draggable = draggable;
}

void Panel::OnMouseDown(int x, int y)
{
    if (!m_enabled || m_closed)
        return;

    // Проверяем клик на заголовок для перетаскивания
    float absX, absY;
    GetAbsolutePosition(absX, absY);

    if (m_draggable && y >= absY && y < absY + m_titleBarHeight)
    {
        m_dragging = true;
        m_dragOffsetX = x - absX;
        m_dragOffsetY = y - absY;
        return;
    }

    // Проверяем клик на кнопку закрытия
    if (m_closable)
    {
        float closeX = absX + m_width - m_titleBarHeight;
        float closeY = absY;
        if (x >= closeX && x < closeX + m_titleBarHeight && y >= closeY && y < closeY + m_titleBarHeight)
        {
            m_closed = true;
            return;
        }
    }

    Widget::OnMouseDown(x, y);
}

void Panel::OnMouseUp(int x, int y)
{
    if (!m_enabled || m_closed)
        return;

    m_dragging = false;
    Widget::OnMouseUp(x, y);
}

void Panel::OnMouseMove(int x, int y)
{
    if (!m_enabled || m_closed)
        return;

    if (m_dragging)
    {
        float newX = x - m_dragOffsetX;
        float newY = y - m_dragOffsetY;
        SetPosition(newX, newY);
    }

    Widget::OnMouseMove(x, y);
}

void Panel::RenderBackground(IDirect3DDevice9* device)
{
    // TODO: Реализовать рендеринг фона панели
    // Использовать SpriteBatcher для отрисовки прямоугольника
}

void Panel::RenderBorder(IDirect3DDevice9* device)
{
    // TODO: Реализовать рендеринг границы панели
    // Можно использовать линии или отдельные спрайты для каждой стороны
}

void Panel::RenderTitleBar(IDirect3DDevice9* device)
{
    if (m_title.empty())
        return;

    // TODO: Реализовать рендеринг заголовка
    // Рендерим прямоугольник заголовка и текст
}

} // namespace UI
