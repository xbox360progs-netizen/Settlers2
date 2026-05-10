#pragma once
#include "Widget.h"
#include "../World/Map.h"

namespace UI {

// Миникарта
// Отображает уменьшенную версию карты
class Minimap : public Widget
{
public:
    Minimap();
    virtual ~Minimap();

    // Инициализация
    virtual bool Initialize(IDirect3DDevice9* device);
    virtual void Shutdown();

    // Обновление
    virtual void Update(float deltaTime);

    // Рендеринг
    virtual void Render(IDirect3DDevice9* device);

    // Установить карту
    void SetMap(World::Map* map) { m_map = map; }
    World::Map* GetMap() const { return m_map; }

    // Позиция камеры на карте
    void SetCameraPosition(float x, float y);
    void GetCameraPosition(float& x, float& y) const { x = m_cameraX; y = m_cameraY; }

    // Масштаб миникарты
    void SetScale(float scale) { m_scale = scale; }
    float GetScale() const { return m_scale; }

    // Показывать/скрывать прямоугольник камеры
    void SetShowCameraRect(bool show);
    bool IsShowCameraRect() const { return m_showCameraRect; }

    // Цвета
    void SetBackgroundColor(D3DCOLOR color);
    void SetCameraRectColor(D3DCOLOR color);

    // Обработка ввода
    virtual void OnMouseDown(int x, int y);
    virtual void OnMouseUp(int x, int y);
    virtual void OnMouseMove(int x, int y);

    // Callback для клика на миникарте
    typedef void (*MinimapClickCallback)(float mapX, float mapY, void* userData);
    void SetClickCallback(MinimapClickCallback callback, void* userData = NULL);

private:
    World::Map* m_map;

    float m_cameraX;
    float m_cameraY;
    float m_scale;

    bool m_showCameraRect;
    D3DCOLOR m_backgroundColor;
    D3DCOLOR m_cameraRectColor;

    MinimapClickCallback m_clickCallback;
    void* m_callbackData;

    bool m_dragging;

    // Рендеринг
    void RenderMap(IDirect3DDevice9* device);
    void RenderCameraRect(IDirect3DDevice9* device);

    // Преобразование координат
    void ScreenToMap(int screenX, int screenY, float& mapX, float& mapY);
    void MapToScreen(float mapX, float mapY, int& screenX, int& screenY);
};

} // namespace UI
