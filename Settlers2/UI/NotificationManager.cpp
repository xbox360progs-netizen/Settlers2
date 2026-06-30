#include "stdafx.h"
#include "NotificationManager.h"
#include "LocalizationService.h"
#include "../Scene/FrameContext.h"
#include <string.h>
#include <stdio.h>

namespace UI {

    NotificationManager::NotificationManager(const LocalizationService* loc)
        : m_count(0)
        , m_loc(loc)
    {
        for (int i = 0; i < MAX_SLOTS; ++i) {
            m_slots[i].active = false;
            m_slots[i].timer = 0.0f;
            m_slots[i].tileX = -1;
            m_slots[i].tileY = -1;
            m_slots[i].descId = MSG_NONE;
            m_slots[i].count = 0;
        }
    }

    NotificationManager::~NotificationManager()
    {
    }

    int NotificationManager::FindFreeSlot()
    {
        for (int i = 0; i < MAX_SLOTS; ++i) {
            if (!m_slots[i].active)
                return i;
        }
        RemoveSlot(0);
        return m_count;
    }

    void NotificationManager::RemoveSlot(int index)
    {
        if (index < 0 || index >= m_count) return;
        for (int i = index; i < m_count - 1; ++i)
            m_slots[i] = m_slots[i + 1];
        m_count--;
    }

    void NotificationManager::Update(float dt)
    {
        for (int i = m_count - 1; i >= 0; --i) {
            if (m_slots[i].active) {
                m_slots[i].timer -= dt;
                if (m_slots[i].timer <= 0.0f) {
                    RemoveSlot(i);
                }
            }
        }
    }

    // ─── Notify overloads (2-field) ─────────────────────────────────

    void NotificationManager::Notify(UiMessageId textId, float duration)
    {
        if (textId == MSG_NONE) return;
        if (TryCoalesce(textId, MSG_NONE, duration))
            return;

        int slot = FindFreeSlot();
        if (slot < 0 || slot >= MAX_SLOTS) return;

        Slot& s = m_slots[slot];
        s.titleId = textId;
        s.line1Id = MSG_NONE;
        s.descId = MSG_NONE;
        s.args = UiFormatArgs();
        s.count = 0;
        s.tileX = -1;
        s.tileY = -1;
        s.timer = duration;
        s.active = true;

        if (slot >= m_count)
            m_count = slot + 1;
    }

    void NotificationManager::Notify(UiMessageId titleId, UiMessageId line1Id, float duration)
    {
        if (titleId == MSG_NONE) return;
        if (TryCoalesce(titleId, line1Id, duration))
            return;

        int slot = FindFreeSlot();
        if (slot < 0 || slot >= MAX_SLOTS) return;

        Slot& s = m_slots[slot];
        s.titleId = titleId;
        s.line1Id = line1Id;
        s.descId = MSG_NONE;
        s.args = UiFormatArgs();
        s.count = 0;
        s.tileX = -1;
        s.tileY = -1;
        s.timer = duration;
        s.active = true;

        if (slot >= m_count)
            m_count = slot + 1;
    }

    void NotificationManager::Notify(UiMessageId titleId, UiMessageId line1Id, const UiFormatArgs& args, float duration)
    {
        if (titleId == MSG_NONE) return;
        if (TryCoalesce(titleId, line1Id, duration))
            return;

        int slot = FindFreeSlot();
        if (slot < 0 || slot >= MAX_SLOTS) return;

        Slot& s = m_slots[slot];
        s.titleId = titleId;
        s.line1Id = line1Id;
        s.descId = MSG_NONE;
        s.args = args;
        s.count = 0;
        s.tileX = -1;
        s.tileY = -1;
        s.timer = duration;
        s.active = true;

        if (slot >= m_count)
            m_count = slot + 1;
    }

    // ─── New Notify overload (3-field: entity name + description) ──

    void NotificationManager::Notify(UiMessageId titleId, UiMessageId nameId, UiMessageId descId, float duration)
    {
        if (titleId == MSG_NONE) return;
        if (TryCoalesce3(titleId, nameId, descId, duration))
            return;

        int slot = FindFreeSlot();
        if (slot < 0 || slot >= MAX_SLOTS) return;

        Slot& s = m_slots[slot];
        s.titleId = titleId;
        s.line1Id = nameId;
        s.descId = descId;
        s.args = UiFormatArgs();
        s.count = 0;
        s.tileX = -1;
        s.tileY = -1;
        s.timer = duration;
        s.active = true;

        if (slot >= m_count)
            m_count = slot + 1;
    }

    // ─── NotifyAt overloads ────────────────────────────────────────

    void NotificationManager::NotifyAt(int tx, int ty, UiMessageId titleId, UiMessageId line1Id, float duration)
    {
        if (titleId == MSG_NONE) return;
        if (TryCoalesce(titleId, line1Id, duration))
            return;

        int slot = FindFreeSlot();
        if (slot < 0 || slot >= MAX_SLOTS) return;

        Slot& s = m_slots[slot];
        s.titleId = titleId;
        s.line1Id = line1Id;
        s.descId = MSG_NONE;
        s.args = UiFormatArgs();
        s.count = 0;
        s.tileX = tx;
        s.tileY = ty;
        s.timer = duration;
        s.active = true;

        if (slot >= m_count)
            m_count = slot + 1;
    }

    void NotificationManager::NotifyAt(int tx, int ty, UiMessageId titleId, UiMessageId nameId, UiMessageId descId, float duration)
    {
        if (titleId == MSG_NONE) return;
        if (TryCoalesce3(titleId, nameId, descId, duration))
            return;

        int slot = FindFreeSlot();
        if (slot < 0 || slot >= MAX_SLOTS) return;

        Slot& s = m_slots[slot];
        s.titleId = titleId;
        s.line1Id = nameId;
        s.descId = descId;
        s.args = UiFormatArgs();
        s.count = 0;
        s.tileX = tx;
        s.tileY = ty;
        s.timer = duration;
        s.active = true;

        if (slot >= m_count)
            m_count = slot + 1;
    }

    // ─── Dedup ─────────────────────────────────────────────────────

    bool NotificationManager::TryCoalesce(UiMessageId titleId, UiMessageId line1Id, float duration)
    {
        for (int i = 0; i < m_count; ++i) {
            Slot& s = m_slots[i];
            if (!s.active) continue;
            if (s.titleId == titleId && s.line1Id == line1Id && s.descId == MSG_NONE) {
                s.timer = duration;
                s.count++;
                return true;
            }
        }
        return false;
    }

    bool NotificationManager::TryCoalesce3(UiMessageId titleId, UiMessageId nameId, UiMessageId descId, float duration)
    {
        for (int i = 0; i < m_count; ++i) {
            Slot& s = m_slots[i];
            if (!s.active) continue;
            if (s.titleId == titleId && s.line1Id == nameId && s.descId == descId) {
                s.timer = duration;
                s.count++;
                return true;
            }
        }
        return false;
    }

    // ─── FillFrameContext ──────────────────────────────────────────

    void NotificationManager::FillFrameContext(Scene::UiFrameState& state) const
    {
        state.notificationCount = 0;
        for (int i = 0; i < m_count && state.notificationCount < Scene::UiFrameState::MAX_NOTIFICATIONS; ++i) {
            const Slot& s = m_slots[i];
            if (!s.active) continue;

            Scene::UiFrameState::UiNotification& dst = state.notifications[state.notificationCount];

            if (s.titleId != MSG_NONE && s.line1Id != MSG_NONE && s.descId != MSG_NONE) {
                // Three-line format: title + entity name + description
                if (m_loc) {
                    const char* t = m_loc->Get(s.titleId);
                    strncpy_s(dst.title, sizeof(dst.title), t ? t : "", _TRUNCATE);
                    const char* n = m_loc->Get(s.line1Id);
                    strncpy_s(dst.line1, sizeof(dst.line1), n ? n : "", _TRUNCATE);
                } else {
                    dst.title[0] = '\0';
                    dst.line1[0] = '\0';
                }

                if (s.count >= 2) {
                    _snprintf(dst.line2, sizeof(dst.line2),
                        s.count >= 99 ? "x99+" : "x%d", s.count);
                } else if (m_loc) {
                    const char* d = m_loc->Get(s.descId);
                    strncpy_s(dst.line2, sizeof(dst.line2), d ? d : "", _TRUNCATE);
                } else {
                    dst.line2[0] = '\0';
                }
            } else if (s.titleId != MSG_NONE && s.line1Id != MSG_NONE) {
                // Two-line format: title + formatted line1
                if (m_loc) {
                    const char* t = m_loc->Get(s.titleId);
                    strncpy_s(dst.title, sizeof(dst.title), t ? t : "", _TRUNCATE);
                    m_loc->Format(s.line1Id, s.args, dst.line1, (int)sizeof(dst.line1));
                } else {
                    dst.title[0] = '\0';
                    dst.line1[0] = '\0';
                }

                if (s.count >= 2) {
                    _snprintf(dst.line2, sizeof(dst.line2),
                        s.count >= 99 ? "x99+" : "x%d", s.count);
                } else {
                    dst.line2[0] = '\0';
                }
            } else if (s.titleId != MSG_NONE && s.args.values[0] != 0) {
                // Single ID with args — format as line1, clear title
                if (m_loc) {
                    m_loc->Format(s.titleId, s.args, dst.line1, (int)sizeof(dst.line1));
                } else {
                    dst.line1[0] = '\0';
                }
                dst.title[0] = '\0';
                dst.line2[0] = '\0';
            } else if (s.titleId != MSG_NONE) {
                // Single ID, no args — title only
                if (m_loc) {
                    const char* t = m_loc->Get(s.titleId);
                    strncpy_s(dst.title, sizeof(dst.title), t ? t : "", _TRUNCATE);
                } else {
                    dst.title[0] = '\0';
                }
                dst.line1[0] = '\0';
                dst.line2[0] = '\0';
            } else {
                dst.title[0] = '\0';
                dst.line1[0] = '\0';
                dst.line2[0] = '\0';
            }

            dst.tileX = s.tileX;
            dst.tileY = s.tileY;
            dst.isActive = true;

            state.notificationCount++;
        }
    }

} // namespace UI
