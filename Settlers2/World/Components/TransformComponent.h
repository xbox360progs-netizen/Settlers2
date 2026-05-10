#pragma once

#include "IComponent.h"
#include <d3dx9math.h>

namespace World {

struct TransformComponent : public IComponent
{
    static ComponentTypeID GetClassTypeID() { return COMPONENT_TRANSFORM; }

    D3DXVECTOR3 position;
    D3DXVECTOR3 rotation;
    D3DXVECTOR3 scale;

    TransformComponent()
        : position(0.0f, 0.0f, 0.0f)
        , rotation(0.0f, 0.0f, 0.0f)
        , scale(1.0f, 1.0f, 1.0f)
    {
    }

    virtual ComponentTypeID GetTypeID() const { return COMPONENT_TRANSFORM; }

    D3DXMATRIX GetWorldMatrix() const
    {
        D3DXMATRIX world;
        D3DXMatrixIdentity(&world);

        D3DXMATRIX translation;
        D3DXMatrixTranslation(&translation, position.x, position.y, position.z);

        D3DXMATRIX rotationX, rotationY, rotationZ;
        D3DXMatrixRotationX(&rotationX, rotation.x);
        D3DXMatrixRotationY(&rotationY, rotation.y);
        D3DXMatrixRotationZ(&rotationZ, rotation.z);

        D3DXMATRIX scaling;
        D3DXMatrixScaling(&scaling, scale.x, scale.y, scale.z);

        world = scaling * rotationZ * rotationY * rotationX * translation;
        return world;
    }
};

} // namespace World
