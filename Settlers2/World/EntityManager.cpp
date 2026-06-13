#include "stdafx.h"
#include "EntityManager.h"

namespace World {

uint32_t NextComponentTypeId() {
    static uint32_t counter = 0;
    return counter++;
}

EntityManager::EntityManager()
    : m_nextEntity(1)
{
    for (size_t i = 0; i < MAX_ENTITIES; ++i)
        m_alive[i] = false;
}

EntityManager::~EntityManager() {
    Clear();
}

Entity EntityManager::CreateEntity() {
    Entity entity;
    if (!m_freeEntities.empty()) {
        entity = m_freeEntities.back();
        m_freeEntities.pop_back();
    } else {
        entity = m_nextEntity++;
    }
    if (entity < MAX_ENTITIES)
        m_alive[entity] = true;
    return entity;
}

void EntityManager::DestroyEntity(Entity entity) {
    if (!IsAlive(entity))
        return;
    if (entity < MAX_ENTITIES)
        m_alive[entity] = false;
    m_freeEntities.push_back(entity);
}

bool EntityManager::IsAlive(Entity entity) const {
    return entity < MAX_ENTITIES && m_alive[entity];
}

void EntityManager::GetAllEntities(std::vector<Entity>& out) const {
    out.clear();
    for (Entity e = 0; e < m_nextEntity && e < (Entity)MAX_ENTITIES; ++e) {
        if (m_alive[e])
            out.push_back(e);
    }
}

void EntityManager::Clear() {
    std::map<uint32_t, IComponentArray*>::iterator it;
    for (it = m_arrays.begin(); it != m_arrays.end(); ++it)
        delete it->second;
    m_arrays.clear();
    m_freeEntities.clear();
    m_nextEntity = 1;
    for (size_t i = 0; i < MAX_ENTITIES; ++i)
        m_alive[i] = false;
}

}
