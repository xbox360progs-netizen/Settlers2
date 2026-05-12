#pragma once
#include <d3dx9.h>
#include <d3d9.h>

// Unified vertex structure for all 2D objects (sprites, text, UI)
// 32-byte aligned for Xbox 360 compatibility
struct CommonVertex2D
{
    D3DXVECTOR3 pos;      // Position (x, y, z) - 12 bytes
    D3DXVECTOR2 uv;       // UV coordinates - 8 bytes  
    D3DCOLOR    color;    // Color - 4 bytes
    float padding[2];       // Padding for 32-byte alignment - 8 bytes
    
    CommonVertex2D() : pos(0, 0, 0), uv(0, 0), color(0xFFFFFFFF) {
        padding[0] = 0;
        padding[1] = 0;
    }
    
    CommonVertex2D(const D3DXVECTOR3& p, const D3DXVECTOR2& t, D3DCOLOR c)
        : pos(p), uv(t), color(c) {
        padding[0] = 0;
        padding[1] = 0;
    }
};

static_assert(sizeof(CommonVertex2D) == 32, "CommonVertex2D must be 32 bytes");
