#include "stdafx.h"
#include "EntityManager.h"
#include "Components/IComponent.h"

namespace World {

EntityManager::EntityManager()
    : m_nextEntityID(1)
{
}

EntityManager::~EntityManager()
{
    Clear();
}

EntityManager& EntityManager::GetInstance()
{
    static EntityManager instance;
    return instance;
}

Entity EntityManager::CreateEntity()
{
    EntityID entityID;

    if (!m_freeIDs.empty())
    {
        entityID = m_freeIDs.back();
        m_freeIDs.pop_back();
    }
    else
    {
        entityID = m_nextEntityID++;
    }

    return Entity(entityID);
}

void EntityManager::DestroyEntity(EntityID entityID)
{
    if (!EntityExists(entityID))
    {
        return;
    }

    RemoveAllComponents(entityID);
    m_freeIDs.push_back(entityID);
}

bool EntityManager::EntityExists(EntityID entityID) const
{
    // Check if entity is not in free list and is valid
    for (size_t i = 0; i < m_freeIDs.size(); ++i)
    {
        if (m_freeIDs[i] == entityID)
        {
            return false;
        }
    }
    return entityID < m_nextEntityID && entityID != INVALID_ENTITY_ID;
}

void EntityManager::RemoveComponent(EntityID entityID, IComponent* component)
{
    std::map<EntityID, std::vector<IComponent*>>::iterator it = m_components.find(entityID);
    if (it != m_components.end())
    {
        for (std::vector<IComponent*>::iterator cit = it->second.begin(); cit != it->second.end(); ++cit)
        {
            if (*cit == component)
            {
                delete *cit;
                it->second.erase(cit);
                break;
            }
        }
    }
}

void EntityManager::RemoveAllComponents(EntityID entityID)
{
    std::map<EntityID, std::vector<IComponent*>>::iterator it = m_components.find(entityID);
    if (it != m_components.end())
    {
        for (std::vector<IComponent*>::iterator cit = it->second.begin(); cit != it->second.end(); ++cit) {
            delete *cit;
        }
        m_components.erase(it);
    }
}

void EntityManager::GetAllEntityIDs(std::vector<EntityID>& outIDs) const
{
    outIDs.clear();
    for (std::map<EntityID, std::vector<IComponent*>>::const_iterator it = m_components.begin();
         it != m_components.end(); ++it)
    {
        // Проверяем что ID не в списке свободных
        bool isFree = false;
        for (size_t i = 0; i < m_freeIDs.size(); ++i)
        {
            if (m_freeIDs[i] == it->first)
            {
                isFree = true;
                break;
            }
        }
        if (!isFree)
        {
            outIDs.push_back(it->first);
        }
    }
}

void EntityManager::Clear()
{
    for (std::map<EntityID, std::vector<IComponent*>>::iterator it = m_components.begin(); it != m_components.end(); ++it) {
        for (std::vector<IComponent*>::iterator cit = it->second.begin(); cit != it->second.end(); ++cit) {
            delete *cit;
        }
    }
    m_components.clear();
    m_freeIDs.clear();
    m_nextEntityID = 1;
}

} // namespace World
