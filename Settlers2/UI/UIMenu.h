#pragma once
#include <d3d9.h>
#include "../Graphics/RenderQueue.h"
#include "../Graphics/SpriteRenderer.h"

class TextManager;
namespace Input { class Gamepad; }

class UIMenu
{
public:
    struct ItemData {
        float u0, v0, u1, v1;
        float x, y;
        float w, h;
        const char* label;
    };

    struct BackgroundData {
        float u0, v0, u1, v1;
        float x, y;
        float w, h;
    };

    UIMenu();
    ~UIMenu();

    void SetAtlas(LPDIRECT3DTEXTURE9 texture, WORD textureSlot);
    void SetBackground(const BackgroundData& bg);
    void SetItems(const ItemData* items, int count);
    void SetTextManager(TextManager* tm) { m_textManager = tm; }
    void SetRenderer(Graphics::SpriteRenderer* sr, Graphics::RenderQueue* rq);

    void Show();
    void Hide();
    bool IsVisible() const { return m_visible; }

    void Update(Input::Gamepad* pad, float deltaTime);
    void Render();

    bool HasSelection() const { return m_selectionMade; }
    int GetSelectedIndex() const { return m_selectedIndex; }
    void ResetSelection();

private:
    LPDIRECT3DTEXTURE9 m_atlasTexture;
    WORD m_atlasSlot;
    bool m_hasBackground;
    BackgroundData m_background;
    const ItemData* m_items;
    int m_itemCount;
    bool m_visible;
    bool m_selectionMade;
    int m_selectedIndex;
    float m_inputCooldown;

    Graphics::SpriteRenderer* m_spriteRenderer;
    Graphics::RenderQueue* m_renderQueue;
    TextManager* m_textManager;
};
