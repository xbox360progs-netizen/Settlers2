#include "stdafx.h"
#include "StatusManager.h"
#include "LocalizationService.h"
#include "../Scene/FrameContext.h"
#include <string.h>
#include <stdio.h>

namespace UI {

    StatusManager::StatusManager()
        : m_id(MSG_NONE)
        , m_timer(0.0f)
        , m_hasStatus(false)
    {
    }

    void StatusManager::Update(float dt)
    {
        if (!m_hasStatus) return;
        if (m_timer > 0.0f) {
            m_timer -= dt;
            if (m_timer <= 0.0f) {
                ClearStatus();
            }
        }
    }

    void StatusManager::SetStatus(UiMessageId id, const UiFormatArgs& args, float duration)
    {
        m_id = id;
        m_args = args;
        m_timer = duration;
        m_hasStatus = (id != MSG_NONE);
    }

    void StatusManager::ClearStatus()
    {
        m_id = MSG_NONE;
        m_timer = 0.0f;
        m_hasStatus = false;
    }

    bool StatusManager::IsActive() const
    {
        return m_hasStatus;
    }

    void StatusManager::FillFrameContext(Scene::InputFrameState& out, const LocalizationService* loc) const
    {
        if (!m_hasStatus || !loc) {
            out.statusText[0] = '\0';
            return;
        }
        loc->Format(m_id, m_args, out.statusText, (int)sizeof(out.statusText));
    }

} // namespace UI