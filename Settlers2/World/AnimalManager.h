#pragma once
#include <vector>
#include <cstdint>
#include "Entity.h"
#include "Animal.h"
#include "AnimalTypes.h"
#include "../Core/Vector2i.h"

namespace World {

class EntityManager;
class AnimalSystem;

class AnimalManager {
public:
    AnimalManager(EntityManager* entityManager, AnimalSystem* animalSystem);

    // ECS-based spawn
    void Spawn(AnimalType type, const Vector2i& pos);

    // Convert old Animal struct to ECS
    void AddExisting(const Animal& animal);

    // Read animals from ECS (builds cache for backward compat)
    const std::vector<Animal>& GetAllAnimals();
    int GetCount() const;

    // Entity-based query (replaces index-based)
    Entity FindAliveAnimal(int x, int y, int radius, AnimalType type) const;
    Entity FindAliveEntity(int x, int y, int radius, AnimalType type) const;
    bool IsAlive(Entity entity) const;
    void TrapAnimal(int index);
    void RemoveAnimal(Entity entity);

private:
    void RebuildCache();

    EntityManager* m_entityManager;
    AnimalSystem* m_animalSystem;
    std::vector<Animal> m_animalCache;
};

}
