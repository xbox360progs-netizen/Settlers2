#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include "Renderer.h"

class Texture;

class SpriteBatch {
public:
    SpriteBatch();
    ~SpriteBatch();

    HRESULT Initialize(LPDIRECT3DDEVICE9 device);
    void Shutdown();

    void Begin();
    void Draw(Texture* texture, float x, float y, float w, float h, float u0 = 0, float v0 = 0, float u1 = 1, float v1 = 1, DWORD color = 0xFFFFFFFF);
    void End();
    // Handle device loss/recovery for Xbox 360 (XDK)
    void OnLostDevice();
    void OnResetDevice();

private:
    void Flush();

    LPDIRECT3DDEVICE9   m_pDevice;
    // Ring-buffer: two vertex buffers
    LPDIRECT3DVERTEXBUFFER9 m_pVB[2];
    LPDIRECT3DINDEXBUFFER9 m_pIB;
    SpriteVertex* m_pVertices;
    UINT m_spriteCount;
    Texture* m_pCurrentTexture;
    bool m_bDrawing;
    bool m_vbLocked[2]; // Tracks if each vertex buffer is currently locked
    int  m_writeIndex;  // Current ring index being written to
};
