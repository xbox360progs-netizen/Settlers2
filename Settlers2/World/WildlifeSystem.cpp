#include "stdafx.h"
#include "WildlifeSystem.h"
#include "Map.h"
#include <cstdlib>

namespace World {

const float WildlifeSystem::SPAWN_COOLDOWN = 5.0f;
const int WildlifeSystem::MAX_PER_SPAWNER = 5;

WildlifeSystem::WildlifeSystem(Map* map)
    : m_map(map)
    , m_spawnTimer(0.0f)
{
    ScanSpawners();
}

WildlifeSystem::~WildlifeSystem()
{
}

void WildlifeSystem::ScanSpawners()
{
    m_spawners.clear();
    if (!m_map) return;

    int layerWidth = m_map->GetWidth() * 2;
    int layerHeight = m_map->GetHeight() * 4;

    for (int y = 0; y < layerHeight; ++y) {
        for (int x = 0; x < layerWidth; ++x) {
            const ResourceNode& node = m_map->GetResourceNode(x, y);
            AnimalType animalType;
            bool isSpawner = true;
            switch (node.type) {
                case ResourceType_WildlifeSpawner_Deer:   animalType = AnimalType_Deer; break;
                case ResourceType_WildlifeSpawner_Rabbit: animalType = AnimalType_Rabbit; break;
                case ResourceType_WildlifeSpawner_Crocodile: animalType = AnimalType_Crocodile; break;
                case ResourceType_WildlifeSpawner_Snake:  animalType = AnimalType_Snake; break;
                default: isSpawner = false; break;
            }
            if (isSpawner && node.amount > 0) {
                SpawnerInfo si;
                si.x = x;
                si.y = y;
                si.type = animalType;
                m_spawners.push_back(si);
            }
        }
    }
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

void WildlifeSystem::ProcessSpawnerRange(int start, int end, std::vector<Animal>& outNewAnimals)
{
    for (int i = start; i < end; ++i) {
        const SpawnerInfo& spawner = m_spawners[i];
        int count = CountAnimalsAtSpawner(spawner.x, spawner.y, spawner.type);
        if (count >= MAX_PER_SPAWNER) continue;

        int missing = MAX_PER_SPAWNER - count;
        int chance = missing * 20;
        if (chance > 100) chance = 100;

        if ((rand() % 100) < chance) {
            Animal a;
            a.type = spawner.type;
            a.state = AnimalState_Alive;
            a.x = spawner.x + (rand() % 7) - 3;
            a.y = spawner.y + (rand() % 7) - 3;
            a.spawnerX = spawner.x;
            a.spawnerY = spawner.y;
            outNewAnimals.push_back(a);
        }
    }
}

void WildlifeSystem::AddAnimals(const std::vector<Animal>& animals)
{
    m_animals.insert(m_animals.end(), animals.begin(), animals.end());
}

void WildlifeSystem::Update(float deltaTime)
{
    if (!m_map) return;

    m_spawnTimer += deltaTime;
    if (m_spawnTimer >= SPAWN_COOLDOWN) {
        m_spawnTimer = 0.0f;
        for (size_t i = 0; i < m_spawners.size(); ++i) {
            SpawnAtSpawner(m_spawners[i]);
        }
    }
}

void WildlifeSystem::SpawnAtSpawner(const SpawnerInfo& spawner)
{
    int count = CountAnimalsAtSpawner(spawner.x, spawner.y, spawner.type);
    if (count >= MAX_PER_SPAWNER) return;

    int missing = MAX_PER_SPAWNER - count;
    int chance = missing * 20;
    if (chance > 100) chance = 100;

    if ((rand() % 100) < chance) {
        Animal a;
        a.type = spawner.type;
        a.state = AnimalState_Alive;
        a.x = spawner.x + (rand() % 7) - 3;
        a.y = spawner.y + (rand() % 7) - 3;
        a.spawnerX = spawner.x;
        a.spawnerY = spawner.y;
        m_animals.push_back(a);
    }
}

int WildlifeSystem::CountAnimalsAtSpawner(int sx, int sy, AnimalType type) const
{
    int count = 0;
    for (size_t i = 0; i < m_animals.size(); ++i) {
        if (m_animals[i].spawnerX == sx && m_animals[i].spawnerY == sy && m_animals[i].type == type) {
            ++count;
        }
    }
    return count;
}

int WildlifeSystem::FindAliveAnimal(int x, int y, int radius, AnimalType type) const
{
    for (size_t i = 0; i < m_animals.size(); ++i) {
        const Animal& a = m_animals[i];
        if (a.state == AnimalState_Alive && a.type == type) {
            int dx = a.x - x;
            int dy = a.y - y;
            if (dx * dx + dy * dy <= radius * radius) {
                return (int)i;
            }
        }
    }
    return -1;
}

void WildlifeSystem::TrapAnimal(int index)
{
    if (index >= 0 && index < (int)m_animals.size()) {
        m_animals[index].state = AnimalState_Trapped;
    }
}

void WildlifeSystem::RemoveAnimal(int index)
{
    if (index >= 0 && index < (int)m_animals.size()) {
        m_animals.erase(m_animals.begin() + index);
    }
}

} // namespace World
