#include "stdafx.h"
#include "WildlifeSystem.h"
#include "Map.h"
#include "AnimalHabitat.h"
#include <cstdlib>

namespace World {

const float WildlifeSystem::SPAWN_COOLDOWN = 5.0f;
const int WildlifeSystem::MAX_PER_SPAWNER = 5;

WildlifeSystem::WildlifeSystem(Map* map, AnimalManager* animalManager, AnimalSystem* animalSystem)
    : m_map(map)
    , m_animalManager(animalManager)
    , m_animalSystem(animalSystem)
    , m_spawnTimer(0.0f)
{
}

WildlifeSystem::~WildlifeSystem()
{
}

bool WildlifeSystem::ShouldSpawn(float dt)
{
    m_spawnTimer += dt;
    if (m_spawnTimer >= SPAWN_COOLDOWN) {
        m_spawnTimer = 0.0f;
        return true;
    }
    return false;
}

void WildlifeSystem::Update(float deltaTime, HabitatRegistry& habitats)
{
    m_animalSystem->Update(deltaTime);

    if (ShouldSpawn(deltaTime)) {
        for (size_t i = 0; i < habitats.GetCount(); ++i) {
            AnimalHabitat* hab = habitats.GetMutableByIndex(i);
            if (hab) {
                SpawnAtHabitat(*hab);
            }
        }
    }
}

void WildlifeSystem::SpawnAtHabitat(AnimalHabitat& habitat)
{
    int maxAnimals = (habitat.maxAnimals > 0) ? habitat.maxAnimals : MAX_PER_SPAWNER;
    if (habitat.currentCount >= maxAnimals) return;

    int missing = maxAnimals - habitat.currentCount;
    int chance = missing * 20;
    if (chance > 100) chance = 100;

    if ((rand() % 100) < chance) {
        Vector2i pos;
        pos.x = habitat.center.x;
        pos.y = habitat.center.y;
        m_animalManager->Spawn(habitat.type, pos, habitat.id);
    }
}

}