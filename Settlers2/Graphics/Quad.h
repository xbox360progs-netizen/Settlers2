#pragma once
#include <d3d9.h>

class Quad
{
public:
    Quad(LPDIRECT3DDEVICE9 device);
    ~Quad();

    bool Initialize(float width, float height);
    void Render();
    void Shutdown();

private:
    LPDIRECT3DDEVICE9 m_device;
    LPDIRECT3DVERTEXDECLARATION9 m_pVertexDecl;
    float m_width;
    float m_height;
};
