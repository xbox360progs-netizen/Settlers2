#pragma once
#include <d3dx9.h>
#include <d3d9.h>

// Text vertex structure (32-byte aligned for Xbox 360)
// Compatible with SpriteVertex structure for unified rendering
struct TextVertex
{
    D3DXVECTOR3 pos;  // Position (x, y, z) - matches SpriteVertex
    D3DXVECTOR2 uv;   // UV coordinates - matches SpriteVertex order
    D3DCOLOR    col;  // Color - matches SpriteVertex order
    float padding[2]; // Padding for 32-byte alignment

    TextVertex() : pos(0, 0, 0), uv(0, 0), col(0xFFFFFFFF) {
        padding[0] = 0;
        padding[1] = 0;
    }

    TextVertex(const D3DXVECTOR3& p, D3DCOLOR c, const D3DXVECTOR2& t)
        : pos(p), uv(t), col(c) {
        padding[0] = 0;
        padding[1] = 0;
    }
};
