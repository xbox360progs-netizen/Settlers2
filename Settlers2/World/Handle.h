#pragma once
#include <vector>
#include <stdint.h>

namespace World {

    template<typename T>
    struct Handle {
        uint32_t index;
        uint32_t generation;

        static const uint32_t INVALID_INDEX = 0xFFFFFFFF;

        Handle() : index(INVALID_INDEX), generation(0) {}
        Handle(uint32_t idx, uint32_t gen) : index(idx), generation(gen) {}

        bool IsValid() const { return index != INVALID_INDEX; }

        bool operator==(const Handle& other) const {
            return index == other.index && generation == other.generation;
        }
        bool operator!=(const Handle& other) const { return !(*this == other); }
    };

    class HandleRegistry {
    public:
        template<typename T>
        Handle<T> Register(T* ptr) {
            uint32_t idx;

            if (!m_freeList.empty()) {
                idx = m_freeList.back();
                m_freeList.pop_back();
                Slot& slot = m_slots[idx];
                slot.generation++;
                slot.ptr = ptr;
                slot.alive = true;
                return Handle<T>(idx, slot.generation);
            }

            idx = (uint32_t)m_slots.size();
            Slot s;
            s.ptr = ptr;
            s.generation = 1;
            s.alive = true;
            m_slots.push_back(s);
            return Handle<T>(idx, 1);
        }

        template<typename T>
        void Unregister(Handle<T> h) {
            if (!h.IsValid()) return;
            if (h.index >= m_slots.size()) return;

            Slot& slot = m_slots[h.index];
            if (!slot.alive) return;
            if (slot.generation != h.generation) return;

            slot.alive = false;
            slot.ptr = NULL;
            m_freeList.push_back(h.index);
        }

        template<typename T>
        T* Resolve(Handle<T> h) const {
            if (!h.IsValid()) return NULL;
            if (h.index >= m_slots.size()) return NULL;

            const Slot& slot = m_slots[h.index];
            if (!slot.alive) return NULL;
            if (slot.generation != h.generation) return NULL;

            return static_cast<T*>(slot.ptr);
        }

        // Unsafe resolve by index only — bypasses generation check.
        // Caller must guarantee the slot is alive (index not recycled or stale).
        void* UnsafeResolveByIndex(uint32_t idx) const {
            if (idx >= m_slots.size()) return NULL;
            if (!m_slots[idx].alive) return NULL;
            return m_slots[idx].ptr;
        }

        template<typename T>
        Handle<T> FindHandle(T* ptr) const {
            for (uint32_t i = 0; i < (uint32_t)m_slots.size(); ++i) {
                if (m_slots[i].alive && m_slots[i].ptr == ptr)
                    return Handle<T>(i, m_slots[i].generation);
            }
            return Handle<T>();
        }

        void Clear() {
            m_slots.clear();
            m_freeList.clear();
        }

    private:
        struct Slot {
            void* ptr;
            uint32_t generation;
            bool alive;
        };

        std::vector<Slot> m_slots;
        std::vector<uint32_t> m_freeList;
    };

}
