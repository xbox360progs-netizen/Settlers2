#include "StdAfx.h"
#include "Widget.h"

namespace UI {

Widget::Widget()
    : m_device(NULL)
    , m_x(0.0f)
    , m_y(0.0f)
    , m_width(100.0f)
    , m_height(100.0f)
    , m_visible(true)
    , m_enabled(true)
    , m_parent(NULL)
    , m_color(0xFFFFFFFF)
    , m_hoverColor(0xFFFFFF00)
    , m_disabledColor(0xFF808080)
    , m_hovered(false)
    , m_pressed(false)
{
}

Widget::~Widget()
{
    Shutdown();
}

bool Widget::Initialize(IDirect3DDevice9* device)
{
    if (!device)
        return false;

    m_device = device;
    m_device->AddRef();

    return true;
}

void Widget::Shutdown()
{
    // Удаляем дочерние виджеты
    for (size_t i = 0; i < m_children.size(); i++)
    {
        if (m_children[i])
        {
            m_children[i]->Shutdown();
            delete m_children[i];
        }
    }
    m_children.clear();

    if (m_device)
    {
        m_device->Release();
        m_device = NULL;
    }
}

void Widget::Update(float deltaTime)
{
    // Обновляем дочерние виджеты
    for (size_t i = 0; i < m_children.size(); i++)
    {
        if (m_children[i] && m_children[i]->IsVisible())
        {
            m_children[i]->Update(deltaTime);
        }
    }
}

void Widget::Render(IDirect3DDevice9* device)
{
    if (!m_visible)
        return;

    // Рендерим дочерние виджеты
    for (size_t i = 0; i < m_children.size(); i++)
    {
        if (m_children[i] && m_children[i]->IsVisible())
        {
            m_children[i]->Render(device);
        }
    }
}

void Widget::OnMouseDown(int x, int y)
{
    if (!m_enabled)
        return;

    // Передаем событие дочерним виджетам
    for (size_t i = 0; i < m_children.size(); i++)
    {
        if (m_children[i] && m_children[i]->IsVisible() && m_children[i]->ContainsPoint(x, y))
        {
            m_children[i]->OnMouseDown(x, y);
            return;
        }
    }

    m_pressed = true;
}

void Widget::OnMouseUp(int x, int y)
{
    if (!m_enabled)
        return;

    // Передаем событие дочерним виджетам
    for (size_t i = 0; i < m_children.size(); i++)
    {
        if (m_children[i] && m_children[i]->IsVisible() && m_children[i]->ContainsPoint(x, y))
        {
            m_children[i]->OnMouseUp(x, y);
            return;
        }
    }

    m_pressed = false;
}

void Widget::OnMouseMove(int x, int y)
{
    if (!m_enabled)
        return;

    // Проверяем наведение
    m_hovered = ContainsPoint(x, y);

    // Передаем событие дочерним виджетам
    for (size_t i = 0; i < m_children.size(); i++)
    {
        if (m_children[i] && m_children[i]->IsVisible())
        {
            m_children[i]->OnMouseMove(x, y);
        }
    }
}

void Widget::OnKeyDown(int key)
{
    if (!m_enabled)
        return;

    // Передаем событие дочерним виджетам
    for (size_t i = 0; i < m_children.size(); i++)
    {
        if (m_children[i] && m_children[i]->IsVisible())
        {
            m_children[i]->OnKeyDown(key);
        }
    }
}

void Widget::OnKeyUp(int key)
{
    if (!m_enabled)
        return;

    // Передаем событие дочерним виджетам
    for (size_t i = 0; i < m_children.size(); i++)
    {
        if (m_children[i] && m_children[i]->IsVisible())
        {
            m_children[i]->OnKeyUp(key);
        }
    }
}

void Widget::SetPosition(float x, float y)
{
    m_x = x;
    m_y = y;
}

void Widget::SetSize(float width, float height)
{
    m_width = width;
    m_height = height;
}

void Widget::AddChild(Widget* child)
{
    if (child)
    {
        child->SetParent(this);
        m_children.push_back(child);
    }
}

void Widget::RemoveChild(Widget* child)
{
    for (size_t i = 0; i < m_children.size(); i++)
    {
        if (m_children[i] == child)
        {
            m_children.erase(m_children.begin() + i);
            child->SetParent(NULL);
            return;
        }
    }
}

Widget* Widget::GetChild(size_t index) const
{
    if (index < m_children.size())
        return m_children[index];
    return NULL;
}

bool Widget::ContainsPoint(float x, float y) const
{
    float absX, absY;
    GetAbsolutePosition(absX, absY);

    return x >= absX && x < absX + m_width && y >= absY && y < absY + m_height;
}

void Widget::GetAbsolutePosition(float& x, float& y) const
{
    x = m_x;
    y = m_y;

    Widget* parent = m_parent;
    while (parent)
    {
        float px, py;
        parent->GetPosition(px, py);
        x += px;
        y += py;
        parent = parent->GetParent();
    }
}

} // namespace UI
