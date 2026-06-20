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
class HabitatRegistry;

class AnimalManager {
public:
    AnimalManager(EntityManager* entityManager, AnimalSystem* animalSystem);

    void Init(HabitatRegistry* habitatRegistry);

    void Spawn(AnimalType type, const Vector2i& pos, uint32_t habitatId);

    void AddExisting(const Animal& animal);

    const std::vector<Animal>& GetAllAnimals();
    int GetCount() const;

    Entity FindAliveAnimal(int x, int y, int radius, AnimalType type) const;
    Entity FindAliveEntity(int x, int y, int radius, AnimalType type) const;
    bool IsAlive(Entity entity) const;
    void TrapAnimal(int index);
    void RemoveAnimal(Entity entity);

private:
    EntityManager* m_entityManager;
    AnimalSystem* m_animalSystem;
    HabitatRegistry* m_habitatRegistry;
    std::vector<Animal> m_animalCache;
};

}