#include "stdafx.h"
#include "WorkerManager.h"
#include "RoadManager.h"

namespace World {

    WorkerManager::WorkerManager()
        : m_warehouse(NULL), m_roadManager(NULL)
    {}

    WorkerManager::~WorkerManager() {
        Clear();
    }

    void WorkerManager::SpawnWorker(Building* home, float startX, float startY) {
        if (!home) return;
        Worker* w = new Worker(home, startX, startY);

        // Compute road route from warehouse flag to building flag
        if (m_roadManager && m_warehouse && m_warehouse->connectedFlag && home->connectedFlag) {
            Flag* whFlag = m_warehouse->connectedFlag;
            Flag* bldFlag = home->connectedFlag;
            w->route = m_roadManager->FindFlagPath(whFlag, bldFlag);
            if (w->route.size() >= 2) {
                w->routeIndex = 0;
                // Initialize first leg direction (like builder InitBuilderFirstLeg)
                {
                    Flag* fromFlag = w->route[0];
                    Flag* toFlag = w->route[1];
                    Road* road = m_roadManager->GetRoadBetween(fromFlag, toFlag);
                    if (road && road->tiles.size() >= 2) {
                        float pathLen = (float)(road->tiles.size() - 1);
                        if (fromFlag->pos.x == road->tiles[0].x && fromFlag->pos.y == road->tiles[0].y) {
                            w->walkDir = 1.0f;
                            w->ep = 0.0f;
                        } else {
                            w->walkDir = -1.0f;
                            w->ep = pathLen;
                        }
                    } else {
                        w->walkDir = 1.0f;
                        w->ep = 0.0f;
                    }
                }
                char buf[512];
                size_t pos = _snprintf(buf, sizeof(buf),
                    "[Worker] Spawn: route %u flags [", (unsigned)w->route.size());
                for (size_t i = 0; i < w->route.size(); ++i) {
                    pos += _snprintf(buf + pos, sizeof(buf) - pos, "%s#%u(%d,%d)",
                        i > 0 ? " " : "",
                        w->route[i]->id,
                        w->route[i]->pos.x,
                        w->route[i]->pos.y);
                }
                _snprintf(buf + pos, sizeof(buf) - pos, "]\n");
                OutputDebugStringA(buf);
            } else {
                OutputDebugStringA("[Worker] Spawn: no road route, walking directly\n");
            }
        } else {
            char buf[256];
            _snprintf(buf, sizeof(buf),
                "[Worker] Spawn: cannot compute route — rm=%d wh=%d whFlag=%d bldFlag=%d\n",
                m_roadManager ? 1 : 0,
                m_warehouse ? 1 : 0,
                (m_warehouse && m_warehouse->connectedFlag) ? 1 : 0,
                (home && home->connectedFlag) ? 1 : 0);
            OutputDebugStringA(buf);
        }

        m_transit.push_back(w);
    }

    void WorkerManager::Update(float dt) {
        if (m_transit.empty()) return;
        for (int i = (int)m_transit.size() - 1; i >= 0; --i) {
            Worker* w = m_transit[i];
            bool stillMoving = w->Update(dt, m_roadManager);
            char buf[256];
            const char* routeDesc = (w->route.size() >= 2) ? "road" : "direct";
            _snprintf(buf, sizeof(buf),
                "[Worker] Update: home=%p type=%d pos=(%.1f,%.1f) target=(%d,%d) route=%s moving=%d\n",
                (void*)w->home, w->home ? w->home->type : -1,
                w->wx, w->wy,
                w->home && w->home->connectedFlag ? w->home->connectedFlag->pos.x : 0,
                w->home && w->home->connectedFlag ? w->home->connectedFlag->pos.y : 0,
                routeDesc, stillMoving ? 1 : 0);
            OutputDebugStringA(buf);
            if (!stillMoving) {
                if (w->home) {
                    _snprintf(buf, sizeof(buf),
                        "[Worker] ARRIVED: building=%p type=%d pop=%d->1\n",
                        (void*)w->home, w->home->type, w->home->m_population);
                    OutputDebugStringA(buf);
                    w->home->m_population = 1;
                }
                if (m_warehouse) {
                    w->state = WorkerState_Idle;
                    m_warehouse->specialists.push_back(w);
                } else {
                    delete w;
                }
                m_transit.erase(m_transit.begin() + i);
            }
        }
    }

    void WorkerManager::Clear() {
        for (size_t i = 0; i < m_transit.size(); ++i)
            delete m_transit[i];
        m_transit.clear();
    }

} // namespace World
