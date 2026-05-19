#include "stdafx.h"
#include "ShadowSystem.h"
#include "Light.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

ShadowSystem::ShadowSystem()
    : m_pDevice(NULL), m_shadowMapSize(1024), m_shadowBias(0.001f),
      m_debugDraw(false), m_cascadeCount(1) {
}

ShadowSystem::~ShadowSystem() {
    Shutdown();
}

void ShadowSystem::Initialize(LPDIRECT3DDEVICE9 pDevice, int shadowMapSize) {
    m_pDevice = pDevice;
    m_shadowMapSize = shadowMapSize;

    CreateDirectionalShadowMap(m_shadowMapSize);

    char buf[256];
    sprintf(buf, "[ShadowSystem] Initialized with shadow map size %d\n", m_shadowMapSize);
    OutputDebugStringA(buf);
}

void ShadowSystem::Shutdown() {
    for (size_t i = 0; i < m_shadowMaps.size(); i++) {
        DestroyShadowMap(m_shadowMaps[i]);
    }
    m_shadowMaps.clear();
}

void ShadowSystem::CreateShadowMapInternal(int width, int height, ShadowMapData& outData) {
    if (!m_pDevice) return;

    ZeroMemory(&outData, sizeof(outData));
    outData.isDirectional = false;

    HRESULT hr = m_pDevice->CreateTexture(
        width, height, 1,
        D3DUSAGE_RENDERTARGET,
        D3DFMT_R32F,
        D3DPOOL_DEFAULT,
        &outData.pTexture,
        NULL
    );

    if (FAILED(hr)) {
        OutputDebugStringA("[ShadowSystem] ERROR: Failed to create shadow map texture\n");
        return;
    }

    hr = outData.pTexture->GetSurfaceLevel(0, &outData.pSurface);
    if (FAILED(hr)) {
        OutputDebugStringA("[ShadowSystem] ERROR: Failed to get shadow map surface\n");
        return;
    }

    hr = m_pDevice->CreateDepthStencilSurface(
        width, height,
        D3DFMT_D16,
        D3DMULTISAMPLE_NONE,
        0,
        TRUE,
        &outData.pDepthSurface,
        NULL
    );

    if (FAILED(hr)) {
        OutputDebugStringA("[ShadowSystem] ERROR: Failed to create shadow map depth surface\n");
    }
}

void ShadowSystem::DestroyShadowMap(ShadowMapData& data) {
    if (data.pDepthSurface) {
        data.pDepthSurface->Release();
        data.pDepthSurface = NULL;
    }
    if (data.pSurface) {
        data.pSurface->Release();
        data.pSurface = NULL;
    }
    if (data.pTexture) {
        data.pTexture->Release();
        data.pTexture = NULL;
    }
    ZeroMemory(&data, sizeof(data));
}

void ShadowSystem::CreateDirectionalShadowMap(int size) {
    ShadowMapData data;
    CreateShadowMapInternal(size, size, data);
    data.isDirectional = true;
    m_shadowMaps.push_back(data);

    D3DXMatrixIdentity(&data.viewMatrix);
    D3DXMatrixIdentity(&data.projectionMatrix);
    D3DXMatrixIdentity(&data.viewProjectionMatrix);

    char buf[256];
    sprintf(buf, "[ShadowSystem] Created directional shadow map (index %d)\n", (int)m_shadowMaps.size() - 1);
    OutputDebugStringA(buf);
}

void ShadowSystem::CreatePointLightShadowMap(Light* pLight, int size) {
    if (!pLight) return;

    ShadowMapData data;
    CreateShadowMapInternal(size, size, data);
    data.isDirectional = false;
    data.position = pLight->GetPosition();
    data.radius = pLight->GetRadius();
    m_shadowMaps.push_back(data);

    char buf[256];
    sprintf(buf, "[ShadowSystem] Created point light shadow map for %s (index %d)\n",
            pLight->GetName(), (int)m_shadowMaps.size() - 1);
    OutputDebugStringA(buf);
}

void ShadowSystem::CreateSpotLightShadowMap(Light* pLight, int size) {
    if (!pLight) return;

    ShadowMapData data;
    CreateShadowMapInternal(size, size, data);
    data.isDirectional = false;
    data.position = pLight->GetPosition();
    data.direction = pLight->GetDirection();
    data.radius = pLight->GetRadius();
    m_shadowMaps.push_back(data);

    char buf[256];
    sprintf(buf, "[ShadowSystem] Created spot light shadow map for %s (index %d)\n",
            pLight->GetName(), (int)m_shadowMaps.size() - 1);
    OutputDebugStringA(buf);
}

void ShadowSystem::SetDirectionalLight(const D3DXVECTOR3& direction, const D3DXVECTOR3& lightPos) {
    if (m_shadowMaps.empty()) return;

    UpdateDirectionalShadowMatrices(0, direction);

    if (m_shadowMaps[0].pSurface && m_shadowMaps[0].pDepthSurface) {
        m_pDevice->SetRenderTarget(0, m_shadowMaps[0].pSurface);
        m_pDevice->SetDepthStencilSurface(m_shadowMaps[0].pDepthSurface);
    }
}

void ShadowSystem::UpdateDirectionalShadowMatrices(int index, const D3DXVECTOR3& lightDir) {
    if (index >= (int)m_shadowMaps.size()) return;

    D3DXVECTOR3 lightPos = -lightDir * 100.0f;
    D3DXVECTOR3 target(0, 0, 0);
    D3DXVECTOR3 up(0, 1, 0);

    D3DXMatrixLookAtLH(&m_shadowMaps[index].viewMatrix, &lightPos, &target, &up);

    D3DXMatrixOrthoLH(&m_shadowMaps[index].projectionMatrix, 50.0f, 50.0f, 0.1f, 500.0f);

    m_shadowMaps[index].viewProjectionMatrix = m_shadowMaps[index].viewMatrix * m_shadowMaps[index].projectionMatrix;
}

void ShadowSystem::UpdatePointShadowMatrices(int index, const D3DXVECTOR3& lightPos) {
    if (index >= (int)m_shadowMaps.size()) return;

    D3DXVECTOR3 targets[6] = {
        D3DXVECTOR3(1, 0, 0), D3DXVECTOR3(-1, 0, 0),
        D3DXVECTOR3(0, 1, 0), D3DXVECTOR3(0, -1, 0),
        D3DXVECTOR3(0, 0, 1), D3DXVECTOR3(0, 0, -1)
    };

    D3DXVECTOR3 ups[6] = {
        D3DXVECTOR3(0, 1, 0), D3DXVECTOR3(0, 1, 0),
        D3DXVECTOR3(0, 0, -1), D3DXVECTOR3(0, 0, 1),
        D3DXVECTOR3(0, 1, 0), D3DXVECTOR3(0, 1, 0)
    };

    for (int i = 0; i < 6; i++) {
        D3DXMatrixLookAtLH(&m_shadowMaps[index].viewMatrix, &lightPos, &targets[i], &ups[i]);
        D3DXMatrixPerspectiveFovLH(&m_shadowMaps[index].projectionMatrix, D3DX_PI * 0.5f, 1.0f, 0.1f, m_shadowMaps[index].radius);
    }
}

void ShadowSystem::UpdateSpotShadowMatrices(int index, const D3DXVECTOR3& lightPos, const D3DXVECTOR3& lightDir) {
    if (index >= (int)m_shadowMaps.size()) return;

    D3DXVECTOR3 target = lightPos + lightDir * 10.0f;
    D3DXVECTOR3 up(0, 1, 0);

    D3DXMatrixLookAtLH(&m_shadowMaps[index].viewMatrix, &lightPos, &target, &up);

    D3DXMatrixPerspectiveFovLH(&m_shadowMaps[index].projectionMatrix, D3DX_PI * 0.25f, 1.0f, 0.1f, m_shadowMaps[index].radius);

    m_shadowMaps[index].viewProjectionMatrix = m_shadowMaps[index].viewMatrix * m_shadowMaps[index].projectionMatrix;
}

void ShadowSystem::BeginShadowPass(Light* pLight) {
    if (!m_pDevice) return;

    int shadowIndex = pLight->GetShadowMapIndex();
    if (shadowIndex < 0 || shadowIndex >= (int)m_shadowMaps.size()) {
        return;
    }

    m_pDevice->SetRenderTarget(0, m_shadowMaps[shadowIndex].pSurface);
    m_pDevice->SetDepthStencilSurface(m_shadowMaps[shadowIndex].pDepthSurface);

    m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(1, 1, 1), 1.0f, 0);

    if (pLight->GetType() == LIGHT_POINT) {
        UpdatePointShadowMatrices(shadowIndex, pLight->GetPosition());
    } else if (pLight->GetType() == LIGHT_SPOT) {
        UpdateSpotShadowMatrices(shadowIndex, pLight->GetPosition(), pLight->GetDirection());
    }
}

void ShadowSystem::EndShadowPass() {
    if (!m_pDevice) return;

    m_pDevice->SetRenderTarget(0, NULL);
    m_pDevice->SetDepthStencilSurface(NULL);
}

void ShadowSystem::BeginDirectionalShadowPass() {
    if (!m_pDevice || m_shadowMaps.empty()) return;

    if (m_shadowMaps[0].pSurface && m_shadowMaps[0].pDepthSurface) {
        m_pDevice->SetRenderTarget(0, m_shadowMaps[0].pSurface);
        m_pDevice->SetDepthStencilSurface(m_shadowMaps[0].pDepthSurface);
        m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(1, 1, 1), 1.0f, 0);
    }
}

void ShadowSystem::EndDirectionalShadowPass() {
    if (!m_pDevice) return;

    m_pDevice->SetRenderTarget(0, NULL);
    m_pDevice->SetDepthStencilSurface(NULL);
}

LPDIRECT3DTEXTURE9 ShadowSystem::GetShadowMapTexture(int index) {
    if (index >= 0 && index < (int)m_shadowMaps.size()) {
        return m_shadowMaps[index].pTexture;
    }
    return NULL;
}

LPDIRECT3DSURFACE9 ShadowSystem::GetShadowMapSurface(int index) {
    if (index >= 0 && index < (int)m_shadowMaps.size()) {
        return m_shadowMaps[index].pSurface;
    }
    return NULL;
}

LPDIRECT3DSURFACE9 ShadowSystem::GetShadowMapDepthSurface(int index) {
    if (index >= 0 && index < (int)m_shadowMaps.size()) {
        return m_shadowMaps[index].pDepthSurface;
    }
    return NULL;
}

const D3DXMATRIX& ShadowSystem::GetShadowMatrix(int index) const {
    static D3DXMATRIX identity;
    D3DXMatrixIdentity(&identity);

    if (index >= 0 && index < (int)m_shadowMaps.size()) {
        return m_shadowMaps[index].viewProjectionMatrix;
    }
    return identity;
}

const D3DXMATRIX& ShadowSystem::GetShadowViewMatrix(int index) const {
    static D3DXMATRIX identity;
    D3DXMatrixIdentity(&identity);

    if (index >= 0 && index < (int)m_shadowMaps.size()) {
        return m_shadowMaps[index].viewMatrix;
    }
    return identity;
}

const D3DXMATRIX& ShadowSystem::GetShadowProjectionMatrix(int index) const {
    static D3DXMATRIX identity;
    D3DXMatrixIdentity(&identity);

    if (index >= 0 && index < (int)m_shadowMaps.size()) {
        return m_shadowMaps[index].projectionMatrix;
    }
    return identity;
}

float ShadowSystem::GetShadowAttenuation(const D3DXVECTOR3& worldPos, int shadowMapIndex) {
    if (shadowMapIndex < 0 || shadowMapIndex >= (int)m_shadowMaps.size()) {
        return 1.0f;
    }

    D3DXVECTOR4 projected;
    D3DXVec3Transform(&projected, &worldPos, &m_shadowMaps[shadowMapIndex].viewProjectionMatrix);

    float u = (projected.x / projected.w) * 0.5f + 0.5f;
    float v = (projected.y / projected.w) * 0.5f + 0.5f;
    float depth = projected.z / projected.w;

    if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) {
        return 1.0f;
    }

    return depth;
}

void ShadowSystem::RenderDebugInfo() {
    if (!m_debugDraw || m_shadowMaps.empty()) return;

    char buf[256];
    sprintf(buf, "[ShadowSystem] Debug: %d shadow maps\n", (int)m_shadowMaps.size());
    OutputDebugStringA(buf);

    for (size_t i = 0; i < m_shadowMaps.size(); i++) {
        sprintf(buf, "  [%d] Directional=%d, Pos=(%.1f,%.1f,%.1f)\n",
                i, m_shadowMaps[i].isDirectional ? 1 : 0,
                m_shadowMaps[i].position.x, m_shadowMaps[i].position.y, m_shadowMaps[i].position.z);
        OutputDebugStringA(buf);
    }
}

}