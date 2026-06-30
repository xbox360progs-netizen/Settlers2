#pragma once
#include "UiMessageId.h"
#include "UiAction.h"

namespace UI {

    static const int MAX_MENU_ITEMS = 48;
    static const int ICON_NONE = -1;

    struct MenuItem {
        UiMessageId labelId;
        UiAction    action;
        bool        enabled;
        bool        visible;
        int         iconId;

        MenuItem()
            : labelId(MSG_NONE)
            , enabled(false)
            , visible(false)
            , iconId(ICON_NONE)
        {
        }

        MenuItem(UiMessageId label, const UiAction& act,
                 bool en = true, bool vis = true, int icon = ICON_NONE)
            : labelId(label)
            , action(act)
            , enabled(en)
            , visible(vis)
            , iconId(icon)
        {
        }
    };

    class MenuModel {
    public:
        MenuModel();

        void SetItems(const MenuItem* items, int count);
        const MenuItem* GetItem(int index) const;
        int GetItemCount() const { return m_itemCount; }

        int GetSelected() const { return m_selected; }
        void SetSelected(int index);

        void SelectNext();
        void SelectPrevious();

        void Clear();

        // Returns the action of the selected item, or default (UI_CMD_NONE)
        UiAction GetSelectedAction() const;

    private:
        MenuItem m_items[MAX_MENU_ITEMS];
        int m_itemCount;
        int m_selected;
    };

} // namespace UI
