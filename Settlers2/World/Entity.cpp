#include "stdafx.h"
#include "Entity.h"

namespace World {

Entity::Entity()
    : m_id(INVALID_ENTITY_ID)
{
}

Entity::Entity(EntityID id)
    : m_id(id)
{
}

} // namespace World
