#include "stdafx.h"
#include "RoadController.h"
#include "PlacementController.h"
#include "../Graphics/TextureRegistry.h"
#include "../Graphics/SpriteAtlas.h"
#include "../World/Map.h"
#include "../World/FlagManager.h"
#include "../World/RoadManager.h"
#include "../World/CarrierManager.h"
#include "../World/ObjectLifecycleManager.h"
#include "../World/ConstructionManager.h"
#include "../World/TileLayer.h"
#include "../World/Flag.h"
#include "../World/Road.h"
#include "../Core/EventBus.h"
#include "../Logic/AStar.h"
#include "../Logic/IsoNeighbors.h"

namespace Scene {

    RoadController::RoadController()
        : m_active(false)
        , m_startX(-1), m_startY(-1)
        , m_statusText(NULL)
        , m_statusTimer(0.0f)
        , m_map(NULL)
        , m_flagManager(NULL)
        , m_roadManager(NULL)
        , m_carrierManager(NULL)
        , m_eventBus(NULL)
        , m_lifecycleMgr(NULL)
        , m_constructionMgr(NULL)
        , m_placementCtrl(NULL)
    {
    }

    void RoadController::SetExternalManagers(
        World::Map* map,
        World::FlagManager* flagManager,
        World::RoadManager* roadManager,
        World::CarrierManager* carrierManager,
        Core::EventBus* eventBus,
        World::ObjectLifecycleManager* lifecycleMgr,
        World::ConstructionManager* constructionMgr)
    {
        m_map = map;
        m_flagManager = flagManager;
        m_roadManager = roadManager;
        m_carrierManager = carrierManager;
        m_eventBus = eventBus;
        m_lifecycleMgr = lifecycleMgr;
        m_constructionMgr = constructionMgr;
    }

    void RoadController::SetPlacementController(PlacementController* pc)
    {
        m_placementCtrl = pc;
    }

    bool RoadController::IsActive() const { return m_active; }

    const std::vector<std::pair<int,int>>& RoadController::GetPreviewPath() const { return m_previewPath; }
    const std::vector<std::pair<int,int>>& RoadController::GetValidNeighbors() const { return m_validNeighbors; }
    const std::vector<std::pair<int,int>>& RoadController::GetAutoPath() const { return m_autoPath; }
    int RoadController::GetStartX() const { return m_startX; }
    int RoadController::GetStartY() const { return m_startY; }
    const char* RoadController::GetStatusText() const { return m_statusText; }
    float RoadController::GetStatusTimer() const { return m_statusTimer; }
    void RoadController::ClearStatus() { m_statusText = NULL; m_statusTimer = 0.0f; }

    // ─── Static helpers ──────────────────────────────────────────────

    bool RoadController::IsNodeRoad(int nx, int ny, World::TileLayer* roadsLayer, const std::vector<std::pair<int,int>>& previewPath)
    {
        if (nx < 0 || ny < 0) return false;
        if (roadsLayer && roadsLayer->GetTile(nx, ny).regionIndex >= 0) return true;
        for (size_t k = 0; k < previewPath.size(); ++k) {
            if (previewPath[k].first == nx && previewPath[k].second == ny) return true;
        }
        return false;
    }

    int RoadController::CalcPatternAt(int x, int y, World::TileLayer* roadsLayer, const std::vector<std::pair<int,int>>& previewPath)
    {
        int pattern = 0;
        bool evenRow = (y % 2 == 0);
        if (evenRow) {
            if (IsNodeRoad(x+1, y-1, roadsLayer, previewPath)) pattern |= 1;
            if (IsNodeRoad(x+1, y+1, roadsLayer, previewPath)) pattern |= 2;
            if (IsNodeRoad(x, y+1, roadsLayer, previewPath))   pattern |= 4;
            if (IsNodeRoad(x, y-1, roadsLayer, previewPath))   pattern |= 8;
        } else {
            if (IsNodeRoad(x, y-1, roadsLayer, previewPath))   pattern |= 1;
            if (IsNodeRoad(x, y+1, roadsLayer, previewPath))   pattern |= 2;
            if (IsNodeRoad(x-1, y+1, roadsLayer, previewPath)) pattern |= 4;
            if (IsNodeRoad(x-1, y-1, roadsLayer, previewPath)) pattern |= 8;
        }
        return pattern;
    }

    std::vector<Vector2i> RoadController::FindTilePath(World::Map* map, int startX, int startY, int endX, int endY)
    {
        std::vector<Vector2i> result;
        if (!map) return result;
        World::TileLayer* roadsLayer = map->GetLayer(World::Roads);
        if (!roadsLayer) return result;

        int rw = roadsLayer->GetWidth();
        int rh = roadsLayer->GetHeight();
        if (startX < 0 || startX >= rw || startY < 0 || startY >= rh) return result;
        if (endX < 0 || endX >= rw || endY < 0 || endY >= rh) return result;

        std::vector<int> parent(rw * rh, -1);
        std::queue<std::pair<int,int>> q;
        q.push(std::make_pair(startX, startY));
        parent[startY * rw + startX] = -2;

        while (!q.empty()) {
            int cx = q.front().first;
            int cy = q.front().second;
            q.pop();

            if (cx == endX && cy == endY) {
                int px = endX, py = endY;
                while (px != startX || py != startY) {
                    Vector2i v; v.x = px; v.y = py;
                    result.push_back(v);
                    int p = parent[py * rw + px];
                    px = p & 0xFFFF;
                    py = (p >> 16) & 0xFFFF;
                    if (px == startX && py == startY) break;
                }
                Vector2i sv; sv.x = startX; sv.y = startY;
                result.push_back(sv);
                std::reverse(result.begin(), result.end());
                return result;
            }

            bool evenRow = (cy % 2 == 0);
            int nx[6], ny[6];
            if (evenRow) {
                int eNX[] = {cx-1, cx+1, cx-1, cx, cx-1, cx};
                int eNY[] = {cy, cy, cy-1, cy-1, cy+1, cy+1};
                memcpy(nx, eNX, sizeof(nx)); memcpy(ny, eNY, sizeof(ny));
            } else {
                int oNX[] = {cx-1, cx+1, cx, cx+1, cx, cx+1};
                int oNY[] = {cy, cy, cy-1, cy-1, cy+1, cy+1};
                memcpy(nx, oNX, sizeof(nx)); memcpy(ny, oNY, sizeof(ny));
            }
            for (int di = 0; di < 6; ++di) {
                int tx = nx[di], ty = ny[di];
                if (tx < 0 || tx >= rw || ty < 0 || ty >= rh) continue;
                if (parent[ty * rw + tx] != -1) continue;
                const World::Tile& rt = roadsLayer->GetTile(tx, ty);
                if (rt.atlasName != "streets") continue;
                parent[ty * rw + tx] = cx | (cy << 16);
                q.push(std::make_pair(tx, ty));
            }
        }
        return result;
    }

    // ─── Public API ──────────────────────────────────────────────────

    void RoadController::Start(int x, int y)
    {
        if (!m_map) return;
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        int nodesW = coords.GetNodesWidth();
        int nodesH = coords.GetNodesHeight();
        if (x < 0 || x >= nodesW || y < 0 || y >= nodesH) return;
        BYTE weight = m_map->GetNodeWeight(x, y);
        if (weight == World::Weight_Deep || weight == World::Weight_Block) return;

        if (!m_flagManager || !m_flagManager->GetFlagAt(x, y)) return;

        if (m_placementCtrl) m_placementCtrl->StartRoad(x, y);
        m_active = true;
        m_startX = x;
        m_startY = y;
        m_previewPath.clear();
        m_previewPath.push_back(std::make_pair(x, y));
        m_validNeighbors.clear();
        m_autoPath.clear();
        m_statusText = "ROAD: A=add tile  B=cancel";
        m_statusTimer = 0.0f;
        OutputDebugStringA("[RoadController] Road building started (tile-by-tile)\n");
    }

    void RoadController::UpdatePreview(int cursorX, int cursorY)
    {
        if (!m_active) return;
        if (!m_map || m_previewPath.empty()) return;

        int lastX = m_previewPath.back().first;
        int lastY = m_previewPath.back().second;

        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        int nodesW = coords.GetNodesWidth();
        int nodesH = coords.GetNodesHeight();
        World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
        World::TileLayer* objectsLayer = m_map->GetLayer(World::Objects);
        World::TileLayer* buildingsLayer = m_map->GetLayer(World::Buildings);
        World::TileLayer* placementLayer = m_map->GetLayer(World::Placement);

        bool evenRow = (lastY % 2 == 0);
        int nx[4], ny[4];
        if (evenRow) {
            nx[0] = lastX + 1; ny[0] = lastY - 1;
            nx[1] = lastX + 1; ny[1] = lastY + 1;
            nx[2] = lastX;     ny[2] = lastY + 1;
            nx[3] = lastX;     ny[3] = lastY - 1;
        } else {
            nx[0] = lastX;     ny[0] = lastY - 1;
            nx[1] = lastX;     ny[1] = lastY + 1;
            nx[2] = lastX - 1; ny[2] = lastY + 1;
            nx[3] = lastX - 1; ny[3] = lastY - 1;
        }

        m_validNeighbors.clear();
        for (int i = 0; i < 4; ++i) {
            int tx = nx[i], ty = ny[i];
            if (tx < 0 || tx >= nodesW || ty < 0 || ty >= nodesH) continue;

            bool alreadyPlaced = false;
            for (size_t j = 0; j < m_previewPath.size(); ++j) {
                if (m_previewPath[j].first == tx && m_previewPath[j].second == ty) {
                    alreadyPlaced = true; break;
                }
            }
            if (alreadyPlaced) continue;

            BYTE w = m_map->GetNodeWeight(tx, ty);
            if (w == World::Weight_Deep || w == World::Weight_Block) continue;

            bool hasFlag = m_flagManager && m_flagManager->GetFlagAt(tx, ty) != NULL;

            if (objectsLayer && !hasFlag) {
                const World::Tile& ot = objectsLayer->GetTile(tx, ty);
                if (ot.u1 > ot.u0 && ot.v1 > ot.v0) continue;
            }
            if (buildingsLayer && !hasFlag) {
                const World::Tile& bt = buildingsLayer->GetTile(tx, ty);
                if (bt.regionIndex >= 0) continue;
            }
            if (placementLayer && !hasFlag) {
                const World::Tile& pt = placementLayer->GetTile(tx, ty);
                if (pt.regionIndex >= 0 && pt.atlasName != "streets") continue;
            }

            m_validNeighbors.push_back(std::make_pair(tx, ty));
        }

        // Auto-path to cursor flag via A*
        m_autoPath.clear();
        bool cursorOnFlag = m_flagManager && m_flagManager->GetFlagAt(cursorX, cursorY) != NULL;
        bool cursorSameAsLast = (cursorX == lastX && cursorY == lastY);
        if (cursorOnFlag && !cursorSameAsLast) {
            struct RoadPassable {
                World::Map* map;
                World::FlagManager* flagManager;
                World::TileLayer* roadsLayer;
                World::TileLayer* objectsLayer;
                World::TileLayer* buildingsLayer;
                World::TileLayer* placementLayer;
                RoadPassable(World::Map* m, World::FlagManager* fm,
                    World::TileLayer* rl, World::TileLayer* ol,
                    World::TileLayer* bl, World::TileLayer* pl)
                    : map(m), flagManager(fm), roadsLayer(rl), objectsLayer(ol),
                      buildingsLayer(bl), placementLayer(pl) {}
                bool operator()(int x, int y) {
                    BYTE w = map->GetNodeWeight(x, y);
                    if (w == World::Weight_Deep || w == World::Weight_Block) return false;
                    bool hasFlag = flagManager && flagManager->GetFlagAt(x, y) != NULL;
                    if (hasFlag) return true;
                    if (roadsLayer) {
                        const World::Tile& rt = roadsLayer->GetTile(x, y);
                        if (rt.regionIndex >= 0) return true;
                    }
                    if (objectsLayer) {
                        const World::Tile& ot = objectsLayer->GetTile(x, y);
                        if (ot.u1 > ot.u0 && ot.v1 > ot.v0) return false;
                    }
                    if (buildingsLayer) {
                        const World::Tile& bt = buildingsLayer->GetTile(x, y);
                        if (bt.regionIndex >= 0) return false;
                    }
                    if (placementLayer) {
                        const World::Tile& pt = placementLayer->GetTile(x, y);
                        if (pt.regionIndex >= 0 && pt.atlasName != "streets") return false;
                    }
                    return true;
                }
            };
            struct RoadCost {
                float operator()(int, int) { return 1.0f; }
            };
            Logic::IsoNeighbors isoNeighbors;
            Logic::AStar::FindPath(
                lastX, lastY, cursorX, cursorY,
                nodesW, nodesH,
                RoadPassable(m_map, m_flagManager, roadsLayer, objectsLayer, buildingsLayer, placementLayer),
                RoadCost(),
                isoNeighbors,
                m_autoPath
            );
            if (!m_autoPath.empty() && m_autoPath[0].first == lastX && m_autoPath[0].second == lastY) {
                m_autoPath.erase(m_autoPath.begin());
            }
        }
    }

    bool RoadController::TryAddTile(int x, int y)
    {
        if (!m_active || m_previewPath.empty()) return false;

        int lastX = m_previewPath.back().first;
        int lastY = m_previewPath.back().second;

        // Cursor on the last tile → commit
        if (x == lastX && y == lastY) {
            Commit();
            return true;
        }

        // Auto-path to a flag
        if (!m_autoPath.empty()) {
            for (size_t i = 0; i < m_autoPath.size(); ++i) {
                m_previewPath.push_back(m_autoPath[i]);
            }
            m_statusText = "ROAD: auto-path built!";
            Commit();
            return true;
        }

        // Check if valid neighbor
        bool isValid = false;
        for (size_t i = 0; i < m_validNeighbors.size(); ++i) {
            if (m_validNeighbors[i].first == x && m_validNeighbors[i].second == y) {
                isValid = true; break;
            }
        }
        if (!isValid) {
            m_statusText = "Cannot build here!";
            m_statusTimer = 1.5f;
            return false;
        }

        m_previewPath.push_back(std::make_pair(x, y));

        char dbg[256];
        _snprintf(dbg, sizeof(dbg), "[Road] Added tile (%d,%d) pathLen=%u\n", x, y, (unsigned)m_previewPath.size());
        OutputDebugStringA(dbg);

        // If neighbor has flag or existing road → commit
        if (m_flagManager && m_flagManager->GetFlagAt(x, y)) {
            Commit();
            return true;
        }
        if (m_map) {
            World::TileLayer* rl = m_map->GetLayer(World::Roads);
            if (rl && rl->GetTile(x, y).regionIndex >= 0) {
                Commit();
                return true;
            }
        }

        m_statusText = "ROAD: A=add tile  B=cancel";
        return true;
    }

    void RoadController::Commit()
    {
        if (!m_active) return;
        if (!m_map) return;

        World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
        if (!roadsLayer) return;

        // Reject if any intermediate tile already has a road
        for (size_t i = 1; i + 1 < m_previewPath.size(); ++i) {
            int px = m_previewPath[i].first;
            int py = m_previewPath[i].second;
            const World::Tile& rt = roadsLayer->GetTile(px, py);
            if (rt.regionIndex >= 0) {
                m_statusText = "Cannot build through existing road!";
                m_statusTimer = 3.0f;
                Cancel();
                return;
            }
        }

        World::TileLayer* placementLayer = m_map->GetLayer(World::Placement);

        // Place road tiles on layer
        for (size_t i = 0; i < m_previewPath.size(); ++i) {
            int px = m_previewPath[i].first;
            int py = m_previewPath[i].second;
            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            int nodesW = coords.GetNodesWidth();
            int nodesH = coords.GetNodesHeight();
            if (px < 0 || px >= nodesW || py < 0 || py >= nodesH) continue;

            World::TileLayer* objectsLayer = m_map->GetLayer(World::Objects);
            bool hasObject = false;
            if (objectsLayer) {
                const World::Tile& ot = objectsLayer->GetTile(px, py);
                if (ot.u1 > ot.u0 && ot.v1 > ot.v0) hasObject = true;
            }
            if (hasObject) {
                if (!m_flagManager || !m_flagManager->GetFlagAt(px, py)) continue;
            }
            if (placementLayer) {
                const World::Tile& pt = placementLayer->GetTile(px, py);
                if (pt.regionIndex >= 0 && pt.atlasName != "streets") continue;
            }

            World::Tile& tile = roadsLayer->GetTile(px, py);
            if (tile.regionIndex < 0) {
                tile.type = World::Decoration;
                tile.regionIndex = 0;
                tile.atlasName = "streets";
                tile.walkable = true;
                tile.buildable = false;
                m_map->SetNodeWeight(px, py, World::Weight_Land);
                if (placementLayer) {
                    World::Tile& pt = placementLayer->GetTile(px, py);
                    pt.regionIndex = 0;
                    pt.type = World::Tile_None;
                    pt.atlasName = "streets";
                    pt.walkable = true;
                    pt.buildable = false;
                }
            } else {
                if (placementLayer) {
                    World::Tile& pt = placementLayer->GetTile(px, py);
                    if (pt.regionIndex < 0) {
                        pt.regionIndex = 0;
                        pt.type = World::Tile_None;
                        pt.atlasName = "streets";
                        pt.walkable = true;
                        pt.buildable = false;
                    }
                }
            }
        }

        // Rebuild sprites for all path tiles and their neighbors
        for (size_t i = 0; i < m_previewPath.size(); ++i) {
            int px = m_previewPath[i].first;
            int py = m_previewPath[i].second;
            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            int nodesW = coords.GetNodesWidth();
            int nodesH = coords.GetNodesHeight();
            if (px < 0 || px >= nodesW || py < 0 || py >= nodesH) continue;

            World::TileLayer* objectsLayer2 = m_map->GetLayer(World::Objects);
            bool hasObject = false;
            if (objectsLayer2) {
                const World::Tile& ot = objectsLayer2->GetTile(px, py);
                if (ot.u1 > ot.u0 && ot.v1 > ot.v0) hasObject = true;
            }
            if (hasObject) {
                if (!m_flagManager || !m_flagManager->GetFlagAt(px, py)) continue;
            }
            if (placementLayer) {
                const World::Tile& pt = placementLayer->GetTile(px, py);
                if (pt.regionIndex >= 0 && pt.atlasName != "streets") continue;
            }
            RebuildSprite(px, py);
            UpdateNeighbors(px, py);
        }

        if (!m_previewPath.empty()) {
            int endX = m_previewPath.back().first;
            int endY = m_previewPath.back().second;
            World::Flag* endFlag = NULL;
            if (m_flagManager) {
                endFlag = m_flagManager->GetFlagAt(endX, endY);
                if (!endFlag) {
                    endFlag = m_flagManager->CreateFlag(endX, endY);
                    endFlag->type = World::FLAG_NORMAL;
                }
                SplitAtFlag(endFlag);
            }

            World::Flag* startFlag = m_flagManager ? m_flagManager->GetFlagAt(m_startX, m_startY) : NULL;
            if (!startFlag) {
                bool evenRow = (m_startY % 2 == 0);
                int sx[6], sy[6];
                if (evenRow) {
                    int eSX[] = {m_startX-1, m_startX+1, m_startX-1, m_startX, m_startX-1, m_startX};
                    int eSY[] = {m_startY, m_startY, m_startY-1, m_startY-1, m_startY+1, m_startY+1};
                    memcpy(sx, eSX, sizeof(sx)); memcpy(sy, eSY, sizeof(sy));
                } else {
                    int oSX[] = {m_startX-1, m_startX+1, m_startX, m_startX+1, m_startX, m_startX+1};
                    int oSY[] = {m_startY, m_startY, m_startY-1, m_startY-1, m_startY+1, m_startY+1};
                    memcpy(sx, oSX, sizeof(sx)); memcpy(sy, oSY, sizeof(sy));
                }
                for (int di = 0; di < 6; ++di) {
                    World::Flag* adj = m_flagManager ? m_flagManager->GetFlagAt(sx[di], sy[di]) : NULL;
                    if (adj) { startFlag = adj; break; }
                }
            }
            if (startFlag && endFlag && startFlag != endFlag && m_roadManager) {
                std::vector<Vector2i> tilePath;
                for (size_t pi = 0; pi < m_previewPath.size(); ++pi) {
                    Vector2i v;
                    v.x = m_previewPath[pi].first;
                    v.y = m_previewPath[pi].second;
                    tilePath.push_back(v);
                }
                World::Road* road = m_roadManager->CreateRoad(startFlag, endFlag, tilePath);
                if (road) {
                    char dbg[256];
                    _snprintf(dbg, sizeof(dbg), "[RoadController] Road %u created: (%d,%d) <-> (%d,%d)\n",
                        road->id, m_startX, m_startY, endX, endY);
                    OutputDebugStringA(dbg);
                }
                SyncCarriers(startFlag);
                SyncCarriers(endFlag);
            }
        }

        if (m_eventBus) {
            Core::RoadBuiltData rd;
            rd.startX = m_startX;
            rd.startY = m_startY;
            rd.endX = m_previewPath.empty() ? -1 : m_previewPath.back().first;
            rd.endY = m_previewPath.empty() ? -1 : m_previewPath.back().second;
            rd.tileCount = (int)m_previewPath.size();
            m_eventBus->Post(Core::Event_RoadBuilt, rd);
        }

        m_statusText = "Road built!";
        m_statusTimer = 2.0f;
        Cancel();
    }

    void RoadController::Cancel()
    {
        if (m_placementCtrl) m_placementCtrl->Cancel();
        m_active = false;
        m_startX = -1;
        m_startY = -1;
        m_previewPath.clear();
        m_validNeighbors.clear();
        m_autoPath.clear();
    }

    // ─── Internal helpers ────────────────────────────────────────────

    bool RoadController::IsNodeRoad(int nx, int ny, World::TileLayer* roadsLayer) const
    {
        return IsNodeRoad(nx, ny, roadsLayer, m_previewPath);
    }

    int RoadController::CalcPatternAt(int x, int y, World::TileLayer* roadsLayer) const
    {
        return CalcPatternAt(x, y, roadsLayer, m_previewPath);
    }

    void RoadController::RebuildSprite(int x, int y)
    {
        TextureRegistry& reg = TextureRegistry::instance();
        std::tr1::shared_ptr<SpriteAtlas> roadsAtlas = reg.getAtlas("streets");
        if (!roadsAtlas) return;
        World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
        if (!roadsLayer) return;
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        int nodesW = coords.GetNodesWidth();
        int nodesH = coords.GetNodesHeight();
        if (x < 0 || x >= nodesW || y < 0 || y >= nodesH) return;

        World::Tile& tile = roadsLayer->GetTile(x, y);
        bool hasRoad = (tile.regionIndex >= 0);
        if (!hasRoad) return;

        int pattern = CalcPatternAt(x, y, roadsLayer);

        int connectionCount = 0;
        int temp = pattern;
        while (temp) {
            connectionCount += temp & 1;
            temp >>= 1;
        }

        const char* groupName = NULL;
        if (connectionCount == 1) {
            switch (pattern) {
                case 1:  groupName = "street_end_s"; break;
                case 2:  groupName = "street_end_w"; break;
                case 4:  groupName = "street_end_n"; break;
                case 8:  groupName = "street_end_e"; break;
                default: groupName = "street_1"; break;
            }
        } else {
            switch (pattern) {
                case 0:  groupName = "street_1"; break;
                case 1:  groupName = "street_1"; break;
                case 2:  groupName = "street_2"; break;
                case 3:  groupName = "street_3"; break;
                case 4:  groupName = "street_1"; break;
                case 5:  groupName = "street_5"; break;
                case 6:  groupName = "street_6"; break;
                case 7:  groupName = "street_7"; break;
                case 8:  groupName = "street_2"; break;
                case 9:  groupName = "street_9"; break;
                case 10: groupName = "street_2"; break;
                case 11: groupName = "street_11"; break;
                case 12: groupName = "street_12"; break;
                case 13: groupName = "street_13"; break;
                case 14: groupName = "street_14"; break;
                case 15: groupName = "street_15"; break;
                default: groupName = "street_1"; break;
            }
        }

        if (!groupName) {
            if (pattern == 2 || pattern == 8 || pattern == 10) {
                groupName = "street_2";
            } else {
                groupName = "street_1";
            }
        }

        const std::vector<uint32_t>* group = roadsAtlas->GetGroup(groupName);
        if (!group || group->empty()) {
            group = roadsAtlas->GetGroup("street_1");
            if (!group || group->empty()) return;
        }

        uint32_t regionIdx = (*group)[rand() % group->size()];
        const SpriteRegion* region = roadsAtlas->GetRegion(regionIdx);
        if (!region) return;

        tile.u0 = region->u0;
        tile.v0 = region->v0;
        tile.u1 = region->u1;
        tile.v1 = region->v1;
        tile.regionIndex = (int)regionIdx;
        tile.atlasName = "streets";
        tile.walkable = true;
        tile.buildable = false;
    }

    void RoadController::UpdateNeighbors(int x, int y)
    {
        World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
        if (!roadsLayer) return;
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        int nodesH = coords.GetNodesHeight();
        bool evenRow = (y % 2 == 0);

        if (y - 1 >= 0) {
            RebuildSprite(evenRow ? x : (x - 1), y - 1);
            RebuildSprite(evenRow ? (x + 1) : x, y - 1);
        }
        if (y + 1 < nodesH) {
            RebuildSprite(evenRow ? x : (x - 1), y + 1);
            RebuildSprite(evenRow ? (x + 1) : x, y + 1);
        }
    }

    void RoadController::SplitAtFlag(World::Flag* flag)
    {
        if (!flag || !m_roadManager || !m_carrierManager) return;

        for (size_t i = 0; i < m_roadManager->GetCount(); ++i) {
            World::Road* road = m_roadManager->GetRoad(i);
            if (!road) continue;

            int splitIdx = -1;
            for (uint32_t t = 1; t + 1 < road->tileCount; ++t) {
                if (road->tiles[t].x == flag->pos.x && road->tiles[t].y == flag->pos.y) {
                    splitIdx = (int)t;
                    break;
                }
            }
            if (splitIdx < 0) continue;

            World::Flag* ra = m_flagManager ? m_flagManager->ResolveFlag(road->a) : NULL;
            World::Flag* rb = m_flagManager ? m_flagManager->ResolveFlag(road->b) : NULL;

            World::Flag* a = ra;
            World::Flag* b = rb;

            if (!a || !b) continue;

            std::vector<Vector2i> pathAX, pathXB;
            for (uint32_t t = 0; t <= (uint32_t)splitIdx; ++t)
                pathAX.push_back(road->tiles[t]);
            for (uint32_t t = (uint32_t)splitIdx; t < road->tileCount; ++t)
                pathXB.push_back(road->tiles[t]);

            if (m_lifecycleMgr) m_lifecycleMgr->ForceDeleteRoad(road);

            World::Road* ax = m_roadManager->CreateRoad(a, flag, pathAX);
            World::Road* xb = m_roadManager->CreateRoad(flag, b, pathXB);

            if (ax) m_carrierManager->SyncCarriersForRoad(ax);
            if (xb) m_carrierManager->SyncCarriersForRoad(xb);

            return;
        }
    }

    void RoadController::SyncCarriers(World::Flag* flag)
    {
        if (!flag || !m_carrierManager || !m_roadManager) return;

        World::Flag* wh = m_constructionMgr ? m_constructionMgr->GetWarehouseFlag() : NULL;
        bool connected = false;
        if (wh && flag == wh) {
            connected = true;
        } else if (wh) {
            connected = (m_roadManager->FindFlagPath(wh, flag).size() >= 2);
        }

        if (!connected) return;

        for (size_t i = 0; i < flag->roads.size(); ++i) {
            World::Road* road = flag->roads[i];
            if (!road) continue;
            if (road->tileCount < 2) continue;
            if (m_carrierManager->GetCarrierForRoad(road)) continue;
            m_carrierManager->CreateCarrier(road);
            if (!m_carrierManager->GetCarrierForRoad(road)) continue;
            World::Flag* rra = m_flagManager ? m_flagManager->ResolveFlag(road->a) : NULL;
            World::Flag* rrb = m_flagManager ? m_flagManager->ResolveFlag(road->b) : NULL;
            World::Flag* other = (rra == flag) ? rrb : rra;
            if (other) {
                SyncCarriers(other);
            }
        }
    }

    void RoadController::LinkToNetwork(World::Flag* flag)
    {
        if (!flag || !m_flagManager || !m_roadManager) return;

        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        int nodesW = coords.GetNodesWidth();
        bool evenRow = (flag->pos.y % 2 == 0);
        for (int dir = 0; dir < 6; ++dir) {
            int nx, ny;
            if (evenRow) {
                int eNX[] = {flag->pos.x-1, flag->pos.x+1, flag->pos.x-1, flag->pos.x, flag->pos.x-1, flag->pos.x};
                int eNY[] = {flag->pos.y, flag->pos.y, flag->pos.y-1, flag->pos.y-1, flag->pos.y+1, flag->pos.y+1};
                nx = eNX[dir]; ny = eNY[dir];
            } else {
                int oNX[] = {flag->pos.x-1, flag->pos.x+1, flag->pos.x, flag->pos.x+1, flag->pos.x, flag->pos.x+1};
                int oNY[] = {flag->pos.y, flag->pos.y, flag->pos.y-1, flag->pos.y-1, flag->pos.y+1, flag->pos.y+1};
                nx = oNX[dir]; ny = oNY[dir];
            }
            if (nx < 0 || nx >= nodesW) continue;
            if (ny < 0 || ny >= nodesH) continue;

            int destWeight = m_map->GetNodeWeight(nx, ny);
            if (destWeight == 0 || destWeight == 3) continue;

            std::vector<Vector2i> path = FindTilePath(m_map, flag->pos.x, flag->pos.y, nx, ny);
            if (path.empty() || path.size() > 3) continue;

            World::Flag* destFlag = m_flagManager ? m_flagManager->GetFlagAt(nx, ny) : NULL;
            if (!destFlag) continue;

            World::Road* existing = m_roadManager->GetRoadBetween(flag, destFlag);
            if (existing) continue;

            // Check that the road doesn't pass through other flags
            bool passesThroughFlag = false;
            for (size_t ti = 0; ti < path.size(); ++ti) {
                if (path[ti].x == flag->pos.x && path[ti].y == flag->pos.y) continue;
                if (path[ti].x == destFlag->pos.x && path[ti].y == destFlag->pos.y) continue;
                if (m_flagManager->GetFlagAt(path[ti].x, path[ti].y)) {
                    passesThroughFlag = true; break;
                }
            }
            if (passesThroughFlag) continue;

            World::Road* road = m_roadManager->CreateRoad(flag, destFlag, path);
            if (road) {
                m_carrierManager->SyncCarriersForRoad(road);
            }
        }
    }

} // namespace Scene
