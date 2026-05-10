#include "stdafx.h"
#include "Quad.h"

struct QuadVertex
{
    float x, y, z, w;
    float u, v;
};

Quad::Quad(LPDIRECT3DDEVICE9 device)
    : m_device(device)
    , m_pVertexDecl(nullptr)
    , m_width(0.0f)
    , m_height(0.0f)
{
}

Quad::~Quad()
{
    Shutdown();
}

bool Quad::Initialize(float width, float height)
{
    m_width = width;
    m_height = height;

    // Create vertex declaration matching shader input: float4 Pos : POSITION, float2 UV : TEXCOORD0
    D3DVERTEXELEMENT9 decl[] = {
        {0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };

    if (FAILED(m_device->CreateVertexDeclaration(decl, &m_pVertexDecl))) {
        return false;
    }

    return true;
}

void Quad::Render()
{
    if (!m_device || !m_pVertexDecl) return;

    QuadVertex vertices[4];
    float halfW = m_width * 0.5f;
    float halfH = m_height * 0.5f;

    // Top-left
    vertices[0].x = -halfW; vertices[0].y = -halfH; vertices[0].z = 0.0f; vertices[0].w = 1.0f;
    vertices[0].u = 0.0f; vertices[0].v = 0.0f;

    // Top-right
    vertices[1].x = halfW; vertices[1].y = -halfH; vertices[1].z = 0.0f; vertices[1].w = 1.0f;
    vertices[1].u = 1.0f; vertices[1].v = 0.0f;

    // Bottom-right
    vertices[2].x = halfW; vertices[2].y = halfH; vertices[2].z = 0.0f; vertices[2].w = 1.0f;
    vertices[2].u = 1.0f; vertices[2].v = 1.0f;

    // Bottom-left
    vertices[3].x = -halfW; vertices[3].y = halfH; vertices[3].z = 0.0f; vertices[3].w = 1.0f;
    vertices[3].u = 0.0f; vertices[3].v = 1.0f;

    // Set vertex declaration and draw
    m_device->SetVertexDeclaration(m_pVertexDecl);
    m_device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vertices, sizeof(QuadVertex));
}

void Quad::Shutdown()
{
    if (m_pVertexDecl) {
        m_pVertexDecl->Release();
        m_pVertexDecl = nullptr;
    }
    m_device = nullptr;
}
