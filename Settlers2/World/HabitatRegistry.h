#pragma once
#include <vector>
#include <cstdint>
#include "AnimalHabitat.h"

namespace World {

class HabitatRegistry {
public:
    HabitatRegistry() : m_nextHabitatId(1) {}
    uint32_t Register(AnimalHabitat& habitat);
    void Restore(const AnimalHabitat& habitat);
    void Clear();

    const AnimalHabitat* GetById(uint32_t id) const;
    AnimalHabitat* GetMutableById(uint32_t id);
    const AnimalHabitat* GetByIndex(size_t index) const;
    AnimalHabitat* GetMutableByIndex(size_t index);
    size_t GetCount() const;

private:
    uint32_t m_nextHabitatId;
    std::vector<AnimalHabitat> m_habitats;
};

} // namespace World