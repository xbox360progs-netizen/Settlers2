#pragma once
#include <vector>
#include "Animal.h"
#include "ResourceNode.h"
#include "Map.h"

namespace World {

struct SpawnerInfo {
    int x, y;
    AnimalType type;
};

class WildlifeSystem {
public:
    WildlifeSystem(Map* map);
    ~WildlifeSystem();

    void Update(float deltaTime);

    bool ShouldSpawn(float dt);
    int GetSpawnerCount() const { return (int)m_spawners.size(); }
    void ProcessSpawnerRange(int start, int end, std::vector<Animal>& outNewAnimals);
    void AddAnimals(const std::vector<Animal>& animals);

    int FindAliveAnimal(int x, int y, int radius, AnimalType type) const;
    bool IsAlive(int index) const;
    void TrapAnimal(int index);
    void RemoveAnimal(int index);

    int GetAnimalCount() const { return (int)m_animals.size(); }

private:
    void ScanSpawners();
    void SpawnAtSpawner(const SpawnerInfo& spawner);
    int CountAnimalsAtSpawner(int sx, int sy, AnimalType type) const;

    Map* m_map;
    std::vector<Animal> m_animals;
    std::vector<SpawnerInfo> m_spawners;
    float m_spawnTimer;

    static const float SPAWN_COOLDOWN;
    static const int MAX_PER_SPAWNER;
};

} // namespace World
