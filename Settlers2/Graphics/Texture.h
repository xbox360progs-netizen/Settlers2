#pragma once
#include <d3d9.h>

class Texture {
public:
    Texture() : m_pTexture(NULL), m_width(0), m_height(0) {}
    ~Texture() { Release(); }

    // Тот самый метод, который требуют LoadingScene и MenuScene
    LPDIRECT3DTEXTURE9 GetTexture() const { 
        return m_pTexture; 
    }

    void SetTexture(LPDIRECT3DTEXTURE9 pTex) {
        if (m_pTexture == pTex) return;
        if (m_pTexture) m_pTexture->Release();
        m_pTexture = pTex;
        if (m_pTexture) {
            m_pTexture->AddRef();
            D3DSURFACE_DESC desc;
            m_pTexture->GetLevelDesc(0, &desc);
            m_width = desc.Width;
            m_height = desc.Height;
        }
    }

    UINT GetWidth() const { return m_width; }
    UINT GetHeight() const { return m_height; }

    void Release() {
        if (m_pTexture) {
            m_pTexture->Release();
            m_pTexture = NULL;
        }
    }

private:
    LPDIRECT3DTEXTURE9 m_pTexture;
    UINT m_width;
    UINT m_height;
};