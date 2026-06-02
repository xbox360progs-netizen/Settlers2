#pragma once
#include "../World/Map.h"
#include "../Logic/CoordinateSystem.h"
#include "../Graphics/SpriteAtlas.h"
#include "../Graphics/RenderQueue.h"
#include "../Graphics/RenderLayers.h"
#include <vector>

namespace Game {

struct Unit {
    int flagAX, flagAY;
    int flagBX, flagBY;
    float t;
    float speed;
    float worldX, worldY;
    int dirIndex;
    bool active;
    bool returningHome;
};

class UnitManager {
public:
    UnitManager();
    ~UnitManager();

    void Initialize(World::Map* map);
    void SetRenderQueue(Graphics::RenderQueue* queue) { m_renderQueue = queue; }

    void RebuildRoadNetwork(const std::vector<std::pair<int,int>>& flags);
    void Update(float deltaTime);
    void Render();

    void SetAtlas(std::tr1::shared_ptr<SpriteAtlas> atlas) { m_atlas = atlas; }
    void SetTextureSlot(WORD slot) { m_textureSlot = slot; }

private:
    void WalkUnitFromTownhallToSegment(int townhallX, int townhallY, int ax, int ay, int bx, int by);
    void FindPathOnRoads(int fromX, int fromY, int toX, int toY, std::vector<std::pair<int,int>>& outPath);
    bool AreFlagsAdjacent(int ax, int ay, int bx, int by, const std::vector<std::pair<int,int>>& flags);
    int GetDirectionIndex(float dx, float dy);

    struct UnitPath {
        std::vector<std::pair<int,int>> nodes;
        int segmentAX, segmentAY;
        int segmentBX, segmentBY;
    };
    std::vector<Unit> m_units;
    std::vector<UnitPath> m_paths;
    World::Map* m_map;
    std::tr1::shared_ptr<SpriteAtlas> m_atlas;
    Graphics::RenderQueue* m_renderQueue;
    WORD m_textureSlot;
};

} // namespace Game
