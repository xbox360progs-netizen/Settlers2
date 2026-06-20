#include "stdafx.h"
#include "HabitatRegistry.h"

namespace World {

uint32_t HabitatRegistry::Register(AnimalHabitat& habitat) {
    habitat.id = m_nextHabitatId++;
    m_habitats.push_back(habitat);
    return habitat.id;
}

void HabitatRegistry::Restore(const AnimalHabitat& habitat) {
    m_habitats.push_back(habitat);
    if (habitat.id >= m_nextHabitatId) {
        m_nextHabitatId = habitat.id + 1;
    }
}

void HabitatRegistry::Clear() {
    m_habitats.clear();
    m_nextHabitatId = 1;
}

const AnimalHabitat* HabitatRegistry::GetById(uint32_t id) const {
    for (size_t i = 0; i < m_habitats.size(); ++i) {
        if (m_habitats[i].id == id) {
            return &m_habitats[i];
        }
    }
    return nullptr;
}

AnimalHabitat* HabitatRegistry::GetMutableById(uint32_t id) {
    for (size_t i = 0; i < m_habitats.size(); ++i) {
        if (m_habitats[i].id == id) {
            return &m_habitats[i];
        }
    }
    return nullptr;
}

const AnimalHabitat* HabitatRegistry::GetByIndex(size_t index) const {
    if (index >= m_habitats.size()) return nullptr;
    return &m_habitats[index];
}

AnimalHabitat* HabitatRegistry::GetMutableByIndex(size_t index) {
    if (index >= m_habitats.size()) return nullptr;
    return &m_habitats[index];
}

size_t HabitatRegistry::GetCount() const {
    return m_habitats.size();
}

} // namespace World