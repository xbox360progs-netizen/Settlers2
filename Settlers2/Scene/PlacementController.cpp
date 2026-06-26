#include "stdafx.h"
#include "PlacementController.h"
#include "../Graphics/TextureRegistry.h"
#include "../Graphics/SpriteAtlas.h"

namespace Scene {

    PlacementController::PlacementController()
        : m_state(PLACESTATE_NONE)
        , m_confirmAction(PLACECONFIRM_NONE)
        , m_confirmTargetX(-1)
        , m_confirmTargetY(-1)
        , m_selectedBuilding(World::Building_None)
        , m_iconIdx(-1)
        , m_constrIdx(-1)
        , m_placementManager(NULL)
    {
    }

    void PlacementController::SetPlacementManager(BuildingPlacementManager* mgr)
    {
        m_placementManager = mgr;
    }

    PlaceBuildState PlacementController::GetState() const { return m_state; }
    PlaceConfirmAction PlacementController::GetConfirmAction() const { return m_confirmAction; }
    int PlacementController::GetConfirmTargetX() const { return m_confirmTargetX; }
    int PlacementController::GetConfirmTargetY() const { return m_confirmTargetY; }
    World::BuildingType PlacementController::GetSelectedBuilding() const { return m_selectedBuilding; }
    int PlacementController::GetIconIdx() const { return m_iconIdx; }
    int PlacementController::GetConstrIdx() const { return m_constrIdx; }
    const std::string& PlacementController::GetIconName() const { return m_iconName; }
    bool PlacementController::IsIdle() const { return m_state == PLACESTATE_NONE; }

    void PlacementController::EnterBuildMode(World::BuildingType type, int iconIdx, int constrIdx, const std::string& iconName)
    {
        m_selectedBuilding = type;
        m_iconIdx = iconIdx;
        m_constrIdx = constrIdx;
        m_iconName = iconName;
        m_state = PLACESTATE_PLACE_FLAG;
        m_confirmAction = PLACECONFIRM_NONE;
    }

    void PlacementController::Cancel()
    {
        m_state = PLACESTATE_NONE;
        m_selectedBuilding = World::Building_None;
        m_iconIdx = -1;
        m_constrIdx = -1;
        m_iconName.clear();
        m_confirmAction = PLACECONFIRM_NONE;
    }

    void PlacementController::CancelConfirm()
    {
        m_confirmAction = PLACECONFIRM_NONE;
        m_state = PLACESTATE_NONE;
    }

    void PlacementController::SetConfirm(PlaceConfirmAction action, int targetX, int targetY)
    {
        m_confirmAction = action;
        m_confirmTargetX = targetX;
        m_confirmTargetY = targetY;
        m_state = PLACESTATE_CONFIRM;
    }

    void PlacementController::SetConfirmTarget(int targetX, int targetY)
    {
        m_confirmTargetX = targetX;
        m_confirmTargetY = targetY;
    }

    void PlacementController::StartRoad(int x, int y)
    {
        m_state = PLACESTATE_PLACE_ROAD;
        m_confirmAction = PLACECONFIRM_NONE;
        // road start coords stored externally in GameScene for now
    }

    PlacementData PlacementController::GetPlacementData(int cursorX, int cursorY)
    {
        if (m_selectedBuilding == World::Building_None || !m_placementManager) {
            PlacementData fallback;
            fallback.valid = false;
            return fallback;
        }
        m_placementManager->SelectBuilding(m_selectedBuilding);
        return m_placementManager->GetPlacementData(cursorX, cursorY);
    }

    PlacementRequest PlacementController::TryPlaceFlag(int tileX, int tileY)
    {
        PlacementRequest req;
        if (m_selectedBuilding == World::Building_None || !m_placementManager) {
            req.errorMsg = "no building selected";
            return req;
        }

        m_placementManager->SelectBuilding(m_selectedBuilding);
        PlacementData data = m_placementManager->GetPlacementData(tileX, tileY);
        if (!data.valid) {
            req.errorMsg = data.errorMsg ? data.errorMsg : "cannot place";
            return req;
        }

        req.valid = true;
        req.buildX = data.buildX;
        req.buildY = data.buildY;
        req.flagX = data.flagX;
        req.flagY = data.flagY;
        req.type = m_selectedBuilding;
        return req;
    }

    PlacementRequest PlacementController::TryPlaceFreeFlag(int tileX, int tileY)
    {
        PlacementRequest req;
        req.flagX = tileX;
        req.flagY = tileY;
        req.valid = true;
        return req;
    }

} // namespace Scene
