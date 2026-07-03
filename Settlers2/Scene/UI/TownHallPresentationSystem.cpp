#include "stdafx.h"
#include "TownHallPresentationSystem.h"
#include "../FrameContext.h"
#include "../../World/FlagManager.h"
#include "../../World/Flag.h"
#include "../../World/Components/Building.h"
#include "../../Logic/EconomyManager.h"
#include "../../Logic/CoordinateSystem.h"
#include "../BuildingPlacement.h"

namespace Scene {

void TownHallPresentationSystem::SetManagers(
    World::FlagManager* flagManager,
    Logic::EconomyManager* economyManager)
{
    m_flagManager = flagManager;
    m_economyManager = economyManager;
}

void TownHallPresentationSystem::BuildRenderFrame(
    const FrameContext& frame,
    RenderTownHallPanel& panel,
    std::vector<RenderBuildingHighlight>& highlights)
{
    highlights.clear();

    // ─── Cursor-on-town-hall highlight (single building) ──────────
    // Only when no menus are open.
    bool menusInactive = !frame.input.menuActive && !frame.input.roadMenuActive
        && !frame.input.flagMenuActive && !frame.input.geologistMenuActive
        && !frame.input.townHallPanelOpen;
    if (menusInactive && frame.input.cursorOnTownHall && m_flagManager) {
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        for (size_t fi = 0; fi < m_flagManager->GetCount(); ++fi) {
            World::Flag* flag = m_flagManager->GetFlag(fi);
            if (!flag || !flag->building) continue;
            // Find the town hall (b_townhall type).
            if (flag->building->type != World::Storehouse) continue;
            RenderBuildingHighlight h;
            h.buildingType = flag->building->type;
            coords.NodeTileToWorld(
                flag->building->pos.x, flag->building->pos.y,
                h.transform.worldX, h.transform.worldY);
            h.transform.depthLayer = static_cast<int>(0.99f * 65535.0f);
            highlights.push_back(h);
            break;
        }
    }

    // ─── Town hall panel ──────────────────────────────────────────
    if (!frame.input.townHallPanelOpen) {
        panel.visible = false;
        return;
    }

    panel.visible = true;
    panel.panelX = (1280.0f - frame.overlay.townHallPanelW) * 0.5f;
    panel.panelY = (720.0f - frame.overlay.townHallPanelH) * 0.5f;
    panel.panelW = frame.overlay.townHallPanelW;
    panel.panelH = frame.overlay.townHallPanelH;
    panel.panelU0 = frame.overlay.townHallPanelU0;
    panel.panelV0 = frame.overlay.townHallPanelV0;
    panel.panelU1 = frame.overlay.townHallPanelU1;
    panel.panelV1 = frame.overlay.townHallPanelV1;

    // Stock values from EconomyManager.
    if (m_economyManager) {
        panel.stockValues[0] = m_economyManager->GetTotalStock(World::ResourceType_Wood);
        panel.stockValues[1] = m_economyManager->GetTotalStock(World::ResourceType_Planks);
        panel.stockValues[2] = m_economyManager->GetTotalStock(World::ResourceType_Stone);
        panel.stockValues[3] = m_economyManager->GetTotalStock(World::ResourceType_Fish);
        panel.stockValues[4] = m_economyManager->GetTotalStock(World::ResourceType_Meat);
        panel.stockValues[5] = m_economyManager->GetTotalStock(World::ResourceType_Coal);
    }

    // ─── Building highlights (all buildings, semi-transparent) ────
    CoordinateSystem& coords = CoordinateSystem::GetInstance();
    if (m_flagManager) {
        for (size_t fi = 0; fi < m_flagManager->GetCount(); ++fi) {
            World::Flag* flag = m_flagManager->GetFlag(fi);
            if (!flag || !flag->building) continue;

            RenderBuildingHighlight h;
            h.buildingType = flag->building->type;
            h.isDepleted = (flag->building->IsDepleted() &&
                            flag->building->m_depletedSpriteIdx >= 0);
            coords.NodeTileToWorld(
                flag->building->pos.x, flag->building->pos.y,
                h.transform.worldX, h.transform.worldY);
            h.transform.depthLayer = static_cast<int>(0.99f * 65535.0f);
            highlights.push_back(h);
        }
    }
}

} // namespace Scene
