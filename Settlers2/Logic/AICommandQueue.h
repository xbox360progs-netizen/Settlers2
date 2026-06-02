#pragma once
#include "../World/Components/Building.h"
#include <queue>
#include <xtl.h> // Xbox specific

namespace Logic {
    struct BuildingOrder {
        World::BuildingType type;
        int x, y;
    };

    class AICommandQueue {
    public:
        AICommandQueue() { InitializeCriticalSection(&m_cs); }
        ~AICommandQueue() { DeleteCriticalSection(&m_cs); }

        void Push(BuildingOrder order) {
            EnterCriticalSection(&m_cs);
            m_queue.push(order);
            LeaveCriticalSection(&m_cs);
        }

        bool Pop(BuildingOrder& order) {
            EnterCriticalSection(&m_cs);
            if (m_queue.empty()) {
                LeaveCriticalSection(&m_cs);
                return false;
            }
            order = m_queue.front();
            m_queue.pop();
            LeaveCriticalSection(&m_cs);
            return true;
        }

    private:
        std::queue<BuildingOrder> m_queue;
        CRITICAL_SECTION m_cs;
    };
}
