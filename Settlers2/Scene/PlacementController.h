#pragma once

#include "../World/Components/Building.h"
#include "BuildingPlacement.h"

namespace Scene {

enum PlaceBuildState {
    PLACESTATE_NONE,
    PLACESTATE_PLACE_FLAG,
    PLACESTATE_PLACE_ROAD,
    PLACESTATE_CONFIRM,
};

enum PlaceConfirmAction {
    PLACECONFIRM_NONE,
    PLACECONFIRM_PLACE_FLAG,
    PLACECONFIRM_START_ROAD,
    PLACECONFIRM_DELETE_FLAG,
};

struct PlacementRequest {
    bool valid;
    int buildX, buildY;
    int flagX, flagY;
    World::BuildingType type;
    const char* errorMsg;

    PlacementRequest() : valid(false), buildX(0), buildY(0), flagX(0), flagY(0), type(World::Building_None), errorMsg(NULL) {}
};

class PlacementController {
public:
    PlacementController();

    void SetPlacementManager(BuildingPlacementManager* mgr);

    PlaceBuildState GetState() const;
    PlaceConfirmAction GetConfirmAction() const;
    int GetConfirmTargetX() const;
    int GetConfirmTargetY() const;
    World::BuildingType GetSelectedBuilding() const;
    int GetIconIdx() const;
    int GetConstrIdx() const;
    const std::string& GetIconName() const;
    bool IsIdle() const;

    void EnterBuildMode(World::BuildingType type, int iconIdx, int constrIdx, const std::string& iconName);
    void Cancel();
    void CancelConfirm();
    void SetConfirm(PlaceConfirmAction action, int targetX, int targetY);
    void SetConfirmTarget(int targetX, int targetY);
    void StartRoad(int x, int y);

    PlacementData GetPlacementData(int cursorX, int cursorY);
    PlacementRequest TryPlaceFlag(int tileX, int tileY);
    PlacementRequest TryPlaceFreeFlag(int tileX, int tileY);

private:
    PlaceBuildState m_state;
    PlaceConfirmAction m_confirmAction;
    int m_confirmTargetX, m_confirmTargetY;
    World::BuildingType m_selectedBuilding;
    int m_iconIdx;
    int m_constrIdx;
    std::string m_iconName;
    BuildingPlacementManager* m_placementManager;
};

} // namespace Scene
