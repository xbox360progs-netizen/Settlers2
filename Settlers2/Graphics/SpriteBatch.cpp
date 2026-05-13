#include "stdafx.h"
#include "SpriteBatch.h"
#include "Texture.h"

#define MAX_SPRITES 1800

SpriteBatch::SpriteBatch()
    : m_pDevice(NULL), m_pIB(NULL), m_pVertices(NULL), m_spriteCount(0),
      m_pCurrentTexture(NULL), m_bDrawing(false), m_writeIndex(0)
{
    m_pVB[0] = NULL; m_pVB[1] = NULL;
    m_vbLocked[0] = false; m_vbLocked[1] = false;
}

SpriteBatch::~SpriteBatch() {
    Shutdown();
}

HRESULT SpriteBatch::Initialize(LPDIRECT3DDEVICE9 device) {
    m_pDevice = device;

    // Create two vertex buffers for ring-buffer (double buffering)
    HRESULT hr = S_OK;
    for (int i = 0; i < 2; ++i) {
        hr = device->CreateVertexBuffer(
            MAX_SPRITES * 4 * sizeof(SpriteVertex),
            0,
            0,
            D3DPOOL_DEFAULT,
            &m_pVB[i],
            NULL
        );
        if (FAILED(hr)) return hr;
        m_vbLocked[i] = false;
        m_pVertices = NULL;
    }

    WORD* indices = new WORD[MAX_SPRITES * 6];
    for (UINT i = 0; i < MAX_SPRITES; i++) {
        indices[i * 6 + 0] = i * 4 + 0;
        indices[i * 6 + 1] = i * 4 + 1;
        indices[i * 6 + 2] = i * 4 + 2;
        indices[i * 6 + 3] = i * 4 + 0;
        indices[i * 6 + 4] = i * 4 + 2;
        indices[i * 6 + 5] = i * 4 + 3;
    }

    hr = device->CreateIndexBuffer(
        MAX_SPRITES * 6 * sizeof(WORD),
        0,
        D3DFMT_INDEX16,
        D3DPOOL_DEFAULT,
        &m_pIB,
        NULL
    );
    if (FAILED(hr)) {
        delete[] indices;
        return hr;
    }

    void* pData;
    m_pIB->Lock(0, 0, &pData, 0);
    memcpy(pData, indices, MAX_SPRITES * 6 * sizeof(WORD));
    m_pIB->Unlock();
    delete[] indices;

    return S_OK;
}

void SpriteBatch::Shutdown() {
    if (m_pIB) { m_pIB->Release(); m_pIB = NULL; }
    for (int i = 0; i < 2; ++i) {
        if (m_pVB[i]) { m_pVB[i]->Release(); m_pVB[i] = NULL; }
        m_vbLocked[i] = false;
    }
    m_pVertices = NULL;
    m_writeIndex = 0;
}

void SpriteBatch::Begin() {
    m_spriteCount = 0;
    m_pCurrentTexture = NULL;
    m_bDrawing = true;

    // Lock the vertex buffer for the current ring slot
    if (!m_vbLocked[m_writeIndex]) {
        HRESULT hrLock = m_pVB[m_writeIndex]->Lock(0, MAX_SPRITES * 4 * sizeof(SpriteVertex), (void**)&m_pVertices, 0);
        if (FAILED(hrLock)) return;
        m_vbLocked[m_writeIndex] = true;
    }

    m_pDevice->SetStreamSource(0, m_pVB[m_writeIndex], 0, sizeof(SpriteVertex));
    m_pDevice->SetIndices(m_pIB);
    m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
}

void SpriteBatch::Draw(Texture* texture, float x, float y, float w, float h, float u0, float v0, float u1, float v1) {
    if (!m_bDrawing || m_spriteCount >= MAX_SPRITES) return;

    // ����� �������� (Batching)
    if (texture != m_pCurrentTexture) {
        if (m_spriteCount > 0) Flush();
        m_pCurrentTexture = texture;
        if (texture) m_pDevice->SetTexture(0, texture->GetTexture());
    }

    // Ensure vertex buffer is locked before writing (in case it was unlocked on Flush)
    int cur = m_writeIndex;
    if (!m_vbLocked[cur]) {
        HRESULT hr = m_pVB[cur]->Lock(0, MAX_SPRITES * 4 * sizeof(SpriteVertex), (void**)&m_pVertices, 0);
        if (FAILED(hr)) return;
        m_vbLocked[cur] = true;
    }

    // ��������� �� ������� 4 ������� ������� (������ �� 32 �����)
    SpriteVertex* v = &m_pVertices[m_spriteCount * 4];

    // ��������� 4 ������� (Quad)
    // �������: X, Y, Z, Color, U, V, Padding
    
    // 0: Top-Left
    float bias = -0.5f; // Half-pixel bias for pixel-perfect 2D rendering
    v[0].x = x + bias;     v[0].y = y + bias;     v[0].z = 0.0f; v[0].color = 0xFFFFFFFF; v[0].u = u0; v[0].v = v0; 
    
    // 1: Top-Right
    v[1].x = x + w + bias; v[1].y = y + bias;     v[1].z = 0.0f; v[1].color = 0xFFFFFFFF; v[1].u = u1;
    
    // 2: Bottom-Right
    v[2].x = x + w + bias; v[2].y = y + h + bias; v[2].z = 0.0f; v[2].color = 0xFFFFFFFF; v[2].u = u1; v[2].v = v1;
    
    // 3: Bottom-Left
    v[3].x = x + bias; v[3].y = y + h + bias; v[3].z = 0.0f; v[3].color = 0xFFFFFFFF; v[3].u = u0; v[3].v = v1;

    m_spriteCount++;
}

void SpriteBatch::Flush() {
    if (m_spriteCount == 0 || !m_pVertices) return;

    // Unlock current VB and draw from it (guard unlock if not locked)
    if (m_pVB[m_writeIndex] && m_vbLocked[m_writeIndex]) {
        m_pVB[m_writeIndex]->Unlock();
        m_vbLocked[m_writeIndex] = false;
    }
    m_pVertices = NULL;

    // Bind the VB for drawing and issue the draw call
    m_pDevice->SetStreamSource(0, m_pVB[m_writeIndex], 0, sizeof(SpriteVertex));

    m_pDevice->DrawIndexedPrimitive(
        D3DPT_TRIANGLELIST,
        0, 0, m_spriteCount * 4,
        0, m_spriteCount * 2
    );

    // Move to the next ring buffer slot and reset batch state
    m_spriteCount = 0;
    m_writeIndex = (m_writeIndex + 1) % 2;
}

void SpriteBatch::End() {
    if (!m_bDrawing) return;
    Flush();
    m_pDevice->SetTexture(0, NULL);
    m_bDrawing = false;
}

// Windows/Xbox 360 device-loss handling
void SpriteBatch::OnLostDevice() {
    // Release resources that are in D3DPOOL_DEFAULT and recreate on reset
    if (m_pIB) { m_pIB->Release(); m_pIB = NULL; }
    for (int i = 0; i < 2; ++i) {
        if (m_pVB[i]) { m_pVB[i]->Release(); m_pVB[i] = NULL; }
        m_vbLocked[i] = false;
    }
    m_pVertices = NULL;
    m_writeIndex = 0;
}

void SpriteBatch::OnResetDevice() {
    // Reset internal batch state
    m_spriteCount = 0;
    m_pCurrentTexture = NULL;
    m_bDrawing = false;
    // Re-create vertex buffers and index buffer for both rings
    for (int i = 0; i < 2; ++i) {
        HRESULT hr = m_pDevice->CreateVertexBuffer(
            MAX_SPRITES * 4 * sizeof(SpriteVertex),
            0,
            0,
            D3DPOOL_DEFAULT,
            &m_pVB[i],
            NULL
        );
        if (FAILED(hr)) {
            return;
        }
        m_pVB[i] = m_pVB[i]; // no-op, just to emphasize we keep both buffers
        m_vbLocked[i] = false;
    }
    // Index Buffer
    HRESULT hr = m_pDevice->CreateIndexBuffer(
        MAX_SPRITES * 6 * sizeof(WORD),
        0,
        D3DFMT_INDEX16,
        D3DPOOL_DEFAULT,
        &m_pIB,
        NULL
    );
    if (FAILED(hr)) {
        return;
    }
    // Fill indices
    void* pData = NULL;
    WORD* pIndices = NULL;
    hr = m_pIB->Lock(0, 0, (void**)&pIndices, 0);
    if (SUCCEEDED(hr)) {
        for (UINT i = 0; i < MAX_SPRITES; i++) {
            int baseV = i * 4;
            int baseI = i * 6;
            pIndices[baseI + 0] = baseV + 0;
            pIndices[baseI + 1] = baseV + 1;
            pIndices[baseI + 2] = baseV + 2;
            pIndices[baseI + 3] = baseV + 0;
            pIndices[baseI + 4] = baseV + 2;
            pIndices[baseI + 5] = baseV + 3;
        }
        m_pIB->Unlock();
    }

    m_pVertices = NULL;
    m_vbLocked[0] = false;
    m_vbLocked[1] = false;
}
