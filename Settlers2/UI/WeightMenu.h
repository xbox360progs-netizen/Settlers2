#pragma once

#include "../World/ResourceNode.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/RenderQueue.h"
#include <d3dx9math.h>

class TextManager;

using Graphics::SpriteRenderer;

namespace UI {

class WeightMenu {
public:
    WeightMenu();
    ~WeightMenu();

    bool Initialize(SpriteRenderer* renderer, TextManager* textManager);
    void Update(float deltaTime);
    void Render();
    void Open(BYTE activeWeight);
    void Close();
    bool IsVisible() const { return m_isVisible; }
    BYTE GetSelectedWeight() const { return m_selectedWeight; }

    void SetTextures(LPDIRECT3DTEXTURE9 background, LPDIRECT3DTEXTURE9 dpadCross);
    void SetDpadUV(float u0, float v0, float u1, float v1) { m_dpadUV = D3DXVECTOR4(u0, v0, u1, v1); }
    void SetBackgroundUV(float u0, float v0, float u1, float v1) { m_bgUV = D3DXVECTOR4(u0, v0, u1, v1); }
    void SetRenderQueue(Graphics::RenderQueue* renderQueue) { m_renderQueue = renderQueue; }
    void SetTextureSlots(WORD bgSlot, WORD dpadSlot) { m_bgSlot = bgSlot; m_dpadSlot = dpadSlot; }
    void SetPlacementMode(bool placementMode) { m_isPlacementMode = placementMode; }

private:
    bool m_isVisible;
    BYTE m_selectedWeight;
    float m_animationTime;

    LPDIRECT3DTEXTURE9 m_backgroundTexture;
    LPDIRECT3DTEXTURE9 m_dpadCrossTexture;
    SpriteRenderer* m_spriteRenderer;
    TextManager* m_textManager;
    Graphics::RenderQueue* m_renderQueue;

    D3DXVECTOR2 m_position;
    float m_scale;

    WORD m_bgSlot;
    WORD m_dpadSlot;
    D3DXVECTOR4 m_dpadUV;
    D3DXVECTOR4 m_bgUV;
    bool m_isPlacementMode;
};

} // namespace UI
