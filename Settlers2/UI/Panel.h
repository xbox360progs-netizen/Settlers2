#pragma once
#include "Widget.h"
#include <string>

namespace UI {

// Панель - контейнер для других виджетов
class Panel : public Widget
{
public:
    Panel();
    virtual ~Panel();

    // Инициализация
    virtual bool Initialize(IDirect3DDevice9* device);
    virtual void Shutdown();

    // Обновление
    virtual void Update(float deltaTime);

    // Рендеринг
    virtual void Render(IDirect3DDevice9* device);

    // Стиль панели
    void SetBackgroundColor(D3DCOLOR color);
    void SetBorderColor(D3DCOLOR color);
    void SetBorderWidth(float width);

    // Заголовок
    void SetTitle(const std::string& title);
    const std::string& GetTitle() const { return m_title; }
    void SetTitleBarHeight(float height);

    // Закрытие
    void SetClosable(bool closable);
    bool IsClosable() const { return m_closable; }
    bool IsClosed() const { return m_closed; }
    void SetClosed(bool closed);

    // Перетаскивание
    void SetDraggable(bool draggable);
    bool IsDraggable() const { return m_draggable; }

    // Обработка ввода
    virtual void OnMouseDown(int x, int y);
    virtual void OnMouseUp(int x, int y);
    virtual void OnMouseMove(int x, int y);

private:
    D3DCOLOR m_backgroundColor;
    D3DCOLOR m_borderColor;
    float m_borderWidth;

    std::string m_title;
    float m_titleBarHeight;

    bool m_closable;
    bool m_closed;
    bool m_draggable;

    bool m_dragging;
    float m_dragOffsetX;
    float m_dragOffsetY;

    // Рендеринг
    void RenderBackground(IDirect3DDevice9* device);
    void RenderBorder(IDirect3DDevice9* device);
    void RenderTitleBar(IDirect3DDevice9* device);
};

} // namespace UI
