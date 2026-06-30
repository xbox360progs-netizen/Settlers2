#pragma once
#include "UiMessageId.h"

namespace Scene {
    struct InputFrameState;
}

namespace UI {

    class LocalizationService;

    class StatusManager {
    public:
        StatusManager();

        void Update(float dt);
        void SetStatus(UiMessageId id, const UiFormatArgs& args = UiFormatArgs(), float duration = 2.0f);
        void ClearStatus();
        bool IsActive() const;

        void FillFrameContext(Scene::InputFrameState& out, const LocalizationService* loc) const;

    private:
        UiMessageId m_id;
        UiFormatArgs m_args;
        float m_timer;
        bool m_hasStatus;
    };

} // namespace UI