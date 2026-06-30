#include "stdafx.h"
#include "MenuModel.h"

namespace UI {

    MenuModel::MenuModel()
        : m_itemCount(0)
        , m_selected(0)
    {
        Clear();
    }

    void MenuModel::SetItems(const MenuItem* items, int count)
    {
        m_selected = 0;
        m_itemCount = (count > MAX_MENU_ITEMS) ? MAX_MENU_ITEMS : count;
        for (int i = 0; i < m_itemCount; ++i) {
            m_items[i] = items[i];
        }
    }

    const MenuItem* MenuModel::GetItem(int index) const
    {
        if (index < 0 || index >= m_itemCount)
            return NULL;
        return &m_items[index];
    }

    void MenuModel::SetSelected(int index)
    {
        if (index >= 0 && index < m_itemCount)
            m_selected = index;
    }

    void MenuModel::SelectNext()
    {
        if (m_itemCount <= 0) return;
        m_selected++;
        if (m_selected >= m_itemCount)
            m_selected = 0;
    }

    void MenuModel::SelectPrevious()
    {
        if (m_itemCount <= 0) return;
        m_selected--;
        if (m_selected < 0)
            m_selected = m_itemCount - 1;
    }

    void MenuModel::Clear()
    {
        for (int i = 0; i < MAX_MENU_ITEMS; ++i) {
            m_items[i] = MenuItem();
        }
        m_itemCount = 0;
        m_selected = 0;
    }

    UiAction MenuModel::GetSelectedAction() const
    {
        if (m_selected < 0 || m_selected >= m_itemCount)
            return UiAction();
        return m_items[m_selected].action;
    }

} // namespace UI
