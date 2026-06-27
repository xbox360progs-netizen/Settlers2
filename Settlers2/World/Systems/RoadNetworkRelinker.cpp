#include "stdafx.h"
#include "RoadNetworkRelinker.h"
#include "../FlagManager.h"
#include "../RoadManager.h"
#include "../CarrierManager.h"
#include "../Flag.h"
#include "../Road.h"
#include "../Map.h"
#include "../../Logic/CoordinateSystem.h"
#include <queue>
#include <algorithm>

namespace World {

RoadNetworkRelinker::RoadNetworkRelinker()
    : m_map(NULL)
    , m_flagManager(NULL)
    , m_roadManager(NULL)
    , m_carrierManager(NULL)
{
}

void RoadNetworkRelinker::SetManagers(Map* map, FlagManager* flagManager, RoadManager* roadManager, CarrierManager* carrierManager)
{
    m_map = map;
    m_flagManager = flagManager;
    m_roadManager = roadManager;
    m_carrierManager = carrierManager;
}

void RoadNetworkRelinker::RebuildFromFlag(Flag* root)
{
    if (!root || !m_map || !m_flagManager || !m_roadManager) return;

    TileLayer* roadsLayer = m_map->GetLayer(LayerType::Roads);
    if (!roadsLayer) return;

    int rw = roadsLayer->GetWidth();
    int rh = roadsLayer->GetHeight();

    int roadsCreated = 0;
    {
        std::vector<bool> visited(rw * rh, false);
        std::queue<std::pair<int,int>> q;
        std::vector<int> parent(rw * rh, -1);
        q.push(std::make_pair(root->pos.x, root->pos.y));
        visited[root->pos.y * rw + root->pos.x] = true;
        parent[root->pos.y * rw + root->pos.x] = -2;

        while (!q.empty()) {
            int cx = q.front().first;
            int cy = q.front().second;
            q.pop();

            Flag* other = (cx == root->pos.x && cy == root->pos.y) ? NULL : m_flagManager->GetFlagAt(cx, cy);
            if (other) {
                if (!m_roadManager->GetRoadBetween(root, other)) {
                    std::vector<Vector2i> tilePath;
                    int px = cx, py = cy;
                    while (px != root->pos.x || py != root->pos.y) {
                        Vector2i v; v.x = px; v.y = py;
                        tilePath.push_back(v);
                        int p = parent[py * rw + px];
                        px = p & 0xFFFF;
                        py = (p >> 16) & 0xFFFF;
                    }
                    Vector2i sv; sv.x = root->pos.x; sv.y = root->pos.y;
                    tilePath.push_back(sv);
                    std::reverse(tilePath.begin(), tilePath.end());
                    m_roadManager->CreateRoad(root, other, tilePath);
                    roadsCreated++;
                }
                continue;
            }

            bool evenRow = (cy % 2 == 0);
            int nx[6], ny[6];
            if (evenRow) {
                int eNX[] = {cx-1, cx+1, cx-1, cx, cx-1, cx};
                int eNY[] = {cy, cy, cy-1, cy-1, cy+1, cy+1};
                memcpy(nx, eNX, sizeof(nx));
                memcpy(ny, eNY, sizeof(ny));
            } else {
                int oNX[] = {cx-1, cx+1, cx, cx+1, cx, cx+1};
                int oNY[] = {cy, cy, cy-1, cy-1, cy+1, cy+1};
                memcpy(nx, oNX, sizeof(nx));
                memcpy(ny, oNY, sizeof(ny));
            }
            for (int di = 0; di < 6; ++di) {
                int tx = nx[di];
                int ty = ny[di];
                if (tx < 0 || tx >= rw || ty < 0 || ty >= rh) continue;
                if (visited[ty * rw + tx]) continue;
                const Tile& rt = roadsLayer->GetTile(tx, ty);
                if (rt.atlasName != "streets") continue;
                visited[ty * rw + tx] = true;
                parent[ty * rw + tx] = cx | (cy << 16);
                q.push(std::make_pair(tx, ty));
            }
        }
    }

    if (root->roads.empty()) {
        char dbg[256];
        _snprintf(dbg, sizeof(dbg), "[Relinker] Flag %u at (%d,%d) isolated\n",
            root->id, root->pos.x, root->pos.y);
        OutputDebugStringA(dbg);
    }
}

void RoadNetworkRelinker::SyncCarriers(Flag* root, Flag* warehouse)
{
    if (!root || !m_carrierManager || !m_roadManager || !m_flagManager) return;

    if (!warehouse) {
        warehouse = FindWarehouse();
    }

    bool connected = false;
    if (warehouse && root == warehouse) {
        connected = true;
    } else if (warehouse) {
        connected = (m_roadManager->FindFlagPath(warehouse, root).size() >= 2);
    }

    char dbg[256];
    _snprintf(dbg, sizeof(dbg), "[Relinker] SyncCarriers for Flag %u (%d,%d) roads=%u %s\n",
        root->id, root->pos.x, root->pos.y, (unsigned)root->roads.size(),
        connected ? "(connected)" : "(isolated)");
    OutputDebugStringA(dbg);
    if (!connected) return;

    SyncCarrierAssignments(root);
}

void RoadNetworkRelinker::SyncCarrierAssignments(Flag* flag)
{
    for (size_t i = 0; i < flag->roads.size(); ++i) {
        Road* road = flag->roads[i];
        if (!road) continue;
        if (road->tileCount < 2) continue;
        if (m_carrierManager->GetCarrierForRoad(road)) continue;
        m_carrierManager->CreateCarrier(road);
        if (!m_carrierManager->GetCarrierForRoad(road)) continue;
        Flag* rra = m_flagManager ? m_flagManager->ResolveFlag(road->a) : NULL;
        Flag* rrb = m_flagManager ? m_flagManager->ResolveFlag(road->b) : NULL;
        Flag* other = (rra == flag) ? rrb : rra;
        if (other) {
            SyncCarrierAssignments(other);
        }
    }
}

Flag* RoadNetworkRelinker::FindWarehouse() const
{
    if (!m_flagManager) return NULL;
    for (size_t i = 0; i < m_flagManager->GetCount(); ++i) {
        Flag* f = m_flagManager->GetFlag(i);
        if (f && f->type == FLAG_WAREHOUSE) return f;
    }
    return NULL;
}

} // namespace World
