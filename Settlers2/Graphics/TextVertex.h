#pragma once
#include <d3dx9.h>
#include <d3d9.h>

// Text vertex structure (32-byte aligned for Xbox 360)
struct TextVertex
{
    D3DXVECTOR3 pos;  // Position (x, y, z)
    D3DCOLOR    col;  // Color
    D3DXVECTOR2 uv;   // UV coordinates
    float padding[2]; // Padding for 32-byte alignment

    TextVertex() : pos(0, 0, 0), col(0xFFFFFFFF), uv(0, 0) {
        padding[0] = 0;
        padding[1] = 0;
    }

    TextVertex(const D3DXVECTOR3& p, D3DCOLOR c, const D3DXVECTOR2& t)
        : pos(p), col(c), uv(t) {
        padding[0] = 0;
        padding[1] = 0;
    }
};
