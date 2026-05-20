#include "stdafx.h"
#include "LightOptimizer.h"
#include <math.h>

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

LightOptimizer::LightOptimizer()
    : m_pDevice(NULL), m_optimizationsEnabled(true), m_debugDraw(false),
      m_scissorWasEnabled(false) {
    ZeroMemory(&m_savedScissorRect, sizeof(m_savedScissorRect));
    ZeroMemory(&m_sphereMesh, sizeof(m_sphereMesh));
    ZeroMemory(&m_coneMesh, sizeof(m_coneMesh));
    ZeroMemory(&m_quadMesh, sizeof(m_quadMesh));
    m_volumeDecl = NULL;
}

LightOptimizer::~LightOptimizer() {
    ReleaseMesh(m_sphereMesh);
    ReleaseMesh(m_coneMesh);
    ReleaseMesh(m_quadMesh);
    if (m_volumeDecl) {
        m_volumeDecl->Release();
        m_volumeDecl = NULL;
    }
}

void LightOptimizer::ReleaseMesh(VolumeMesh& mesh) {
    if (mesh.vb) {
        mesh.vb->Release();
        mesh.vb = NULL;
    }
    if (mesh.ib) {
        mesh.ib->Release();
        mesh.ib = NULL;
    }
    mesh.vertexCount = 0;
    mesh.indexCount = 0;
}

void LightOptimizer::ApplyScissorRectForPointLight(const D3DXVECTOR3& lightPos, float radius, int screenWidth, int screenHeight) {
    if (!m_pDevice || !m_optimizationsEnabled) return;

    RECT scissorRect;
    ProjectSphereToScreen(lightPos, radius, screenWidth, screenHeight, scissorRect);

    m_pDevice->GetScissorRect(&m_savedScissorRect);
    m_pDevice->SetScissorRect(&scissorRect);
    m_pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);

    m_scissorWasEnabled = true;
}

void LightOptimizer::ApplyScissorRectForSpotLight(const D3DXVECTOR3& lightPos, const D3DXVECTOR3& lightDir, float radius, float angle, int screenWidth, int screenHeight) {
    if (!m_pDevice || !m_optimizationsEnabled) return;

    RECT scissorRect;
    ProjectConeToScreen(lightPos, lightDir, radius, angle, screenWidth, screenHeight, scissorRect);

    m_pDevice->GetScissorRect(&m_savedScissorRect);
    m_pDevice->SetScissorRect(&scissorRect);
    m_pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);

    m_scissorWasEnabled = true;
}

void LightOptimizer::DisableScissor() {
    if (!m_pDevice) return;
    m_pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
}

void LightOptimizer::RestorePreviousScissor() {
    if (!m_pDevice || !m_scissorWasEnabled) return;
    m_pDevice->SetScissorRect(&m_savedScissorRect);
    m_scissorWasEnabled = false;
}

void LightOptimizer::SaveViewportState() {
}

void LightOptimizer::RestoreViewportState() {
}

void LightOptimizer::ProjectSphereToScreen(const D3DXVECTOR3& center, float radius, int screenW, int screenH, RECT& outRect) {
    float minX = (float)screenW;
    float maxX = 0.0f;
    float minY = (float)screenH;
    float maxY = 0.0f;

    int samples = 16;
    float step = 3.14159265359f * 2.0f / samples;

    for (int i = 0; i < samples; i++) {
        float angle = i * step;
        D3DXVECTOR3 offset(cosf(angle) * radius, sinf(angle) * radius, 0);
        D3DXVECTOR3 worldPos = center + offset;

        float screenX = worldPos.x * 0.5f + screenW * 0.5f;
        float screenY = worldPos.y * 0.5f + screenH * 0.5f;

        if (screenX < minX) minX = screenX;
        if (screenX > maxX) maxX = screenX;
        if (screenY < minY) minY = screenY;
        if (screenY > maxY) maxY = screenY;
    }

    int padding = 4;
    outRect.left = max(0, (int)(minX - padding));
    outRect.top = max(0, (int)(minY - padding));
    outRect.right = min(screenW, (int)(maxX + padding));
    outRect.bottom = min(screenH, (int)(maxY + padding));
}

void LightOptimizer::ProjectConeToScreen(const D3DXVECTOR3& origin, const D3DXVECTOR3& direction, float length, float angle, int screenW, int screenH, RECT& outRect) {
    D3DXVECTOR3 tip = origin;
    D3DXVECTOR3 base = origin + direction * length;

    float baseRadius = length * tanf(angle * 0.5f);

    RECT tipRect, baseRect;
    ProjectSphereToScreen(tip, 1.0f, screenW, screenH, tipRect);
    ProjectSphereToScreen(base, baseRadius, screenW, screenH, baseRect);

    outRect.left = min(tipRect.left, baseRect.left);
    outRect.top = min(tipRect.top, baseRect.top);
    outRect.right = max(tipRect.right, baseRect.right);
    outRect.bottom = max(tipRect.bottom, baseRect.bottom);
}

void LightOptimizer::CreateSphereMesh(float radius, int segments) {
    if (!m_pDevice) return;

    ReleaseMesh(m_sphereMesh);

    int rings = segments;
    int sectors = segments;
    int vertCount = (rings + 1) * (sectors + 1);
    int indexCount = rings * sectors * 6;

    struct Vertex {
        float x, y, z;
    };

    HRESULT hr = m_pDevice->CreateVertexBuffer(vertCount * sizeof(Vertex), 0, 0, D3DPOOL_DEFAULT, &m_sphereMesh.vb, NULL);
    if (FAILED(hr)) return;

    hr = m_pDevice->CreateIndexBuffer(indexCount * sizeof(WORD), 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_sphereMesh.ib, NULL);
    if (FAILED(hr)) return;

    Vertex* verts;
    m_sphereMesh.vb->Lock(0, 0, (void**)&verts, 0);

    for (int r = 0; r <= rings; r++) {
        float v = (float)r / rings;
        float phi = v * 3.14159265359f;

        for (int s = 0; s <= sectors; s++) {
            float u = (float)s / sectors;
            float theta = u * 2.0f * 3.14159265359f;

            int idx = r * (sectors + 1) + s;
            verts[idx].x = radius * sinf(phi) * cosf(theta);
            verts[idx].y = radius * cosf(phi);
            verts[idx].z = radius * sinf(phi) * sinf(theta);
        }
    }

    m_sphereMesh.vb->Unlock();

    WORD* indices;
    m_sphereMesh.ib->Lock(0, 0, (void**)&indices, 0);

    int idx = 0;
    for (int r = 0; r < rings; r++) {
        for (int s = 0; s < sectors; s++) {
            int curr = r * (sectors + 1) + s;
            int next = curr + sectors + 1;

            indices[idx++] = curr;
            indices[idx++] = next;
            indices[idx++] = curr + 1;

            indices[idx++] = curr + 1;
            indices[idx++] = next;
            indices[idx++] = next + 1;
        }
    }

    m_sphereMesh.ib->Unlock();
    m_sphereMesh.vertexCount = vertCount;
    m_sphereMesh.indexCount = indexCount;
}

void LightOptimizer::CreateConeMesh(float angle, int segments) {
    if (!m_pDevice) return;

    ReleaseMesh(m_coneMesh);

    int vertCount = segments + 2;
    int indexCount = segments * 3;

    struct Vertex {
        float x, y, z;
    };

    HRESULT hr = m_pDevice->CreateVertexBuffer(vertCount * sizeof(Vertex), 0, 0, D3DPOOL_DEFAULT, &m_coneMesh.vb, NULL);
    if (FAILED(hr)) return;

    hr = m_pDevice->CreateIndexBuffer(indexCount * sizeof(WORD), 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_coneMesh.ib, NULL);
    if (FAILED(hr)) return;

    Vertex* verts;
    m_coneMesh.vb->Lock(0, 0, (void**)&verts, 0);

    verts[0].x = 0; verts[0].y = 0; verts[0].z = 0;

    float radius = tanf(angle * 0.5f);
    for (int i = 0; i < segments; i++) {
        float t = (float)i / segments * 2.0f * 3.14159265359f;
        verts[i + 1].x = cosf(t) * radius;
        verts[i + 1].y = 1.0f;
        verts[i + 1].z = sinf(t) * radius;
    }

    verts[segments + 1].x = 0; verts[segments + 1].y = -1.0f; verts[segments + 1].z = 0;

    m_coneMesh.vb->Unlock();

    WORD* indices;
    m_coneMesh.ib->Lock(0, 0, (void**)&indices, 0);

    int idx = 0;
    for (int i = 0; i < segments; i++) {
        indices[idx++] = 0;
        indices[idx++] = i + 1;
        indices[idx++] = ((i + 1) % segments) + 1;
    }

    for (int i = 0; i < segments; i++) {
        indices[idx++] = segments + 1;
        indices[idx++] = ((i + 1) % segments) + 1;
        indices[idx++] = i + 1;
    }

    m_coneMesh.ib->Unlock();
    m_coneMesh.vertexCount = vertCount;
    m_coneMesh.indexCount = indexCount;
}

void LightOptimizer::CreateQuadMesh() {
    if (!m_pDevice) return;

    ReleaseMesh(m_quadMesh);

    struct Vertex {
        float x, y, z;
        float u, v;
    };

    Vertex verts[4] = {
        { -1, -1, 0, 0, 0 },
        {  1, -1, 0, 1, 0 },
        {  1,  1, 0, 1, 1 },
        { -1,  1, 0, 0, 1 }
    };

    HRESULT hr = m_pDevice->CreateVertexBuffer(4 * sizeof(Vertex), 0, 0, D3DPOOL_DEFAULT, &m_quadMesh.vb, NULL);
    if (FAILED(hr)) return;

    WORD indices[6] = { 0, 1, 2, 0, 2, 3 };
    hr = m_pDevice->CreateIndexBuffer(6 * sizeof(WORD), 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_quadMesh.ib, NULL);
    if (FAILED(hr)) return;

    void* pData;
    m_quadMesh.vb->Lock(0, 0, &pData, 0);
    memcpy(pData, verts, sizeof(verts));
    m_quadMesh.vb->Unlock();

    m_quadMesh.ib->Lock(0, 0, &pData, 0);
    memcpy(pData, indices, sizeof(indices));
    m_quadMesh.ib->Unlock();

    m_quadMesh.vertexCount = 4;
    m_quadMesh.indexCount = 6;
}

void LightOptimizer::DrawLightVolumeSphere(const D3DXVECTOR3& pos, float radius) {
    if (!m_pDevice || !m_debugDraw) return;

    if (!m_sphereMesh.vb) {
        CreateSphereMesh(radius, 16);
    }

    if (m_sphereMesh.vb && m_sphereMesh.ib) {
        // Xbox 360: SetTransform not available, transforms done via shaders
        m_pDevice->SetStreamSource(0, m_sphereMesh.vb, 0, sizeof(float) * 3);
        m_pDevice->SetIndices(m_sphereMesh.ib);
        m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, m_sphereMesh.vertexCount, 0, m_sphereMesh.indexCount / 3);
    }
}

void LightOptimizer::DrawLightVolumeCone(const D3DXVECTOR3& pos, const D3DXVECTOR3& dir, float length, float angle) {
    if (!m_pDevice || !m_debugDraw) return;

    if (!m_coneMesh.vb) {
        CreateConeMesh(angle, 16);
    }

    if (m_coneMesh.vb && m_coneMesh.ib) {
        // Xbox 360: SetTransform not available, transforms done via shaders
        D3DXVECTOR3 up(0, 1, 0);
        D3DXVECTOR3 axis;
        D3DXVec3Cross(&axis, &up, &dir);
        float angle2 = acosf(D3DXVec3Dot(&up, &dir));
        D3DXMATRIX rotate;
        D3DXMatrixRotationAxis(&rotate, &axis, angle2);

        m_pDevice->SetStreamSource(0, m_coneMesh.vb, 0, sizeof(float) * 3);
        m_pDevice->SetIndices(m_coneMesh.ib);
        m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, m_coneMesh.vertexCount, 0, m_coneMesh.indexCount / 3);
    }
}

void LightOptimizer::DrawFullscreenQuad() {
    if (!m_pDevice) return;

    if (!m_quadMesh.vb) {
        CreateQuadMesh();
    }

    if (m_quadMesh.vb && m_quadMesh.ib) {
        m_pDevice->SetStreamSource(0, m_quadMesh.vb, 0, sizeof(float) * 5);
        m_pDevice->SetIndices(m_quadMesh.ib);
        m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2);
    }
}

}