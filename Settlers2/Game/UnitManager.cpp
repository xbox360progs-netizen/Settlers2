#include "stdafx.h"
#include "Unit.h"
#include "../World/TileLayer.h"
#include "../Logic/AStar.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/RenderCommandBuilder.h"

namespace Game {

UnitManager::UnitManager()
    : m_map(NULL)
    , m_renderQueue(NULL)
    , m_textureSlot(0)
{
}

UnitManager::~UnitManager()
{
}

void UnitManager::Initialize(World::Map* map)
{
    m_map = map;
    m_units.clear();
    m_paths.clear();
}

struct RoadOnlyPassable {
    World::Map* map;
    RoadOnlyPassable(World::Map* m) : map(m) {}
    bool operator()(int x, int y) {
        World::TileLayer* roadsLayer = map->GetLayer(World::Roads);
        if (!roadsLayer) return false;
        if (x < 0 || x >= roadsLayer->GetWidth() || y < 0 || y >= roadsLayer->GetHeight()) return false;
        const World::Tile& tile = roadsLayer->GetTile(x, y);
        return tile.regionIndex >= 0;
    }
};

struct RoadCost {
    float operator()(int x, int y) { return 1.0f; }
};

void UnitManager::FindPathOnRoads(int fromX, int fromY, int toX, int toY, std::vector<std::pair<int,int>>& outPath)
{
    outPath.clear();
    if (!m_map) return;
    World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
    if (!roadsLayer) return;
    int w = roadsLayer->GetWidth();
    int h = roadsLayer->GetHeight();
    if (fromX < 0 || fromX >= w || fromY < 0 || fromY >= h) return;
    if (toX < 0 || toX >= w || toY < 0 || toY >= h) return;

    Logic::IsoNeighbors isoNeighbors;
    Logic::AStar::FindPath(fromX, fromY, toX, toY, w, h,
        RoadOnlyPassable(m_map), RoadCost(), isoNeighbors, outPath);
}

bool UnitManager::AreFlagsAdjacent(int ax, int ay, int bx, int by, const std::vector<std::pair<int,int>>& flags)
{
    std::vector<std::pair<int,int>> path;
    FindPathOnRoads(ax, ay, bx, by, path);
    if (path.size() < 2) return false;

    for (size_t pi = 1; pi < path.size() - 1; ++pi) {
        for (size_t fi = 0; fi < flags.size(); ++fi) {
            if (flags[fi].first == path[pi].first && flags[fi].second == path[pi].second)
                return false;
        }
    }
    return true;
}

int UnitManager::GetDirectionIndex(float dx, float dy)
{
    if (dx > 0.0f && dy < 0.0f) return 0;
    if (dx > 0.0f && dy > 0.0f) return 1;
    if (dx < 0.0f && dy < 0.0f) return 2;
    if (dx < 0.0f && dy > 0.0f) return 3;
    return (dy >= 0.0f) ? 1 : 0;
}

void UnitManager::WalkUnitFromTownhallToSegment(int townhallX, int townhallY, int ax, int ay, int bx, int by)
{
    if (!m_atlas) return;

    // Check if unit already exists for this segment
    for (size_t i = 0; i < m_units.size(); ++i) {
        if (!m_units[i].active || m_units[i].returningHome) continue;
        if ((m_units[i].flagAX == ax && m_units[i].flagAY == ay &&
             m_units[i].flagBX == bx && m_units[i].flagBY == by) ||
            (m_units[i].flagAX == bx && m_units[i].flagAY == by &&
             m_units[i].flagBX == ax && m_units[i].flagBY == ay))
            return;
    }

    // Pathfind A→B to get segment path and find its midpoint
    std::vector<std::pair<int,int>> segPath;
    FindPathOnRoads(ax, ay, bx, by, segPath);
    if (segPath.size() < 2) return;

    int midIdx = (int)segPath.size() / 2;
    int midX = segPath[midIdx].first;
    int midY = segPath[midIdx].second;

    // Pathfind from townhall to the segment midpoint
    std::vector<std::pair<int,int>> walkPath;
    FindPathOnRoads(townhallX, townhallY, midX, midY, walkPath);
    if (walkPath.size() < 2) return;

    UnitPath up;
    up.nodes = walkPath;
    up.segmentAX = ax;
    up.segmentAY = ay;
    up.segmentBX = bx;
    up.segmentBY = by;
    m_paths.push_back(up);

    Unit u;
    u.flagAX = ax;
    u.flagAY = ay;
    u.flagBX = bx;
    u.flagBY = by;
    u.t = 0.0f;
    u.speed = 0.2f;
    u.active = true;
    u.returningHome = false;
    CoordinateSystem& coords = CoordinateSystem::GetInstance();
    coords.NodeTileToWorld(townhallX, townhallY, u.worldX, u.worldY);
    u.dirIndex = 1;
    m_units.push_back(u);
}

void UnitManager::RebuildRoadNetwork(const std::vector<std::pair<int,int>>& flags)
{
    if (!m_map) {
        for (size_t i = 0; i < m_units.size(); ++i)
            if (m_units[i].active && !m_units[i].returningHome)
                m_units[i].returningHome = true;
        return;
    }

    if (flags.size() < 2) {
        for (size_t i = 0; i < m_units.size(); ++i)
            if (m_units[i].active && !m_units[i].returningHome)
                m_units[i].returningHome = true;
        return;
    }

    int townhallX = flags[0].first;
    int townhallY = flags[0].second;

    // Mark existing units whose segment no longer exists as returningHome
    for (size_t i = 0; i < m_units.size(); ++i) {
        if (!m_units[i].active || m_units[i].returningHome) continue;
        bool segmentExists = false;
        bool foundA = false, foundB = false;
        for (size_t fi = 0; fi < flags.size(); ++fi) {
            if (flags[fi].first == m_units[i].flagAX && flags[fi].second == m_units[i].flagAY)
                foundA = true;
            if (flags[fi].first == m_units[i].flagBX && flags[fi].second == m_units[i].flagBY)
                foundB = true;
        }
        if (foundA && foundB) {
            segmentExists = AreFlagsAdjacent(
                m_units[i].flagAX, m_units[i].flagAY,
                m_units[i].flagBX, m_units[i].flagBY, flags);
        }
        if (!segmentExists) {
            m_units[i].returningHome = true;
        }
    }

    // Walk new units from townhall to each adjacent segment's midpoint
    for (size_t i = 0; i < flags.size(); ++i) {
        for (size_t j = i + 1; j < flags.size(); ++j) {
            if (AreFlagsAdjacent(flags[i].first, flags[i].second,
                                 flags[j].first, flags[j].second, flags)) {
                WalkUnitFromTownhallToSegment(townhallX, townhallY,
                    flags[i].first, flags[i].second,
                    flags[j].first, flags[j].second);
            }
        }
    }
}

void UnitManager::Update(float deltaTime)
{
    if (!m_map || !m_atlas) return;

    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    for (size_t i = 0; i < m_units.size(); ++i) {
        Unit& u = m_units[i];
        if (!u.active) continue;

        // Find the matching path (by segment endpoints)
        UnitPath* path = NULL;
        for (size_t pi = 0; pi < m_paths.size(); ++pi) {
            if (m_paths[pi].nodes.empty()) continue;
            if ((m_paths[pi].segmentAX == u.flagAX && m_paths[pi].segmentAY == u.flagAY &&
                 m_paths[pi].segmentBX == u.flagBX && m_paths[pi].segmentBY == u.flagBY) ||
                (m_paths[pi].segmentAX == u.flagBX && m_paths[pi].segmentAY == u.flagBY &&
                 m_paths[pi].segmentBX == u.flagAX && m_paths[pi].segmentBY == u.flagAY)) {
                path = &m_paths[pi];
                break;
            }
        }

        if (!path || path->nodes.size() < 2) {
            u.active = false;
            continue;
        }

        float totalLen = (float)(path->nodes.size() - 1);

        if (u.returningHome) {
            u.t -= u.speed * deltaTime;
            if (u.t <= 0.0f) {
                u.active = false;
                continue;
            }
        } else {
            // Check if road at current position still exists
            int curIdx = (int)(u.t * totalLen);
            if (curIdx < 0) curIdx = 0;
            if (curIdx >= (int)path->nodes.size()) curIdx = (int)path->nodes.size() - 1;
            int curX = path->nodes[curIdx].first;
            int curY = path->nodes[curIdx].second;
            World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
            if (roadsLayer) {
                const World::Tile& ct = roadsLayer->GetTile(curX, curY);
                if (ct.regionIndex < 0) {
                    u.returningHome = true;
                    continue;
                }
            }

            // Walk from townhall to segment midpoint and stop
            u.t += u.speed * deltaTime;
            if (u.t > 1.0f) u.t = 1.0f;
        }

        float totalT = u.t * totalLen;
        int segIdx = (int)totalT;
        float segT = totalT - segIdx;
        if (segIdx >= (int)path->nodes.size() - 1) {
            segIdx = (int)path->nodes.size() - 2;
            segT = 1.0f;
        }

        int x0 = path->nodes[segIdx].first;
        int y0 = path->nodes[segIdx].second;
        int x1 = path->nodes[segIdx + 1].first;
        int y1 = path->nodes[segIdx + 1].second;

        float wx0, wy0, wx1, wy1;
        coords.NodeTileToWorld(x0, y0, wx0, wy0);
        coords.NodeTileToWorld(x1, y1, wx1, wy1);

        u.worldX = wx0 + (wx1 - wx0) * segT;
        u.worldY = wy0 + (wy1 - wy0) * segT;

        float dx = wx1 - wx0;
        float dy = wy1 - wy0;
        if (u.returningHome) { dx = -dx; dy = -dy; }
        u.dirIndex = GetDirectionIndex(dx, dy);
    }
}

void UnitManager::Render()
{
    if (!m_atlas || !m_renderQueue) return;

    const int DIR_INDICES[4] = { 0, 1, 2, 3 };

    for (size_t i = 0; i < m_units.size(); ++i) {
        const Unit& u = m_units[i];
        if (!u.active) continue;

        int spriteIdx = DIR_INDICES[u.dirIndex];
        const SpriteRegion* region = m_atlas->GetRegion(spriteIdx);
        if (!region) continue;

        Graphics::RenderCommandBuilder()
            .WorldSprite(u.worldX - region->pivotX, u.worldY - region->pivotY,
                (float)region->width, (float)region->height,
                region->u0, region->v0, region->u1, region->v1,
                m_textureSlot, static_cast<WORD>(30020 + u.flagBY * 400))
            .Submit(m_renderQueue);
    }
}

} // namespace Game
