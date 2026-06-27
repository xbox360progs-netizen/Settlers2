#pragma once

#include <vector>
#include <utility>
#include <string>

namespace World {
    class Map;
    class FlagManager;
    class RoadManager;
    class CarrierManager;
    class Flag;
    class TileLayer;
    class RoadNetworkRelinker;
}

namespace Core {
    class EventBus;
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

    // Status info
    const char* GetStatusText() const;
    float GetStatusTimer() const;
    void ClearStatus();

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

    const char* m_statusText;
    float m_statusTimer;

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

    // Internal helpers
    void SplitAtFlag(World::Flag* flag);
    bool IsNodeRoad(int nx, int ny, World::TileLayer* roadsLayer) const;
    int CalcPatternAt(int x, int y, World::TileLayer* roadsLayer) const;


};

} // namespace Scene
