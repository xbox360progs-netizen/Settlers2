#pragma once
#include "../World/Components/Building.h"
#include <queue>
#include "../Core/PlatformLock.h"

namespace Logic {
    struct BuildingOrder {
        World::BuildingType type;
        int x, y;
    };

    class AICommandQueue {
    public:
        void Push(BuildingOrder order) {
            m_lock.Lock();
            m_queue.push(order);
            m_lock.Unlock();
        }

        bool Pop(BuildingOrder& order) {
            m_lock.Lock();
            if (m_queue.empty()) {
                m_lock.Unlock();
                return false;
            }
            order = m_queue.front();
            m_queue.pop();
            m_lock.Unlock();
            return true;
        }

    private:
        std::queue<BuildingOrder> m_queue;
        PlatformLock m_lock;
    };
}
