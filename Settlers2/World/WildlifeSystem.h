#pragma once
#include <vector>
#include <cstdlib>
#include "Animal.h"
#include "AnimalHabitat.h"
#include "HabitatRegistry.h"
#include "AnimalManager.h"

namespace World {

class Map; // forward

class WildlifeSystem {
public:
    WildlifeSystem(Map* map, AnimalManager* animalManager);
    ~WildlifeSystem();

    void Update(float deltaTime, const HabitatRegistry& habitats);

    bool ShouldSpawn(float dt);
    void AddAnimals(const std::vector<Animal>& animals);

    int FindAliveAnimal(int x, int y, int radius, AnimalType type) const {
        return m_animalManager->FindAliveAnimal(x, y, radius, type);
    }
    bool IsAlive(int index) const {
        return m_animalManager->IsAlive(index);
    }
    void TrapAnimal(int index) {
        m_animalManager->TrapAnimal(index);
    }
    void RemoveAnimal(int index) {
        m_animalManager->RemoveAnimal(index);
    }

    int GetAnimalCount() const { return m_animalManager->GetCount(); }
    const std::vector<Animal>& GetAllAnimals() const {
        return m_animalManager->GetAllAnimals();
    }

private:
    void SpawnAtHabitat(const AnimalHabitat& habitat);
    int CountAnimalsAtHabitat(int hx, int hy, AnimalType type) const;

    void MoveAnimals(float dt);

    Map* m_map;
    AnimalManager* m_animalManager;
    float m_spawnTimer;
    float m_dirTimer;

    static const float SPAWN_COOLDOWN;
    static const int MAX_PER_SPAWNER;
};

} // namespace World
