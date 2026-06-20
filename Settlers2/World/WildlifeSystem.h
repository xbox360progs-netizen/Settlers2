#pragma once
#include <vector>
#include <cstdlib>
#include "Animal.h"
#include "AnimalHabitat.h"
#include "HabitatRegistry.h"
#include "AnimalManager.h"
#include "Systems/AnimalSystem.h"

namespace World {

class Map;

class WildlifeSystem {
public:
    WildlifeSystem(Map* map, AnimalManager* animalManager, AnimalSystem* animalSystem);
    ~WildlifeSystem();

    void Update(float deltaTime, HabitatRegistry& habitats);

    bool ShouldSpawn(float dt);

    Entity FindAliveAnimal(int x, int y, int radius, AnimalType type) const {
        return m_animalManager->FindAliveAnimal(x, y, radius, type);
    }
    bool IsAlive(Entity entity) const {
        return m_animalManager->IsAlive(entity);
    }
    void TrapAnimal(int index) {
        m_animalManager->TrapAnimal(index);
    }
    void RemoveAnimal(Entity entity) {
        m_animalManager->RemoveAnimal(entity);
    }

    int GetAnimalCount() const { return m_animalManager->GetCount(); }
    const std::vector<Animal>& GetAllAnimals() {
        return m_animalManager->GetAllAnimals();
    }

private:
    void SpawnAtHabitat(AnimalHabitat& habitat);

    Map* m_map;
    AnimalManager* m_animalManager;
    AnimalSystem* m_animalSystem;
    float m_spawnTimer;

    static const float SPAWN_COOLDOWN;
    static const int MAX_PER_SPAWNER;
};

}