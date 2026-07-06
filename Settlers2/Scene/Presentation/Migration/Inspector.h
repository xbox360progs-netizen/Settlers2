#pragma once
#include <stdint.h>
#include <vector>

// Skeleton Inspector — migration verification tool.
// Click an entity → see its ID + state in a debug label overlay.
// Not a full UI; purpose is to verify that BuildingSource/FlagSource
// return correct data from SimulationCore during migration.

namespace Scene {

struct RenderDebugLabel;
class IFlagSource;
class IBuildingSource;
class IConstructionSiteSource;

struct InspectorSelection {
    enum Type { None, Flag, Building, ConstructionSite };

    Type     type;
    uint32_t buildingId;   // View-level ID (matches BuildingView::buildingId)
    uint8_t  buildingType; // legacy BuildingType enum value
    uint8_t  fsmState;     // 0=Idle, 1=Producing, 2=OutputFull
    uint8_t  kind;         // 0=flag, 1=building, 2=constructionsite
    bool     hasWorker;
    bool     depleted;

    InspectorSelection()
        : type(None)
        , buildingId(0)
        , buildingType(0)
        , fsmState(0)
        , kind(0)
        , hasWorker(false)
        , depleted(false)
    {
    }
};

class Inspector {
public:
    void SetSources(
        IFlagSource* flagSource,
        IBuildingSource* buildingSource,
        IConstructionSiteSource* constructionSiteSource
    );

    // Call on mouse click. worldX/worldY are camera-projected world coords.
    void OnClick(float worldX, float worldY);

    // Append debug labels for current selection into the frame's label list.
    void BuildDebugLabels(std::vector<RenderDebugLabel>& labels);

    void Clear();

private:
    IFlagSource*             m_flagSource;
    IBuildingSource*         m_buildingSource;
    IConstructionSiteSource* m_constructionSiteSource;
    InspectorSelection       m_selection;
};

} // namespace Scene
