#pragma once
#include "Entity.h"
#include "Component.h"
#include <vector>
#include <map>

namespace World {

struct IComponentArray {
    virtual ~IComponentArray() {}
};

template<typename T>
struct ComponentArray : IComponentArray {
    std::vector<T> components;
    std::vector<bool> has;
};

uint32_t NextComponentTypeId();

template<typename T>
struct ComponentTypeId {
    static uint32_t GetId() {
        static uint32_t id = NextComponentTypeId();
        return id;
    }
};

class EntityManager {
public:
    EntityManager();
    ~EntityManager();

    Entity CreateEntity();
    void DestroyEntity(Entity entity);
    bool IsAlive(Entity entity) const;

    template<typename T>
    T& AddComponent(Entity entity) {
        ComponentArray<T>* arr = GetOrCreateArray<T>();
        if (entity >= arr->components.size()) {
            arr->components.resize(entity + 1);
            arr->has.resize(entity + 1, false);
        }
        arr->has[entity] = true;
        return arr->components[entity];
    }

    template<typename T>
    T* GetComponent(Entity entity) {
        ComponentArray<T>* arr = GetArray<T>();
        if (!arr || entity >= arr->components.size() || !arr->has[entity])
            return NULL;
        return &arr->components[entity];
    }

    template<typename T>
    const T* GetComponent(Entity entity) const {
        const ComponentArray<T>* arr = GetArray<T>();
        if (!arr || entity >= arr->components.size() || !arr->has[entity])
            return NULL;
        return &arr->components[entity];
    }

    template<typename T>
    bool HasComponent(Entity entity) const {
        const ComponentArray<T>* arr = GetArray<T>();
        return arr && entity < arr->has.size() && arr->has[entity];
    }

    template<typename T>
    void RemoveComponent(Entity entity) {
        ComponentArray<T>* arr = GetArray<T>();
        if (arr && entity < arr->has.size())
            arr->has[entity] = false;
    }

    template<typename T, typename Func>
    void ForEach(Func func) {
        ComponentArray<T>* arr = GetArray<T>();
        if (!arr) return;
        for (Entity e = 0; e < (Entity)arr->has.size(); ++e) {
            if (arr->has[e] && IsAlive(e))
                func(e, arr->components[e]);
        }
    }

    template<typename T1, typename T2, typename Func>
    void ForEach(Func func) {
        ComponentArray<T1>* arr1 = GetArray<T1>();
        ComponentArray<T2>* arr2 = GetArray<T2>();
        if (!arr1 || !arr2) return;
        size_t n = arr1->has.size() > arr2->has.size() ? arr1->has.size() : arr2->has.size();
        for (Entity e = 0; e < (Entity)n; ++e) {
            if (e < arr1->has.size() && arr1->has[e] &&
                e < arr2->has.size() && arr2->has[e] &&
                IsAlive(e))
                func(e, arr1->components[e], arr2->components[e]);
        }
    }

    void GetAllEntities(std::vector<Entity>& out) const;
    void Clear();

private:
    template<typename T>
    ComponentArray<T>* GetOrCreateArray() {
        uint32_t id = ComponentTypeId<T>::GetId();
        std::map<uint32_t, IComponentArray*>::iterator it = m_arrays.find(id);
        if (it != m_arrays.end())
            return static_cast<ComponentArray<T>*>(it->second);
        ComponentArray<T>* arr = new ComponentArray<T>();
        m_arrays[id] = arr;
        return arr;
    }

    template<typename T>
    ComponentArray<T>* GetArray() {
        uint32_t id = ComponentTypeId<T>::GetId();
        std::map<uint32_t, IComponentArray*>::iterator it = m_arrays.find(id);
        return it != m_arrays.end() ? static_cast<ComponentArray<T>*>(it->second) : NULL;
    }

    template<typename T>
    const ComponentArray<T>* GetArray() const {
        uint32_t id = ComponentTypeId<T>::GetId();
        std::map<uint32_t, IComponentArray*>::const_iterator it = m_arrays.find(id);
        return it != m_arrays.end() ? static_cast<const ComponentArray<T>*>(it->second) : NULL;
    }

    static const size_t MAX_ENTITIES = 4096;
    Entity m_nextEntity;
    std::vector<Entity> m_freeEntities;
    bool m_alive[MAX_ENTITIES];
    std::map<uint32_t, IComponentArray*> m_arrays;
};

}
