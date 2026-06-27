#pragma once

namespace World {
    class Map;
    class CarrierManager;
    class Flag;
    class FlagManager;
    class RoadManager;

class RoadNetworkRelinker {
public:
    RoadNetworkRelinker();

    void SetManagers(Map* map, FlagManager* flagManager, RoadManager* roadManager, CarrierManager* carrierManager);

    // BFS from root flag across existing road tiles, creating Road objects
    // between reachable flags. Called during load/restore and after road commits.
    void RebuildFromFlag(Flag* root);

    // Create carriers for each road connected to root, propagating recursively
    // to neighbors. Only spawns carriers if connected to warehouse.
    // warehouse may be NULL; if so, the relinker searches FlagManager for a FLAG_WAREHOUSE.
    void SyncCarriers(Flag* root, Flag* warehouse = NULL);

private:
    void SyncCarrierAssignments(Flag* flag);
    Flag* FindWarehouse() const;

    Map* m_map;
    FlagManager* m_flagManager;
    RoadManager* m_roadManager;
    CarrierManager* m_carrierManager;
};

} // namespace World
