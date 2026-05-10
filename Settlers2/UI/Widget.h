#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>

namespace UI {

// Базовый виджет
class Widget
{
public:
    Widget();
    virtual ~Widget();

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
    virtual void OnKeyDown(int key);
    virtual void OnKeyUp(int key);

    // Позиция и размер
    void SetPosition(float x, float y);
    void SetSize(float width, float height);
    void GetPosition(float& x, float& y) const { x = m_x; y = m_y; }
    void GetSize(float& width, float& height) const { width = m_width; height = m_height; }

    // Видимость
    void SetVisible(bool visible) { m_visible = visible; }
    bool IsVisible() const { return m_visible; }

    // Активность
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    // Родительский виджет
    void SetParent(Widget* parent) { m_parent = parent; }
    Widget* GetParent() const { return m_parent; }

    // Дочерние виджеты
    void AddChild(Widget* child);
    void RemoveChild(Widget* child);
    Widget* GetChild(size_t index) const;
    size_t GetChildCount() const { return m_children.size(); }

    // Проверка попадания точки
    bool ContainsPoint(float x, float y) const;

    // Получить абсолютную позицию
    void GetAbsolutePosition(float& x, float& y) const;

protected:
    IDirect3DDevice9* m_device;

    float m_x;
    float m_y;
    float m_width;
    float m_height;

    bool m_visible;
    bool m_enabled;

    Widget* m_parent;
    std::vector<Widget*> m_children;

    // Цвета
    D3DCOLOR m_color;
    D3DCOLOR m_hoverColor;
    D3DCOLOR m_disabledColor;

    // Состояние
    bool m_hovered;
    bool m_pressed;
};

} // namespace UI
