#include "stdafx.h"
#include "SpriteShadow.h"
#include "Renderer.h"
#include "Texture.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

SpriteShadowSystem::SpriteShadowSystem()
    : m_pDevice(NULL)
    , m_enabled(true)
    , m_defaultColor(0x40000000)
{
}

SpriteShadowSystem::~SpriteShadowSystem() {
    Shutdown();
}

void SpriteShadowSystem::Initialize(IDirect3DDevice9* pDevice) {
    m_pDevice = pDevice;
    OutputDebugStringA("[SpriteShadowSystem] Initialized\n");
}

void SpriteShadowSystem::Shutdown() {
    m_shadows.clear();
    m_pDevice = NULL;
}

void SpriteShadowSystem::BeginFrame() {
    m_shadows.clear();
}

void SpriteShadowSystem::EndFrame() {
}

void SpriteShadowSystem::AddSpriteShadow(const SpriteShadowData& shadow) {
    m_shadows.push_back(shadow);
}

void SpriteShadowSystem::ClearShadowList() {
    m_shadows.clear();
}

void SpriteShadowSystem::Render() {
    if (!m_enabled || m_shadows.empty() || !m_pDevice) return;

    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    for (size_t i = 0; i < m_shadows.size(); i++) {
        const SpriteShadowData& s = m_shadows[i];

        float sx = s.x + s.offsetX;
        float sy = s.y + s.offsetY;
        float sw = s.width;
        float sh = s.height;
        DWORD color = s.color != 0 ? s.color : m_defaultColor;

        SpriteVertex v[4];

        v[0].x = sx;          v[0].y = sy;          v[0].z = 0.0f;
        v[0].u = 0.0f;        v[0].v = 0.0f;
        v[0].color = color;
        v[0].padding[0] = 0; v[0].padding[1] = 0;

        v[1].x = sx + sw;     v[1].y = sy;          v[1].z = 0.0f;
        v[1].u = 1.0f;        v[1].v = 0.0f;
        v[1].color = color;
        v[1].padding[0] = 0; v[1].padding[1] = 0;

        v[2].x = sx;          v[2].y = sy + sh;     v[2].z = 0.0f;
        v[2].u = 0.0f;        v[2].v = 1.0f;
        v[2].color = color;
        v[2].padding[0] = 0; v[2].padding[1] = 0;

        v[3].x = sx + sw;     v[3].y = sy + sh;     v[3].z = 0.0f;
        v[3].u = 1.0f;        v[3].v = 1.0f;
        v[3].color = color;
        v[3].padding[0] = 0; v[3].padding[1] = 0;

        m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(SpriteVertex));
    }

    m_pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}

}
