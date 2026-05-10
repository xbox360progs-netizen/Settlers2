#pragma once

#include "Entity.h"
#include <vector>
#include <map>
#include <memory>

namespace World {

class IComponent;

class EntityManager
{
public:
    static EntityManager& GetInstance();

    Entity CreateEntity();
    void DestroyEntity(EntityID entityID);

    bool EntityExists(EntityID entityID) const;

    template<typename T>
    T* AddComponent(EntityID entityID)
    {
        IComponent* component = new T();
        m_components[entityID].push_back(component);
        return static_cast<T*>(component);
    }

    template<typename T>
    T* GetComponent(EntityID entityID)
    {
        std::map<EntityID, std::vector<IComponent*>>::iterator it = m_components.find(entityID);
        if (it != m_components.end())
        {
            for (std::vector<IComponent*>::iterator compIt = it->second.begin(); compIt != it->second.end(); ++compIt)
            {
                if ((*compIt)->GetTypeID() == T::GetClassTypeID())
                {
                    return static_cast<T*>(*compIt);
                }
            }
        }
        return NULL;
    }

    void RemoveComponent(EntityID entityID, IComponent* component);
    void RemoveAllComponents(EntityID entityID);

    void GetAllEntityIDs(std::vector<EntityID>& outIDs) const;

    void Clear();

private:
    EntityManager();
    ~EntityManager();

    EntityManager(const EntityManager&);
    EntityManager& operator=(const EntityManager&);

    EntityID m_nextEntityID;
    std::vector<EntityID> m_freeIDs;
    std::map<EntityID, std::vector<IComponent*>> m_components;
};

} // namespace World
