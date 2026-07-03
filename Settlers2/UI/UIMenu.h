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
    // Read-only accessors for rendering
    bool IsVisible() const { return m_visible; }
    int GetItemCount() const { return m_itemCount; }
    int GetSelectedIndex() const { return m_selectedIndex; }
    const BackgroundData& GetBackground() const { return m_background; }
    const ItemData* GetItems() const { return m_items; }
    WORD GetAtlasSlot() const { return m_atlasSlot; }
    LPDIRECT3DTEXTURE9 GetAtlasTexture() const { return m_atlasTexture; }

    void Show();
    void Hide();

    void Update(Input::Gamepad* pad, float deltaTime);
    void Render();

    bool HasSelection() const { return m_selectionMade; }
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
