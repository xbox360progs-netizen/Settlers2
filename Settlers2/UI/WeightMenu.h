#pragma once

#include "../World/ResourceNode.h"
#include <d3dx9math.h>

class Texture;
class SpriteRenderer;
class TextManager;

namespace UI {

class WeightMenu {
public:
    WeightMenu();
    ~WeightMenu();

    bool Initialize(Texture* background, Texture* dpadCross, SpriteRenderer* renderer, TextManager* textManager);
    void Update(float deltaTime);
    void Render();
    void Open();
    void Close();
    bool IsVisible() const { return m_isVisible; }
    BYTE GetSelectedWeight() const { return m_selectedWeight; }

private:
    bool m_isVisible;
    BYTE m_selectedWeight;
    float m_animationTime;

    Texture* m_backgroundTexture;
    Texture* m_dpadCrossTexture;
    SpriteRenderer* m_spriteRenderer;
    TextManager* m_textManager;

    D3DXVECTOR2 m_position;
    float m_scale;
};

} // namespace UI
