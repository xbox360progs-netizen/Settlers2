#pragma once
#include <vector>
#include <cstdint>
#include "Animal.h"
#include "../Core/Vector2i.h"

namespace World {

class AnimalManager {
public:
    AnimalManager();

    void Spawn(AnimalType type, const Vector2i& pos);
    void AddExisting(const Animal& animal);

    const std::vector<Animal>& GetAllAnimals() const { return m_animals; }
    std::vector<Animal>& GetAllAnimals() { return m_animals; }
    int GetCount() const { return (int)m_animals.size(); }

    int FindAliveAnimal(int x, int y, int radius, AnimalType type) const;
    bool IsAlive(int index) const;
    void TrapAnimal(int index);
    void RemoveAnimal(int index);

private:
    std::vector<Animal> m_animals;
};

} // namespace World
