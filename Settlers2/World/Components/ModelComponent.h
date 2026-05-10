#pragma once

#include "IComponent.h"

namespace World {

struct ModelComponent : public IComponent
{
    static ComponentTypeID GetClassTypeID() { return COMPONENT_MODEL; }

    // Заглушка для будущей 3D поддержки
    void* meshData;
    void* materialData;
    bool visible;

    ModelComponent()
        : meshData(NULL)
        , materialData(NULL)
        , visible(true)
    {
    }

    virtual ComponentTypeID GetTypeID() const { return COMPONENT_MODEL; }
};

} // namespace World
