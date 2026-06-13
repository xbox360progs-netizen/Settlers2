#include "stdafx.h"
#include "AnimalManager.h"
#include "EntityManager.h"
#include "Systems/AnimalSystem.h"
#include <cstdlib>

namespace World {

AnimalManager::AnimalManager(EntityManager* entityManager, AnimalSystem* animalSystem)
    : m_entityManager(entityManager)
    , m_animalSystem(animalSystem)
{
}

void AnimalManager::Spawn(AnimalType type, const Vector2i& pos) {
    m_animalSystem->CreateAnimal(type, pos);
}

void AnimalManager::AddExisting(const Animal& animal) {
    Entity entity = m_entityManager->CreateEntity();

    PositionComponent& pc = m_entityManager->AddComponent<PositionComponent>(entity);
    pc.x = animal.x;
    pc.y = animal.y;

    VelocityComponent& vc = m_entityManager->AddComponent<VelocityComponent>(entity);
    vc.vx = animal.vx;
    vc.vy = animal.vy;

    AnimalComponent& ac = m_entityManager->AddComponent<AnimalComponent>(entity);
    ac.type = animal.type;
    ac.state = animal.state;
    ac.spawnerX = animal.spawnerX;
    ac.spawnerY = animal.spawnerY;
    ac.stopTimer = 0.0f;

}

const std::vector<Animal>& AnimalManager::GetAllAnimals() {
    RebuildCache();
    return m_animalCache;
}

int AnimalManager::GetCount() const {
    return (int)m_animalSystem->GetCount();
}

Entity AnimalManager::FindAliveAnimal(int x, int y, int radius, AnimalType type) const {
    return m_animalSystem->FindAliveAnimal(x, y, radius, type);
}

Entity AnimalManager::FindAliveEntity(int x, int y, int radius, AnimalType type) const {
    return m_animalSystem->FindAliveAnimal(x, y, radius, type);
}

bool AnimalManager::IsAlive(Entity entity) const {
    return m_animalSystem->IsAlive(entity);
}

void AnimalManager::TrapAnimal(int index) {
    RebuildCache();

    if (index < 0 || index >= (int)m_animalCache.size()) return;

    // Find entity matching this cache entry
    Animal& a = m_animalCache[index];
    a.state = AnimalState_Trapped;

    // Update ECS
    std::vector<Entity> entities;
    m_entityManager->GetAllEntities(entities);
    for (size_t i = 0; i < entities.size(); ++i) {
        Entity e = entities[i];
        AnimalComponent* ac = m_entityManager->GetComponent<AnimalComponent>(e);
        if (ac && (int)ac->type == (int)a.type) {
            PositionComponent* pc = m_entityManager->GetComponent<PositionComponent>(e);
            if (pc && (int)pc->x == (int)a.x && (int)pc->y == (int)a.y) {
                ac->state = AnimalState_Trapped;
                return;
            }
        }
    }
}

void AnimalManager::RemoveAnimal(Entity entity) {
    m_animalSystem->RemoveAnimal(entity);
}

void AnimalManager::RebuildCache() {
    m_animalCache.clear();
    m_animalSystem->GetAllAnimals(m_animalCache);
}

}
