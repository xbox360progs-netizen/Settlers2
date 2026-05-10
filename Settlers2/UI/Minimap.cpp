#include "StdAfx.h"
#include "Minimap.h"

namespace UI {

Minimap::Minimap()
    : Widget()
    , m_map(NULL)
    , m_cameraX(0.0f)
    , m_cameraY(0.0f)
    , m_scale(0.1f)
    , m_showCameraRect(true)
    , m_backgroundColor(0xFF000000)
    , m_cameraRectColor(0xFFFFFFFF)
    , m_clickCallback(NULL)
    , m_callbackData(NULL)
    , m_dragging(false)
{
}

Minimap::~Minimap()
{
    Shutdown();
}

bool Minimap::Initialize(IDirect3DDevice9* device)
{
    if (!Widget::Initialize(device))
        return false;

    return true;
}

void Minimap::Shutdown()
{
    Widget::Shutdown();
}

void Minimap::Update(float deltaTime)
{
    Widget::Update(deltaTime);
}

void Minimap::Render(IDirect3DDevice9* device)
{
    if (!m_visible)
        return;

    RenderMap(device);

    if (m_showCameraRect)
    {
        RenderCameraRect(device);
    }

    Widget::Render(device);
}

void Minimap::SetCameraPosition(float x, float y)
{
    m_cameraX = x;
    m_cameraY = y;
}

void Minimap::SetShowCameraRect(bool show)
{
    m_showCameraRect = show;
}

void Minimap::SetBackgroundColor(D3DCOLOR color)
{
    m_backgroundColor = color;
}

void Minimap::SetCameraRectColor(D3DCOLOR color)
{
    m_cameraRectColor = color;
}

void Minimap::OnMouseDown(int x, int y)
{
    if (!m_enabled || !m_map)
        return;

    Widget::OnMouseDown(x, y);

    if (ContainsPoint(x, y))
    {
        m_dragging = true;

        float mapX, mapY;
        ScreenToMap(x, y, mapX, mapY);

        if (m_clickCallback)
        {
            m_clickCallback(mapX, mapY, m_callbackData);
        }
    }
}

void Minimap::OnMouseUp(int x, int y)
{
    m_dragging = false;
    Widget::OnMouseUp(x, y);
}

void Minimap::OnMouseMove(int x, int y)
{
    if (!m_enabled || !m_map)
        return;

    Widget::OnMouseMove(x, y);

    if (m_dragging && ContainsPoint(x, y))
    {
        float mapX, mapY;
        ScreenToMap(x, y, mapX, mapY);

        if (m_clickCallback)
        {
            m_clickCallback(mapX, mapY, m_callbackData);
        }
    }
}

void Minimap::SetClickCallback(MinimapClickCallback callback, void* userData)
{
    m_clickCallback = callback;
    m_callbackData = userData;
}

void Minimap::RenderMap(IDirect3DDevice9* device)
{
    if (!m_map)
        return;

    // TODO: Реализовать рендеринг карты на миникарте
    // Нужно:
    // 1. Отрисовать фон
    // 2. Отрисовать тайлы карты в уменьшенном масштабе
    // 3. Использовать разные цвета для разных типов тайлов
}

void Minimap::RenderCameraRect(IDirect3DDevice9* device)
{
    if (!m_map)
        return;

    // TODO: Реализовать рендеринг прямоугольника камеры
    // Нужно:
    // 1. Преобразовать позицию камеры в координаты миникарты
    // 2. Отрисовать прямоугольник, показывающий область видимости
}

void Minimap::ScreenToMap(int screenX, int screenY, float& mapX, float& mapY)
{
    float absX, absY;
    GetAbsolutePosition(absX, absY);

    float relX = screenX - absX;
    float relY = screenY - absY;

    mapX = relX / m_scale;
    mapY = relY / m_scale;
}

void Minimap::MapToScreen(float mapX, float mapY, int& screenX, int& screenY)
{
    float absX, absY;
    GetAbsolutePosition(absX, absY);

    screenX = static_cast<int>(absX + mapX * m_scale);
    screenY = static_cast<int>(absY + mapY * m_scale);
}

} // namespace UI
