#include "stdafx.h"
#include "WildlifeSystem.h"
#include "Map.h"
#include <cstdlib>

namespace World {

const float WildlifeSystem::SPAWN_COOLDOWN = 5.0f;
const int WildlifeSystem::MAX_PER_SPAWNER = 5;

WildlifeSystem::WildlifeSystem(Map* map, AnimalManager* animalManager, AnimalSystem* animalSystem)
    : m_map(map)
    , m_animalManager(animalManager)
    , m_animalSystem(animalSystem)
    , m_spawnTimer(0.0f)
    , m_dirTimer(0.0f)
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

void WildlifeSystem::Update(float deltaTime, const HabitatRegistry& habitats)
{
    // ECS updates animal movement
    m_animalSystem->Update(deltaTime);

    if (ShouldSpawn(deltaTime)) {
        for (size_t i = 0; i < habitats.GetCount(); ++i) {
            const AnimalHabitat* hab = habitats.GetById((uint32_t)(i + 1));
            if (hab) {
                SpawnAtHabitat(*hab);
            }
        }
    }
}

void WildlifeSystem::SpawnAtHabitat(const AnimalHabitat& habitat)
{
    int maxAnimals = (habitat.maxAnimals > 0) ? habitat.maxAnimals : MAX_PER_SPAWNER;
    int count = CountAnimalsAtHabitat(habitat.center.x, habitat.center.y, habitat.type);
    if (count >= maxAnimals) return;

    int missing = maxAnimals - count;
    int chance = missing * 20;
    if (chance > 100) chance = 100;

    if ((rand() % 100) < chance) {
        Vector2i pos;
        pos.x = habitat.center.x;
        pos.y = habitat.center.y;
        m_animalManager->Spawn(habitat.type, pos);
    }
}

int WildlifeSystem::CountAnimalsAtHabitat(int hx, int hy, AnimalType type) const
{
    std::vector<Animal> animals;
    m_animalSystem->GetAllAnimals(animals);
    int count = 0;
    for (size_t i = 0; i < animals.size(); ++i) {
        if (animals[i].spawnerX == hx && animals[i].spawnerY == hy && animals[i].type == type) {
            ++count;
        }
    }
    return count;
}

void WildlifeSystem::AddAnimals(const std::vector<Animal>& animals)
{
    for (size_t i = 0; i < animals.size(); ++i) {
        m_animalManager->AddExisting(animals[i]);
    }
}

}
