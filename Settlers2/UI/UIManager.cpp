#include "StdAfx.h"
#include "UIManager.h"

namespace UI {

UIManager::UIManager()
    : m_device(NULL)
    , m_inputManager(NULL)
    , m_rootWidget(NULL)
    , m_focusedWidget(NULL)
    , m_screenWidth(1280)
    , m_screenHeight(720)
{
}

UIManager::~UIManager()
{
    Shutdown();
}

bool UIManager::Initialize(IDirect3DDevice9* device, Input::InputManager* inputManager)
{
    if (!device || !inputManager)
        return false;

    m_device = device;
    m_device->AddRef();

    m_inputManager = inputManager;

    return true;
}

void UIManager::Shutdown()
{
    // Удаляем все виджеты
    for (size_t i = 0; i < m_widgets.size(); i++)
    {
        if (m_widgets[i])
        {
            m_widgets[i]->Shutdown();
            delete m_widgets[i];
        }
    }
    m_widgets.clear();

    m_rootWidget = NULL;
    m_focusedWidget = NULL;

    if (m_device)
    {
        m_device->Release();
        m_device = NULL;
    }

    m_inputManager = NULL;
}

void UIManager::Update(float deltaTime)
{
    // Обновляем все виджеты
    for (size_t i = 0; i < m_widgets.size(); i++)
    {
        if (m_widgets[i] && m_widgets[i]->IsVisible())
        {
            m_widgets[i]->Update(deltaTime);
        }
    }

    // Обновляем корневой виджет
    if (m_rootWidget && m_rootWidget->IsVisible())
    {
        m_rootWidget->Update(deltaTime);
    }
}

void UIManager::Render(IDirect3DDevice9* device)
{
    // Рендерим все виджеты
    for (size_t i = 0; i < m_widgets.size(); i++)
    {
        if (m_widgets[i] && m_widgets[i]->IsVisible())
        {
            m_widgets[i]->Render(device);
        }
    }

    // Рендерим корневой виджет
    if (m_rootWidget && m_rootWidget->IsVisible())
    {
        m_rootWidget->Render(device);
    }
}

void UIManager::AddWidget(Widget* widget)
{
    if (widget)
    {
        m_widgets.push_back(widget);
    }
}

void UIManager::RemoveWidget(Widget* widget)
{
    for (size_t i = 0; i < m_widgets.size(); i++)
    {
        if (m_widgets[i] == widget)
        {
            m_widgets.erase(m_widgets.begin() + i);
            return;
        }
    }
}

Widget* UIManager::GetWidget(size_t index) const
{
    if (index < m_widgets.size())
        return m_widgets[index];
    return NULL;
}

void UIManager::OnMouseDown(int x, int y)
{
    // Сначала проверяем корневой виджет
    if (m_rootWidget && m_rootWidget->IsVisible() && m_rootWidget->ContainsPoint(x, y))
    {
        m_rootWidget->OnMouseDown(x, y);
        return;
    }

    // Проверяем остальные виджеты в обратном порядке (сверху вниз)
    for (int i = static_cast<int>(m_widgets.size()) - 1; i >= 0; i--)
    {
        if (m_widgets[i] && m_widgets[i]->IsVisible() && m_widgets[i]->ContainsPoint(x, y))
        {
            m_widgets[i]->OnMouseDown(x, y);
            m_focusedWidget = m_widgets[i];
            return;
        }
    }

    // Если ни один виджет не нажат, снимаем фокус
    m_focusedWidget = NULL;
}

void UIManager::OnMouseUp(int x, int y)
{
    if (m_focusedWidget)
    {
        m_focusedWidget->OnMouseUp(x, y);
    }
    else if (m_rootWidget)
    {
        m_rootWidget->OnMouseUp(x, y);
    }
}

void UIManager::OnMouseMove(int x, int y)
{
    // Передаем событие всем видимым виджетам
    if (m_rootWidget && m_rootWidget->IsVisible())
    {
        m_rootWidget->OnMouseMove(x, y);
    }

    for (size_t i = 0; i < m_widgets.size(); i++)
    {
        if (m_widgets[i] && m_widgets[i]->IsVisible())
        {
            m_widgets[i]->OnMouseMove(x, y);
        }
    }
}

void UIManager::OnKeyDown(int key)
{
    if (m_focusedWidget && m_focusedWidget->IsEnabled())
    {
        m_focusedWidget->OnKeyDown(key);
    }
}

void UIManager::OnKeyUp(int key)
{
    if (m_focusedWidget && m_focusedWidget->IsEnabled())
    {
        m_focusedWidget->OnKeyUp(key);
    }
}

void UIManager::SetScreenSize(int width, int height)
{
    m_screenWidth = width;
    m_screenHeight = height;
}

Widget* UIManager::FindWidgetAt(int x, int y) const
{
    // Проверяем корневой виджет
    if (m_rootWidget && m_rootWidget->IsVisible() && m_rootWidget->ContainsPoint(x, y))
    {
        return m_rootWidget;
    }

    // Проверяем остальные виджеты в обратном порядке
    for (int i = static_cast<int>(m_widgets.size()) - 1; i >= 0; i--)
    {
        if (m_widgets[i] && m_widgets[i]->IsVisible() && m_widgets[i]->ContainsPoint(x, y))
        {
            return m_widgets[i];
        }
    }

    return NULL;
}

} // namespace UI
