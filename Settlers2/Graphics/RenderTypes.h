#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <type_traits>

#pragma pack(push, 1)
struct SpriteVertex {
    float x, y, z;
    float u, v;
    DWORD color;
    float padding[2];
};
#pragma pack(pop)

#define SPRITE_VERTEX_STRIDE 32
static_assert(sizeof(SpriteVertex) == SPRITE_VERTEX_STRIDE, "SpriteVertex must be 32 bytes for Xbox 360!");

namespace Graphics {

struct RenderCommand {
    float x, y;
    float width, height;

    float u0, v0;
    float u1, v1;

    DWORD color;

    WORD textureID;
    WORD shaderID;

    BYTE blendMode;
    BYTE layer;

    WORD depth;

    unsigned __int64 sortKey;
};

static_assert(std::is_pod<RenderCommand>::value,
    "RenderCommand must be POD");

static_assert(std::is_trivial<RenderCommand>::value,
    "RenderCommand must be trivial");

static_assert(std::is_standard_layout<RenderCommand>::value,
    "RenderCommand must be standard layout");

inline unsigned __int64 BuildSortKey(BYTE layer, BYTE blend, WORD shader, WORD texture, WORD depth) {
    return ((unsigned __int64)layer << 56)
         | ((unsigned __int64)blend << 48)
         | ((unsigned __int64)shader << 32)
         | ((unsigned __int64)texture << 16)
         | ((unsigned __int64)depth);
}

}

struct RenderStateBlock {
    DWORD zEnable;
    DWORD alphaBlendEnable;
    DWORD srcBlend;
    DWORD destBlend;
    DWORD cullMode;

    RenderStateBlock()
        : zEnable(D3DZB_FALSE), alphaBlendEnable(FALSE),
          srcBlend(D3DBLEND_SRCALPHA), destBlend(D3DBLEND_INVSRCALPHA),
          cullMode(D3DCULL_NONE) {}
};
