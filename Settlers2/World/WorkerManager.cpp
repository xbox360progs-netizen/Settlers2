#include "stdafx.h"
#include "WorkerManager.h"
#include "RoadManager.h"
#include "Flag.h"
#include "Components/Building.h"
#include <string.h>

namespace World {

    WorkerManager::WorkerManager()
        : m_activeCount(0), m_freeCount(0), m_updateIndex(0), m_roadManager(NULL)
    {
        for (int i = 0; i < MAX_TRANSIT_WORKERS; ++i) {
            m_freeSlots[i] = i;
        }
        m_freeCount = MAX_TRANSIT_WORKERS;
    }

    WorkerManager::~WorkerManager() {
        Clear();
    }

    int WorkerManager::SpawnWorker(Building* home, Flag* startFlag) {
        if (m_freeCount <= 0) {
            OutputDebugStringA("[WorkerManager] Pool exhausted!\n");
            return -1;
        }
        int idx = m_freeSlots[--m_freeCount];
        Worker& w = m_pool[idx];
        w.state = WorkerState_MovingToJob;
        w.homeBuildingIdx = (uint32_t)(size_t)home;
        w.ep = 0.0f;
        w.walkDir = 1.0f;
        w.routeIndex = 0;
        w.profession = Profession_Transit;
        w.carriedResource = ResourceType_None;
        w.stateTimer = 0;
        w.padding[0] = 0;

        // Build route from warehouse flag to building flag via roads
        memset(m_routes[idx].flags, 0, sizeof(m_routes[idx].flags));
        w.routeCount = 0;
        if (home && home->connectedFlag && m_roadManager && startFlag) {
            std::vector<Flag*> path = m_roadManager->FindFlagPath(startFlag, home->connectedFlag);
            for (size_t i = 0; i < path.size() && i < MAX_WORKER_ROUTE_FLAGS; ++i) {
                m_routes[idx].flags[i] = path[i];
                ++w.routeCount;
            }
        }
        // Fallback: direct destination if no road path found
        if (w.routeCount == 0 && home && home->connectedFlag) {
            m_routes[idx].flags[0] = home->connectedFlag;
            w.routeCount = 1;
        }
        w.posX = startFlag ? (float)startFlag->pos.x : 0.0f;
        w.posY = startFlag ? (float)startFlag->pos.y : 0.0f;

        char buf[256];
        _snprintf(buf, sizeof(buf), "[Worker] Spawn idx=%d home=%p route=%u\n",
            idx, (void*)home, (unsigned)w.routeCount);
        OutputDebugStringA(buf);

        m_activeIndices[m_activeCount++] = idx;
        return idx;
    }

    void WorkerManager::FreeWorker(int index) {
        if (index < 0 || index >= MAX_TRANSIT_WORKERS) return;
        m_pool[index].state = WorkerState_Idle;

        // Clear cold route
        memset(m_routes[index].flags, 0, sizeof(m_routes[index].flags));

        for (int i = 0; i < m_activeCount; ++i) {
            if (m_activeIndices[i] == index) {
                m_activeIndices[i] = m_activeIndices[--m_activeCount];
                break;
            }
        }

        m_freeSlots[m_freeCount++] = index;
    }

    void WorkerManager::Update(float dt) {
        if (m_activeCount == 0) return;

        // Phase 1: walk ALL moving workers every frame (linear hot-path pass)
        for (int ai = 0; ai < m_activeCount; ++ai) {
            int poolIdx = m_activeIndices[ai];
            Worker& w = m_pool[poolIdx];
            if (w.state != WorkerState_MovingToJob) continue;

            Building* home = (Building*)(size_t)w.homeBuildingIdx;
            Flag* destFlag = home ? home->connectedFlag : NULL;
            if (!destFlag) continue;

            // Pass cold route data from parallel array
            Flag** route = m_routes[poolIdx].flags;
            bool stillMoving = w.Update(dt, m_roadManager, destFlag, route);
            if (!stillMoving) {
                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[Worker] ARRIVED: idx=%d home=%p\n",
                    poolIdx, (void*)home);
                OutputDebugStringA(buf);

                if (home) {
                    home->m_population = 1;
                }
                FreeWorker(poolIdx);
                // Re-process this index after swap (ai stays same)
                --ai;
            }
        }

        // Phase 2: interleaved AI — process 16 workers per frame (round-robin)
        if (m_activeCount == 0) return;
        if (m_updateIndex >= m_activeCount) m_updateIndex = 0;
        int updatesThisFrame = 16;
        if (updatesThisFrame > m_activeCount) updatesThisFrame = m_activeCount;

        for (int k = 0; k < updatesThisFrame; ++k) {
            m_updateIndex = (m_updateIndex + 1) % m_activeCount;
            Worker& w = m_pool[m_activeIndices[m_updateIndex]];

            if (w.stateTimer > 0) {
                w.stateTimer--;
                continue;
            }

            // Future: expand WorkerState AI here for non-transit workers
            // Currently all transit workers become Idle on arrival and are freed
        }
    }

    void WorkerManager::Clear() {
        m_activeCount = 0;
        m_freeCount = 0;
        for (int i = 0; i < MAX_TRANSIT_WORKERS; ++i) {
            m_freeSlots[i] = i;
            memset(m_routes[i].flags, 0, sizeof(m_routes[i].flags));
        }
        m_freeCount = MAX_TRANSIT_WORKERS;
        m_updateIndex = 0;
    }

} // namespace World