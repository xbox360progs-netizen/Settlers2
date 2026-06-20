#include "stdafx.h"
#include "AnimalManager.h"
#include "EntityManager.h"
#include "Systems/AnimalSystem.h"
#include "HabitatRegistry.h"
#include "AnimalHabitat.h"
#include <cstdlib>

namespace World {

AnimalManager::AnimalManager(EntityManager* entityManager, AnimalSystem* animalSystem)
    : m_entityManager(entityManager)
    , m_animalSystem(animalSystem)
    , m_habitatRegistry(NULL)
{
}

void AnimalManager::Init(HabitatRegistry* habitatRegistry) {
    m_habitatRegistry = habitatRegistry;
}

void AnimalManager::Spawn(AnimalType type, const Vector2i& pos, uint32_t habitatId) {
    m_animalSystem->CreateAnimal(type, pos, habitatId);

    if (m_habitatRegistry && habitatId != 0) {
        AnimalHabitat* hab = m_habitatRegistry->GetMutableById(habitatId);
        if (hab) {
            hab->currentCount++;
        }
    }
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
    ac.habitatId = animal.habitatId;
    ac.stopTimer = animal.stopTimer;

    if (!m_animalSystem->RegisterEntity(entity)) return;

    if (m_habitatRegistry && ac.habitatId != 0) {
        AnimalHabitat* hab = m_habitatRegistry->GetMutableById(ac.habitatId);
        if (hab) {
            hab->currentCount++;
        }
    }
}

const std::vector<Animal>& AnimalManager::GetAllAnimals() {
    m_animalCache.clear();
    m_animalSystem->GetAllAnimals(m_animalCache);
    return m_animalCache;
}

int AnimalManager::GetCount() const {
    return (int)m_animalSystem->GetActiveCount();
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
    Entity targetEntity = m_animalSystem->GetEntityByActiveIdx(index);
    if (targetEntity == INVALID_ENTITY) return;

    AnimalComponent* ac = m_entityManager->GetComponent<AnimalComponent>(targetEntity);
    if (!ac || ac->state == AnimalState_Trapped) return;

    ac->state = AnimalState_Trapped;

    if (m_habitatRegistry && ac->habitatId != 0) {
        AnimalHabitat* hab = m_habitatRegistry->GetMutableById(ac->habitatId);
        if (hab && hab->currentCount > 0) {
            hab->currentCount--;
        }
    }
}

void AnimalManager::RemoveAnimal(Entity entity) {
    AnimalComponent* ac = m_entityManager->GetComponent<AnimalComponent>(entity);
    if (ac && ac->state == AnimalState_Alive) {
        if (m_habitatRegistry && ac->habitatId != 0) {
            AnimalHabitat* hab = m_habitatRegistry->GetMutableById(ac->habitatId);
            if (hab && hab->currentCount > 0) {
                hab->currentCount--;
            }
        }
    }

    m_animalSystem->RemoveAnimal(entity);
}

}