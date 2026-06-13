// HabitatRegistry.h

#pragma once
#include <vector>
#include <cstdint>
#include "AnimalHabitat.h"

namespace World {

class HabitatRegistry {
public:
	HabitatRegistry() : m_nextHabitatId(1) {}
    uint32_t Register(World::AnimalHabitat& habitat);
    void Restore(const World::AnimalHabitat& habitat);
    void Clear();

    const World::AnimalHabitat* GetById(uint32_t id) const;
    const World::AnimalHabitat* GetByIndex(size_t index) const;
    size_t GetCount() const;

private:
    uint32_t m_nextHabitatId;
    std::vector<World::AnimalHabitat> m_habitats;
};

} // namespace World