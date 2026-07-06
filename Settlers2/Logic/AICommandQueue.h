#pragma once
#include "../World/Components/Building.h"
#include "../Platform/Lock.h"
#include <queue>

namespace Logic {
    struct BuildingOrder {
        World::BuildingType type;
        int x, y;
    };

    class AICommandQueue {
    public:
        AICommandQueue() {}
        ~AICommandQueue() {}

        void Push(BuildingOrder order) {
            m_lock.Acquire();
            m_queue.push(order);
            m_lock.Release();
        }

        bool Pop(BuildingOrder& order) {
            m_lock.Acquire();
            if (m_queue.empty()) {
                m_lock.Release();
                return false;
            }
            order = m_queue.front();
            m_queue.pop();
            m_lock.Release();
            return true;
        }

    private:
        std::queue<BuildingOrder> m_queue;
        Platform::Lock m_lock;
    };
}
