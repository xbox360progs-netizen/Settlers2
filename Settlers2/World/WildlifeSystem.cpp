#include "stdafx.h"
#include "WildlifeSystem.h"
#include "Map.h"
#include <cstdlib>

namespace World {

const float WildlifeSystem::SPAWN_COOLDOWN = 5.0f;
const int WildlifeSystem::MAX_PER_SPAWNER = 5;
const float DIAG_SPEED = 0.3f;

WildlifeSystem::WildlifeSystem(Map* map, AnimalManager* animalManager)
    : m_map(map)
    , m_animalManager(animalManager)
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
    MoveAnimals(deltaTime);

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
    const std::vector<Animal>& animals = m_animalManager->GetAllAnimals();
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

static void RandomDiagDir(float& vx, float& vy)
{
    int dir = rand() % 4;
    switch (dir) {
        case 0: vx =  DIAG_SPEED; vy = -DIAG_SPEED; break;
        case 1: vx =  DIAG_SPEED; vy =  DIAG_SPEED; break;
        case 2: vx = -DIAG_SPEED; vy =  DIAG_SPEED; break;
        case 3: vx = -DIAG_SPEED; vy = -DIAG_SPEED; break;
    }
}

void WildlifeSystem::MoveAnimals(float dt)
{
    const float MOVE_RANGE = 6.0f;
    const float DIR_CHANGE_INTERVAL = 1.2f;
    const float STOP_DURATION = 0.1f;

    m_dirTimer += dt;
    bool dirChangeTick = false;
    if (m_dirTimer >= DIR_CHANGE_INTERVAL) {
        m_dirTimer = 0.0f;
        dirChangeTick = true;
    }

    std::vector<Animal>& animals = m_animalManager->GetAllAnimals();
    for (size_t i = 0; i < animals.size(); ++i) {
        Animal& a = animals[i];
        if (a.state != AnimalState_Alive) continue;

        if (a.stopTimer > 0.0f) {
            a.vx = 0.0f;
            a.vy = 0.0f;
            a.stopTimer -= dt;
            if (a.stopTimer <= 0.0f) {
                a.stopTimer = 0.0f;
                RandomDiagDir(a.vx, a.vy);
            }
            continue;
        }

        if (dirChangeTick && (rand() % 3) == 0) {
            a.vx = 0.0f;
            a.vy = 0.0f;
            a.stopTimer = STOP_DURATION;
            continue;
        }

        float nx = a.x + a.vx * dt;
        float ny = a.y + a.vy * dt;

        // Don't walk on water
        int destX = (int)(nx + 0.5f);
        int destY = (int)(ny + 0.5f);
        BYTE weight = m_map->GetNodeWeight(destX, destY);
        if (weight == Weight_Deep || weight == Weight_Shallow) {
            a.vx = -a.vx;
            a.vy = -a.vy;
            continue;
        }

        float dxs = nx - (float)a.spawnerX;
        float dys = ny - (float)a.spawnerY;
        if (dxs * dxs + dys * dys <= MOVE_RANGE * MOVE_RANGE) {
            a.x = nx;
            a.y = ny;
        } else {
            a.vx = -a.vx;
            a.vy = -a.vy;
        }
    }
}

} // namespace World
