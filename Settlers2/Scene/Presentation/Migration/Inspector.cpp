#include "stdafx.h"
#include "Inspector.h"
#include "../../UI/RenderDebugLabel.h"
#include "IBuildingSource.h"
#include "IFlagSource.h"
#include "../../../Logic/CoordinateSystem.h"
#include <cstdio>

namespace Scene {

void Inspector::SetSources(
    IFlagSource* flagSource,
    IBuildingSource* buildingSource,
    IConstructionSiteSource* constructionSiteSource)
{
    m_flagSource = flagSource;
    m_buildingSource = buildingSource;
    m_constructionSiteSource = constructionSiteSource;
}

void Inspector::OnClick(float worldX, float worldY)
{
    Clear();

    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    // Convert click to node coords for spatial lookup
    int nx, ny;
    coords.WorldToNodeTile(worldX, worldY, nx, ny);

    // 1. Check flags (exact node match)
    if (m_flagSource) {
        uint32_t count = m_flagSource->GetFlagCount();
        for (uint32_t i = 0; i < count; ++i) {
            FlagView fv;
            if (!m_flagSource->GetFlag(i, fv)) continue;
            if (fv.nodeX == nx && fv.nodeY == ny) {
                m_selection.type = InspectorSelection::Flag;
                m_selection.buildingId = i + 1;
                m_selection.kind = 0;
                return;
            }
        }
    }

    // 2. Check completed buildings
    if (m_buildingSource) {
        uint32_t count = m_buildingSource->GetBuildingCount();
        for (uint32_t i = 0; i < count; ++i) {
            BuildingView bv;
            if (!m_buildingSource->GetBuilding(i, bv)) continue;
            if (bv.flagX == nx && bv.flagY == ny) {
                m_selection.type = InspectorSelection::Building;
                m_selection.buildingId = bv.buildingId;
                m_selection.buildingType = bv.buildingType;
                m_selection.fsmState = bv.fsmState;
                m_selection.hasWorker = bv.hasWorker;
                m_selection.depleted = bv.depleted;
                m_selection.kind = 1;
                return;
            }
        }
    }

    // 3. Check construction sites
    if (m_constructionSiteSource) {
        uint32_t count = m_constructionSiteSource->GetConstructionSiteCount();
        for (uint32_t i = 0; i < count; ++i) {
            BuildingView bv;
            if (!m_constructionSiteSource->GetConstructionSite(i, bv)) continue;
            if (bv.flagX == nx && bv.flagY == ny) {
                m_selection.type = InspectorSelection::ConstructionSite;
                m_selection.buildingId = bv.buildingId;
                m_selection.buildingType = bv.buildingType;
                m_selection.hasWorker = bv.hasWorker;
                m_selection.kind = 2;
                return;
            }
        }
    }
}

void Inspector::BuildDebugLabels(std::vector<RenderDebugLabel>& labels)
{
    if (m_selection.type == InspectorSelection::None) return;

    CoordinateSystem& coords = CoordinateSystem::GetInstance();
    char buf[128];
    int pos = 0;

    const char* typeName = "?";
    switch (m_selection.type) {
        case InspectorSelection::Flag:            typeName = "Flag"; break;
        case InspectorSelection::Building:        typeName = "Building"; break;
        case InspectorSelection::ConstructionSite: typeName = "ConstSite"; break;
        default: break;
    }

    pos += _snprintf(buf + pos, sizeof(buf) - pos, "[%s] ID=%u\n", typeName, m_selection.buildingId);

    if (m_selection.type == InspectorSelection::Building || m_selection.type == InspectorSelection::ConstructionSite) {
        pos += _snprintf(buf + pos, sizeof(buf) - pos, "Type=%u\n", m_selection.buildingType);
    }

    if (m_selection.type == InspectorSelection::Building) {
        pos += _snprintf(buf + pos, sizeof(buf) - pos, "FSM=%s Worker=%s Depleted=%s\n",
            (m_selection.fsmState == 0) ? "Idle" :
            (m_selection.fsmState == 1) ? "Producing" : "Full",
            m_selection.hasWorker ? "Yes" : "No",
            m_selection.depleted ? "Yes" : "No");
    }

    if (m_selection.type == InspectorSelection::ConstructionSite) {
        pos += _snprintf(buf + pos, sizeof(buf) - pos, "Builder=%s\n",
            m_selection.hasWorker ? "Yes" : "No");
    }

    RenderDebugLabel label;
    label.worldX = 10.0f;
    label.worldY = 80.0f;
    label.color = 0xCCFFFFFF;
    label.scale = 0.07f;
    label.fontId = 1;  // FONT_DEBUG
    label.style = 0;
    label.depth = 0.05f;
    label.layer = 0;
    label.isScreenSpace = true;
    strncpy_s(label.text, sizeof(label.text), buf, _TRUNCATE);
    labels.push_back(label);
}

void Inspector::Clear()
{
    m_selection = InspectorSelection();
}

} // namespace Scene
