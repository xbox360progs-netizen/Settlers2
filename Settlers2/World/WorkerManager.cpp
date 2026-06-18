#include "stdafx.h"
#include "WorkerManager.h"

namespace World {

    WorkerManager::WorkerManager()
        : m_warehouse(NULL)
    {}

    WorkerManager::~WorkerManager() {
        Clear();
    }

    void WorkerManager::SpawnWorker(Building* home, float startX, float startY) {
        if (!home) return;
        char buf[256];
        _snprintf(buf, sizeof(buf),
            "[Worker] SpawnWorker: building=%p type=%d at (%d,%d) from (%.1f,%.1f) m_maxPop=%d\n",
            (void*)home, home->type, home->pos.x, home->pos.y, startX, startY, home->m_maxPopulation);
        OutputDebugStringA(buf);
        Worker* w = new Worker(home, startX, startY);
        m_transit.push_back(w);
    }

    void WorkerManager::Update(float dt) {
        if (m_transit.empty()) return;
        for (int i = (int)m_transit.size() - 1; i >= 0; --i) {
            Worker* w = m_transit[i];
            bool stillMoving = w->Update(dt);
            char buf[256];
            _snprintf(buf, sizeof(buf),
                "[Worker] Update: home=%p type=%d pos=(%.1f,%.1f) target=(%d,%d) moving=%d dist=%.2f\n",
                (void*)w->home, w->home ? w->home->type : -1,
                w->wx, w->wy,
                w->home && w->home->connectedFlag ? w->home->connectedFlag->pos.x : 0,
                w->home && w->home->connectedFlag ? w->home->connectedFlag->pos.y : 0,
                stillMoving ? 1 : 0,
                w->home && w->home->connectedFlag ? sqrtf(
                    (w->home->connectedFlag->pos.x - w->wx) * (w->home->connectedFlag->pos.x - w->wx) +
                    (w->home->connectedFlag->pos.y - w->wy) * (w->home->connectedFlag->pos.y - w->wy)) : 0.0f);
            OutputDebugStringA(buf);
            if (!stillMoving) {
                // Worker arrived at building
                if (w->home) {
                    _snprintf(buf, sizeof(buf),
                        "[Worker] ARRIVED: building=%p type=%d pop=%d->1\n",
                        (void*)w->home, w->home->type, w->home->m_population);
                    OutputDebugStringA(buf);
                    w->home->m_population = 1;
                }
                // Add to warehouse specialists if possible
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
