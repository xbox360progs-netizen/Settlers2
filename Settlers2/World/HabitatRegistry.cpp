#include "stdafx.h"
#include "HabitatRegistry.h"
namespace World {

uint32_t HabitatRegistry::Register(World::AnimalHabitat& habitat) {
    habitat.id = m_nextHabitatId++;
    m_habitats.push_back(habitat);
    return habitat.id;
}

void HabitatRegistry::Restore(const World::AnimalHabitat& habitat) {
    m_habitats.push_back(habitat);
    if (habitat.id >= m_nextHabitatId) {
        m_nextHabitatId = habitat.id + 1;
    }
}

void HabitatRegistry::Clear() {
    m_habitats.clear();
    m_nextHabitatId = 1;
}

const World::AnimalHabitat* HabitatRegistry::GetById(uint32_t id) const {
    for (std::vector<World::AnimalHabitat>::const_iterator it = m_habitats.begin(); it != m_habitats.end(); ++it) {
    const World::AnimalHabitat& hab = *it;
    if (hab.id == id) {
        return &hab;
    }
}
    return nullptr;
}

const World::AnimalHabitat* HabitatRegistry::GetByIndex(size_t index) const {
    if (index >= m_habitats.size()) return nullptr;
    return &m_habitats[index];
}

size_t HabitatRegistry::GetCount() const {
    return m_habitats.size();
}

} // namespace World