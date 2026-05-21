#pragma once
#include <d3d9.h>
#include <d3dx9.h>

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

    volatile long status;

    RenderCommand()
        : x(0), y(0), width(0), height(0),
          u0(0), v0(0), u1(1), v1(1),
          color(0xFFFFFFFF),
          textureID(0), shaderID(0),
          blendMode(0), layer(0),
          depth(0), sortKey(0), status(0) {}
};

inline unsigned __int64 BuildSortKey(BYTE layer, BYTE blend, WORD shader, WORD texture, WORD depth) {
    return ((unsigned __int64)layer << 56)
         | ((unsigned __int64)blend << 48)
         | ((unsigned __int64)shader << 32)
         | ((unsigned __int64)texture << 16)
         | ((unsigned __int64)depth);
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
