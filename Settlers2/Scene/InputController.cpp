#include "stdafx.h"
#include "InputController.h"
#include "../UI/GridMenu.h"
#include "../UI/UIMenu.h"
#include "../UI/StatusManager.h"
#include "../World/Map.h"
#include "../Graphics/TextureRegistry.h"
#include "../Graphics/SpriteAtlas.h"
#include "../Logic/CoordinateSystem.h"
#include "MenuBootstrap.h"
#include <cmath>

namespace Scene {

    InputController::InputController(
        Input::InputManager* inputManager,
        Core::CommandBus& commandBus,
        Core::EventBus* eventBus,
        World::Map* map,
        World::FlagManager* flagManager,
        PlacementController* placement,
        RoadController* roadController,
        GridMenu* buildMenu,
        UIMenu* flagMenu)
        : m_inputManager(inputManager)
        , m_commandBus(commandBus)
        , m_eventBus(eventBus)
        , m_map(map)
        , m_flagManager(flagManager)
        , m_placement(placement)
        , m_roadController(roadController)
        , m_buildMenu(buildMenu)
        , m_flagMenu(flagMenu)
        , m_host(NULL)
        , m_statusManager(NULL)
        , m_cursorTileX(0)
        , m_cursorTileY(0)
        , m_gamepadCursorCooldown(0.0f)
        , m_gamepadActive(false)
        , m_cursorOnTownHall(false)
        , m_menuActive(false)
        , m_roadMenuActive(false)
        , m_flagMenuActive(false)
        , m_geologistMenuActive(false)
        , m_townHallPanelOpen(false)
        , m_logisticsDebug(false)
    {
    }

    void InputController::Tick(float dt)
    {
        if (m_gamepadCursorCooldown > 0.0f)
            m_gamepadCursorCooldown -= dt;
    }

    void InputController::HandleGamepadInput()
    {
        if (!m_inputManager || !m_map) return;
        if (m_menuActive || m_roadMenuActive || m_flagMenuActive || m_geologistMenuActive || m_townHallPanelOpen) return;

        Input::Gamepad* pad = m_inputManager->GetGamepad();
        if (!pad || !pad->IsConnected()) return;

        int dx = 0, dy = 0;

        if (pad->IsButtonPressed(Input::GP_DPadUp))    { dy = -1; m_gamepadActive = true; }
        if (pad->IsButtonPressed(Input::GP_DPadDown))  { dy = 1;  m_gamepadActive = true; }
        if (pad->IsButtonPressed(Input::GP_DPadLeft))  { dx = -1; m_gamepadActive = true; }
        if (pad->IsButtonPressed(Input::GP_DPadRight)) { dx = 1;  m_gamepadActive = true; }

        if (dx == 0 && dy == 0 && m_gamepadCursorCooldown <= 0.0f) {
            float sx, sy;
            pad->GetLeftStick(sx, sy);
            if (fabsf(sx) > 0.5f || fabsf(sy) > 0.5f) {
                if (fabsf(sx) > 0.5f) dx = (sx > 0.0f) ? 1 : -1;
                if (fabsf(sy) > 0.5f) dy = (sy > 0.0f) ? 1 : -1;
                m_gamepadActive = true;
                m_gamepadCursorCooldown = 0.15f;
            }
        }

        if (dx != 0 || dy != 0) {
            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            int nodesW = coords.GetNodesWidth();
            int nodesH = coords.GetNodesHeight();

            m_gamepadCursor.x += dx;
            m_gamepadCursor.y += dy;

            if (m_gamepadCursor.x < 0) m_gamepadCursor.x = 0;
            if (m_gamepadCursor.x >= nodesW) m_gamepadCursor.x = nodesW - 1;
            if (m_gamepadCursor.y < 0) m_gamepadCursor.y = 0;
            if (m_gamepadCursor.y >= nodesH) m_gamepadCursor.y = nodesH - 1;

            m_cursorTileX = m_gamepadCursor.x;
            m_cursorTileY = m_gamepadCursor.y;
        }
    }

    void InputController::HandleInput()
    {
        if (!m_inputManager) return;
        Input::Gamepad* pad = m_inputManager->GetGamepad();
        if (!pad) return;

        bool rbPressed = pad->IsButtonPressed(Input::GP_RB);
        bool bPressed = pad->IsButtonPressed(Input::GP_B);
        bool aPressed = pad->IsButtonPressed(Input::GP_A);
        bool yPressed = pad->IsButtonPressed(Input::GP_Y);

        if (pad->IsButtonPressed(Input::GP_Back)) {
            ToggleLogisticsDebug();
            char dbg[64];
            _snprintf(dbg, sizeof(dbg), "[Input] Logistics debug %s\n", m_logisticsDebug ? "ON" : "OFF");
            OutputDebugStringA(dbg);
            if (m_statusManager) {
                UI::UiMessageId msgId = m_logisticsDebug ? UI::MSG_LOGISTICS_DEBUG_ON : UI::MSG_LOGISTICS_DEBUG_OFF;
                m_statusManager->SetStatus(msgId, UI::UiFormatArgs(), 2.0f);
            }
        }

        // ── Placement states ──────────────────────────────────
        if (m_placement->GetState() == PLACESTATE_PLACE_FLAG) {
            if (aPressed) {
                OutputDebugStringA("[Input] A pressed in PLACESTATE_PLACE_FLAG — calling HandlePlaceAtCursor\n");
                HandlePlaceAtCursor();
            } else if (bPressed) {
                m_placement->Cancel();
                if (m_statusManager) m_statusManager->SetStatus(UI::MSG_PLACEMENT_CANCELLED, UI::UiFormatArgs(), 2.0f);
                OutputDebugStringA("[Input] Flag placement cancelled\n");
            }
        } else if (m_placement->GetState() == PLACESTATE_PLACE_ROAD) {
            if (aPressed) {
                m_roadController->TryAddTile(m_cursorTileX, m_cursorTileY);
            } else if (bPressed) {
                if (m_statusManager) m_statusManager->SetStatus(UI::MSG_ROAD_CANCELLED, UI::UiFormatArgs(), 2.0f);
                m_roadController->Cancel();
                OutputDebugStringA("[Input] Road cancelled\n");
            }
        } else if (m_placement->GetState() == PLACESTATE_CONFIRM) {
            if (aPressed) {
                if (m_placement->GetConfirmAction() == PLACECONFIRM_PLACE_FLAG) {
                    HandleConfirmFreeFlag();
                } else if (m_placement->GetConfirmAction() == PLACECONFIRM_START_ROAD) {
                    m_roadController->Start(m_placement->GetConfirmTargetX(), m_placement->GetConfirmTargetY());
                } else if (m_placement->GetConfirmAction() == PLACECONFIRM_DELETE_FLAG) {
                    if (m_host) {
                        m_host->DeleteFlagAt(m_placement->GetConfirmTargetX(), m_placement->GetConfirmTargetY());
                    }
                    m_placement->CancelConfirm();
                }
            } else if (bPressed) {
                if (m_statusManager) m_statusManager->SetStatus(UI::MSG_CANCELLED, UI::UiFormatArgs(), 2.0f);
                m_placement->CancelConfirm();
            }

        // ── Menu states ───────────────────────────────────────
        } else if (m_menuActive) {
            if (m_buildMenu) {
                m_buildMenu->Update(pad, 1.0f / 60.0f);

                if (bPressed) {
                    m_menuActive = false;
                    m_buildMenu->Hide();
                }

                if (m_buildMenu->HasSelection()) {
                    int selIdx = m_buildMenu->GetSelectedAction().value;
                    if (selIdx >= 0) {
                        std::tr1::shared_ptr<SpriteAtlas> iconAtlas = TextureRegistry::instance().getAtlas("Icon");
                        if (iconAtlas) {
                            const SpriteRegion* reg = iconAtlas->GetRegion(selIdx);
                            if (reg) {
                                std::string iconName = reg->name;
                                TextureRegistry::instance().getTextureOrLoad("Buildings");
                                std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = TextureRegistry::instance().getAtlas("Buildings");
                                if (buildingsAtlas) {
                                    uint32_t cIdx = buildingsAtlas->GetIndex("construction");
                                    if (cIdx == 0xFFFFFFFF) cIdx = buildingsAtlas->GetIndex("Construction");
                                    if (cIdx == 0xFFFFFFFF) cIdx = buildingsAtlas->GetIndex("ConstructionSite");
                                    int constrIdx = (cIdx != 0xFFFFFFFF) ? (int)cIdx : -1;

                                    World::BuildingType bt = MenuBootstrap::GetBuildingTypeFromSpriteName(iconName);
                                    if (bt != World::Building_None) {
                                        {
                                            char dbg[256];
                                            _snprintf(dbg, sizeof(dbg), "[Input] Build menu: selected building type=%d icon=%s\n", (int)bt, iconName.c_str());
                                            OutputDebugStringA(dbg);
                                        }
                                        m_placement->EnterBuildMode(bt, selIdx, constrIdx, iconName);
                                    } else {
                                        char dbg[256];
                                        _snprintf(dbg, sizeof(dbg), "[Input] Build menu: GetBuildingTypeFromSpriteName failed for icon=%s\n", iconName.c_str());
                                        OutputDebugStringA(dbg);
                                    }
                                }
                            }
                        }
                    }
                    m_menuActive = false;
                    m_buildMenu->Hide();
                    m_buildMenu->ResetSelection();
                }
            }

        } else if (m_flagMenuActive) {
            if (m_flagMenu) {
                m_flagMenu->Update(pad, 1.0f / 60.0f);

                if (bPressed || !m_flagMenu->IsVisible()) {
                    m_flagMenuActive = false;
                    m_flagMenu->Hide();
                }

                if (m_flagMenu->HasSelection()) {
                    int selIdx = m_flagMenu->GetSelectedIndex();
                    if (selIdx >= 0 && selIdx < 3) {
                        if (selIdx == 0) {
                            HandleConfirmFreeFlag();
                        } else if (selIdx == 1) {
                            // Delete flag (or flag + building)
                            World::Flag* f = NULL;
                            if (m_flagManager) {
                                f = m_flagManager->GetFlagAt(m_cursorTileX, m_cursorTileY);
                                if (!f) {
                                    for (int dy = -1; dy <= 1; ++dy) {
                                        for (int dx = -1; dx <= 1; ++dx) {
                                            World::Flag* tf = m_flagManager->GetFlagAt(m_cursorTileX + dx, m_cursorTileY + dy);
                                            if (tf) { f = tf; break; }
                                        }
                                        if (f) break;
                                    }
                                }
                            }
                            if (f) {
                                if (f->type == World::FLAG_WAREHOUSE) {
                                    if (m_statusManager) m_statusManager->SetStatus(UI::MSG_CANNOT_DELETE_TOWN_HALL, UI::UiFormatArgs(), 2.0f);
                                } else if (f->building || f->pendingBuilding != World::Building_None) {
                                    // Needs host confirmation — set up confirm state
                                    m_placement->SetConfirm(PLACECONFIRM_DELETE_FLAG, f->pos.x, f->pos.y);
                                    if (m_statusManager) m_statusManager->SetStatus(UI::MSG_DELETE_FLAG_PROMPT, UI::UiFormatArgs(), 3.0f);
                                } else {
                                    // Simple flag deletion — pipeline handles road cleanup
                                    Core::DeleteFlagCmd dfd;
                                    dfd.flagId = f->id;
                                    m_commandBus.Post(Core::Cmd_DeleteFlag, dfd);
                                    if (m_statusManager) m_statusManager->SetStatus(UI::MSG_FLAG_REMOVED, UI::UiFormatArgs(), 2.0f);
                                }
                            } else {
                                if (m_statusManager) m_statusManager->SetStatus(UI::MSG_NO_FLAG_NEARBY, UI::UiFormatArgs(), 2.0f);
                            }
                        } else if (selIdx == 2) {
                            // Start road (place free flag first if none exists)
                            m_flagMenuActive = false;
                            m_flagMenu->Hide();
                            if (m_flagManager && !m_flagManager->GetFlagAt(m_cursorTileX, m_cursorTileY)) {
                                HandleConfirmFreeFlag();
                            }
                            m_roadController->Start(m_cursorTileX, m_cursorTileY);
                        }
                    }
                    m_flagMenuActive = false;
                    m_flagMenu->Hide();
                    m_flagMenu->ResetSelection();
                }
            }

        } else if (m_townHallPanelOpen) {
            if (bPressed) {
                m_townHallPanelOpen = false;
                if (m_statusManager) m_statusManager->ClearStatus();
            }

        // ── Default mode ──────────────────────────────────────
        } else {
            if (bPressed && m_geologistMenuActive) {
                if (m_host) m_host->CancelGeologist();
            }
            if (aPressed) {
                bool handled = false;
                if (m_map) {
                    const World::Tile& objTile = m_map->GetTile(World::Objects, m_cursorTileX, m_cursorTileY);
                    if (objTile.type == World::Mountain || objTile.type == World::MountainOnWater || objTile.type == World::Rock) {
                        if (m_host) m_host->OnMountainTileAction(m_cursorTileX, m_cursorTileY);
                        handled = true;
                    }
                }
                if (!handled && m_cursorOnTownHall) {
                    m_townHallPanelOpen = true;
                }
            }
            if (yPressed) {
                bool skipFlagMenu = false;
                if (m_map) {
                    const World::Tile& objTile = m_map->GetTile(World::Objects, m_cursorTileX, m_cursorTileY);
                    if (objTile.type == World::Mountain || objTile.type == World::MountainOnWater || objTile.type == World::Rock) {
                        skipFlagMenu = true;
                    }
                    if (!skipFlagMenu) {
                        BYTE weight = m_map->GetNodeWeight(m_cursorTileX, m_cursorTileY);
                        if (weight == World::Weight_Deep || weight == World::Weight_Shallow) {
                            skipFlagMenu = true;
                        }
                    }
                }
                if (!skipFlagMenu && m_flagManager) {
                    World::Flag* nearestFlag = NULL;
                    int nearestDist = 999;
                    int flagX = m_cursorTileX, flagY = m_cursorTileY;
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            World::Flag* f = m_flagManager->GetFlagAt(m_cursorTileX + dx, m_cursorTileY + dy);
                            if (f) {
                                int dist = abs(dx) + abs(dy);
                                if (dist < nearestDist) {
                                    nearestDist = dist;
                                    nearestFlag = f;
                                    flagX = m_cursorTileX + dx;
                                    flagY = m_cursorTileY + dy;
                                }
                            }
                        }
                    }
                    if (nearestFlag) {
                        m_placement->SetConfirmTarget(flagX, flagY);
                    } else {
                        m_placement->SetConfirmTarget(m_cursorTileX, m_cursorTileY);
                    }
                    m_flagMenuActive = true;
                    m_flagMenu->Show();
                }
            } else if (rbPressed && m_buildMenu) {
                m_menuActive = true;
                m_buildMenu->ResetSelection();
                m_buildMenu->Show(640.0f, 360.0f);
            }
        }
    }

    void InputController::HandlePlaceAtCursor()
    {
        OutputDebugStringA("[Input] HandlePlaceAtCursor — calling TryPlaceFlag\n");
        PlacementRequest req = m_placement->TryPlaceFlag(m_cursorTileX, m_cursorTileY);
        if (!req.valid) {
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[Input] PlaceFlag: cannot place at (%d,%d): %s\n",
                m_cursorTileX, m_cursorTileY, req.errorMsg ? req.errorMsg : "unknown");
            OutputDebugStringA(dbg);
            return;
        }

        Core::PlaceFlagCmd pfd;
        pfd.tileX = req.flagX;
        pfd.tileY = req.flagY;
        pfd.buildingType = req.type;
        pfd.isFreeFlag = false;
        pfd.buildX = req.buildX;
        pfd.buildY = req.buildY;
        pfd.autoConnectRoad = true;
        m_commandBus.Post(Core::Cmd_PlaceFlag, pfd);

        m_placement->Cancel();
        // Auto-start road building so player can connect the new flag
        if (req.type != World::Building_None && m_roadController) {
            m_roadController->Start(req.flagX, req.flagY);
        }
        if (m_statusManager) m_statusManager->SetStatus(UI::MSG_BUILDING_STARTED, UI::UiFormatArgs(), 2.0f);
        {
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[Input] PlaceFlag: placed type=%d at (%d,%d)\n",
                (int)req.type, req.buildX, req.buildY);
            OutputDebugStringA(dbg);
        }
    }

    void InputController::HandleConfirmFreeFlag()
    {
        if (!m_flagManager || !m_map) return;

        const World::Tile& objTile = m_map->GetTile(World::Objects, m_cursorTileX, m_cursorTileY);
        if (objTile.type != World::Tile_None) {
            if (m_statusManager) m_statusManager->SetStatus(UI::MSG_CANNOT_PLACE_FLAG_OBJECT, UI::UiFormatArgs(), 2.0f);
            return;
        }

        BYTE weight = m_map->GetNodeWeight(m_cursorTileX, m_cursorTileY);
        if (weight != World::Weight_Deep && weight != World::Weight_Shallow) {
            if (m_statusManager) m_statusManager->SetStatus(UI::MSG_FLAG_WATER_ONLY, UI::UiFormatArgs(), 2.0f);
            return;
        }

        m_placement->CancelConfirm();

        Core::PlaceFlagCmd pfd;
        pfd.tileX = m_cursorTileX;
        pfd.tileY = m_cursorTileY;
        pfd.buildingType = World::Building_None;
        pfd.isFreeFlag = true;
        pfd.buildX = 0;
        pfd.buildY = 0;
        pfd.autoConnectRoad = false;
        m_commandBus.Post(Core::Cmd_PlaceFlag, pfd);

        if (m_eventBus) {
            m_eventBus->Post(Core::Event_FlagTopologyChanged);
        }

        if (m_statusManager) m_statusManager->SetStatus(UI::MSG_FLAG_PLACED, UI::UiFormatArgs(), 2.0f);
        char dbg[256];
        _snprintf(dbg, sizeof(dbg), "[Input] Free flag placed at (%d,%d)\n", m_cursorTileX, m_cursorTileY);
        OutputDebugStringA(dbg);
    }

    void InputController::FillFrameContext(InputFrameState& out) const
    {
        out.cursorTileX        = m_cursorTileX;
        out.cursorTileY        = m_cursorTileY;
        out.gamepadActive      = m_gamepadActive;
        out.gamepadCursorX     = m_gamepadCursor.x;
        out.gamepadCursorY     = m_gamepadCursor.y;
        out.menuActive         = m_menuActive;
        out.roadMenuActive     = m_roadMenuActive;
        out.flagMenuActive     = m_flagMenuActive;
        out.geologistMenuActive = m_geologistMenuActive;
        out.townHallPanelOpen  = m_townHallPanelOpen;
        out.cursorOnTownHall   = m_cursorOnTownHall;
        out.logisticsDebug     = m_logisticsDebug;
    }

}
