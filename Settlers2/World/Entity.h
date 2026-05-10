#pragma once

#include <cstdint>

namespace World {

typedef unsigned int EntityID;

const EntityID INVALID_ENTITY_ID = 0;

class Entity
{
public:
    Entity();
    Entity(EntityID id);

    EntityID GetID() const { return m_id; }
    bool IsValid() const { return m_id != INVALID_ENTITY_ID; }

private:
    EntityID m_id;
};

} // namespace World
