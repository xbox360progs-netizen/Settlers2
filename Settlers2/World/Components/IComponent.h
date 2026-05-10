#pragma once

namespace World {

enum ComponentTypeID
{
    COMPONENT_TRANSFORM,
    COMPONENT_SPRITE,
    COMPONENT_MODEL,
    COMPONENT_COUNT
};

class IComponent
{
public:
    virtual ~IComponent() {}

    virtual void Update(float deltaTime) {}
    virtual ComponentTypeID GetTypeID() const = 0;
};

} // namespace World
