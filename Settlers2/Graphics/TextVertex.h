// Graphics/TextVertex.h
#ifndef TEXTVERTEX_H
#define TEXTVERTEX_H

#pragma once
#include <d3dx9.h>
#include <d3d9.h>

// —труктура вертекса дл€ текста
struct TextVertex
{
    D3DXVECTOR3 pos;  // позици€ в 2D или 3D пространстве
    D3DCOLOR    col;  // цвет
    D3DXVECTOR2 uv;   // UV координаты

    TextVertex() : pos(0,0,0), col(0xFFFFFFFF), uv(0,0) {}
    TextVertex(const D3DXVECTOR3& p, D3DCOLOR c, const D3DXVECTOR2& t)
        : pos(p), col(c), uv(t) {}
};

#endif // TEXTVERTEX_H