#pragma once

#include "IComponent.h"
#include <d3d9.h>
#include <d3dx9math.h>

namespace World {

struct SpriteComponent : public IComponent
{
    static ComponentTypeID GetClassTypeID() { return COMPONENT_SPRITE; }

    IDirect3DTexture9* texture;
    D3DXVECTOR2 uvMin;
    D3DXVECTOR2 uvMax;
    D3DXVECTOR2 size;
    D3DCOLOR color;
    bool visible;

    SpriteComponent()
        : texture(NULL)
        , uvMin(0.0f, 0.0f)
        , uvMax(1.0f, 1.0f)
        , size(1.0f, 1.0f)
        , color(0xFFFFFFFF)
        , visible(true)
    {
    }

    virtual ComponentTypeID GetTypeID() const { return COMPONENT_SPRITE; }
};

} // namespace World
