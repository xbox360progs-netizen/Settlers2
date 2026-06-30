#pragma once

#include <vector>
#include <utility>

namespace World {
    class Map;
    class FlagManager;
    class RoadManager;
    class CarrierManager;
    class Flag;
    class TileLayer;
    class RoadNetworkRelinker;
    class ObjectLifecycleManager;
    class ConstructionManager;
}

namespace Core {
    class EventBus;
}

namespace UI {
    class StatusManager;
}

namespace Scene {

class PlacementController;

class RoadController {
public:
    RoadController();

    void SetExternalManagers(
        World::Map* map,
        World::FlagManager* flagManager,
        World::RoadManager* roadManager,
        World::CarrierManager* carrierManager,
        Core::EventBus* eventBus,
        class World::ObjectLifecycleManager* lifecycleMgr,
        class World::ConstructionManager* constructionMgr
    );
    void SetPlacementController(PlacementController* pc);
    void SetRelinker(World::RoadNetworkRelinker* relinker);
    void SetStatusManager(UI::StatusManager* sm) { m_statusManager = sm; }

    bool IsActive() const;

    void Start(int x, int y);
    void UpdatePreview(int cursorX, int cursorY);
    bool TryAddTile(int x, int y);
    void Commit();
    void Cancel();

    // Read-only access for rendering
    const std::vector<std::pair<int,int>>& GetPreviewPath() const;
    const std::vector<std::pair<int,int>>& GetValidNeighbors() const;
    const std::vector<std::pair<int,int>>& GetAutoPath() const;
    int GetStartX() const;
    int GetStartY() const;

    // Static helpers for rendering
    static bool IsNodeRoad(int nx, int ny, World::TileLayer* roadsLayer, const std::vector<std::pair<int,int>>& previewPath);
    static int CalcPatternAt(int x, int y, World::TileLayer* roadsLayer, const std::vector<std::pair<int,int>>& previewPath);

    // Public helpers for external road tile update (flag deletion, etc.)
    void UpdateNeighbors(int x, int y);
    void RebuildSprite(int x, int y);

private:
    bool m_active;
    int m_startX, m_startY;
    std::vector<std::pair<int,int>> m_previewPath;
    std::vector<std::pair<int,int>> m_validNeighbors;
    std::vector<std::pair<int,int>> m_autoPath;

    // External dependencies
    World::Map* m_map;
    World::FlagManager* m_flagManager;
    World::RoadManager* m_roadManager;
    World::CarrierManager* m_carrierManager;
    Core::EventBus* m_eventBus;
    class World::ObjectLifecycleManager* m_lifecycleMgr;
    class World::ConstructionManager* m_constructionMgr;
    class World::RoadNetworkRelinker* m_relinker;
    PlacementController* m_placementCtrl;
    UI::StatusManager* m_statusManager;

    // Internal helpers
    void SplitAtFlag(World::Flag* flag);
    bool IsNodeRoad(int nx, int ny, World::TileLayer* roadsLayer) const;
    int CalcPatternAt(int x, int y, World::TileLayer* roadsLayer) const;


};

} // namespace Scene
