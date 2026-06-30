#pragma once
#include "UiMessageId.h"

namespace Scene { struct UiFrameState; }

namespace UI {

    class LocalizationService;

    class NotificationManager {
    public:
        explicit NotificationManager(const LocalizationService* loc);
        ~NotificationManager();

        void Update(float dt);

        // Single message — shown as title with no line1
        void Notify(UiMessageId textId, float duration = 5.0f);

        // Two-line notification with optional format args for line1
        void Notify(UiMessageId titleId, UiMessageId line1Id, float duration = 5.0f);
        void Notify(UiMessageId titleId, UiMessageId line1Id, const UiFormatArgs& args, float duration = 5.0f);

        // Three-line notification with entity name + description
        void Notify(UiMessageId titleId, UiMessageId nameId, UiMessageId descId, float duration = 5.0f);

        // Notification with tile position
        void NotifyAt(int tx, int ty, UiMessageId titleId, UiMessageId line1Id, float duration = 5.0f);
        void NotifyAt(int tx, int ty, UiMessageId titleId, UiMessageId nameId, UiMessageId descId, float duration = 5.0f);

        void FillFrameContext(Scene::UiFrameState& state) const;

        int GetCount() const { return m_count; }

    private:
        static const int MAX_SLOTS = 4;

        struct Slot {
            UiMessageId titleId;
            UiMessageId line1Id;
            UiMessageId descId;      // description shown as line2 (MSG_NONE = no description)
            UiFormatArgs args;
            int count;               // dedup accumulation (0 = no dedup)
            int tileX;
            int tileY;
            float timer;
            bool active;
        };

        Slot m_slots[MAX_SLOTS];
        int m_count;
        const LocalizationService* m_loc;

        int FindFreeSlot();
        void RemoveSlot(int index);
        bool TryCoalesce(UiMessageId titleId, UiMessageId line1Id, float duration);
        bool TryCoalesce3(UiMessageId titleId, UiMessageId nameId, UiMessageId descId, float duration);
    };

} // namespace UI
