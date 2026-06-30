#include "stdafx.h"
#include "GeologistController.h"
#include "../UI/StatusManager.h"
#include "../UI/UIMenu.h"
#include "../UI/LocalizationService.h"
#include "../UI/NotificationManager.h"
#include "../World/Map.h"
#include <stdio.h>

namespace Scene {

    static UI::UiMessageId GetResourceFoundMsg(World::ResourceType type)
    {
        switch (type) {
            case World::ResourceType_Coal:     return UI::MSG_GEOLOGIST_COAL_FOUND;
            case World::ResourceType_IronOre:  return UI::MSG_GEOLOGIST_IRON_FOUND;
            case World::ResourceType_GoldOre:  return UI::MSG_GEOLOGIST_GOLD_FOUND;
            case World::ResourceType_Stone:    return UI::MSG_GEOLOGIST_STONE_FOUND;
            case World::ResourceType_Marble:   return UI::MSG_GEOLOGIST_MARBLE_FOUND;
            case World::ResourceType_Granite:  return UI::MSG_GEOLOGIST_GRANITE_FOUND;
            case World::ResourceType_BronzeOre:return UI::MSG_GEOLOGIST_BRONZE_FOUND;
            default:                           return UI::MSG_GEOLOGIST_UNKNOWN_FOUND;
        }
    }

    GeologistController::GeologistController()
        : m_state(OverlayFrameState::GEOLOGIST_NONE)
        , m_tileX(-1)
        , m_tileY(-1)
        , m_timer(0.0f)
        , m_map(NULL)
        , m_statusManager(NULL)
        , m_host(NULL)
        , m_geologistMenu(NULL)
        , m_loc(NULL)
        , m_notificationManager(NULL)
    {
    }

    GeologistController::~GeologistController()
    {
    }

    void GeologistController::Initialize(World::Map* map,
                                         UI::StatusManager* statusManager,
                                         IGeologistHost* host,
                                         UIMenu* geologistMenu,
                                         const UI::LocalizationService* loc,
                                         UI::NotificationManager* notificationManager)
    {
        m_map = map;
        m_statusManager = statusManager;
        m_host = host;
        m_geologistMenu = geologistMenu;
        m_loc = loc;
        m_notificationManager = notificationManager;
    }

    void GeologistController::Update(float dt)
    {
        if (m_state == OverlayFrameState::GEOLOGIST_WORKING) {
            m_timer -= dt;
            if (m_timer <= 0.0f) {
                m_timer = 0.0f;
                if (m_map && m_tileX >= 0 && m_tileY >= 0) {
                    World::ResourceNode& node = m_map->GetResourceNode(m_tileX, m_tileY);
                    node.surveyed = true;

                    bool hasResource = (node.type != World::ResourceType_None && node.amount > 0);

                    if (m_notificationManager) {
                        if (hasResource) {
                            m_notificationManager->Notify(
                                UI::MSG_GEOLOGIST_REPORT,
                                GetResourceFoundMsg(node.type),
                                UI::UiFormatArgs(node.amount),
                                6.0f);
                        } else {
                            m_notificationManager->Notify(
                                UI::MSG_GEOLOGIST_REPORT,
                                UI::MSG_GEOLOGIST_BARREN_FOUND,
                                5.0f);
                        }
                    }

                    if (m_statusManager) {
                        if (hasResource) {
                            m_statusManager->SetStatus(GetResourceFoundMsg(node.type), UI::UiFormatArgs(node.amount), 5.0f);
                        } else {
                            m_statusManager->SetStatus(UI::MSG_GEOLOGIST_BARREN_FOUND, UI::UiFormatArgs(), 5.0f);
                        }
                    }
                    if (m_host) m_host->SetGeologistMenuActive(false);
                }
                m_state = OverlayFrameState::GEOLOGIST_NONE;
            } else if (m_statusManager) {
                UI::UiFormatArgs sec((int)(m_timer + 0.5f));
                m_statusManager->SetStatus(UI::MSG_GEOLOGIST_WORKING_SEC, sec, 0.0f);
            }
        }
    }

    void GeologistController::FillFrameContext(OverlayFrameState& overlay) const
    {
        overlay.geologistState = m_state;
        overlay.geologistTileX = m_tileX;
        overlay.geologistTileY = m_tileY;
    }

    void GeologistController::OnTileAction(int tileX, int tileY)
    {
        if (m_state == OverlayFrameState::GEOLOGIST_CONFIRM && m_tileX == tileX && m_tileY == tileY) {
            m_state = OverlayFrameState::GEOLOGIST_WORKING;
            m_timer = 60.0f;
            if (m_host) m_host->SetGeologistMenuActive(false);
            if (m_statusManager) m_statusManager->SetStatus(UI::MSG_GEOLOGIST_WORKING, UI::UiFormatArgs(), 0.0f);
            if (m_geologistMenu) m_geologistMenu->Hide();
        } else if (m_state == OverlayFrameState::GEOLOGIST_NONE) {
            m_state = OverlayFrameState::GEOLOGIST_CONFIRM;
            m_tileX = tileX;
            m_tileY = tileY;
            if (m_host) m_host->SetGeologistMenuActive(true);
            if (m_statusManager) m_statusManager->SetStatus(UI::MSG_GEOLOGIST_CONFIRM, UI::UiFormatArgs(), 0.0f);
            if (m_geologistMenu) m_geologistMenu->Show();
        } else if (m_statusManager) {
            m_statusManager->SetStatus(UI::MSG_GEOLOGIST_ALREADY, UI::UiFormatArgs(), 1.5f);
        }
    }

    void GeologistController::Cancel()
    {
        m_state = OverlayFrameState::GEOLOGIST_NONE;
        m_tileX = -1;
        m_tileY = -1;
        if (m_host) m_host->SetGeologistMenuActive(false);
        if (m_statusManager) m_statusManager->SetStatus(UI::MSG_GEOLOGIST_CANCELLED, UI::UiFormatArgs(), 2.0f);
        if (m_geologistMenu) m_geologistMenu->Hide();
    }

} // namespace Scene
