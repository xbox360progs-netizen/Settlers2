#include "stdafx.h"
#include "UIMenu.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/RenderLayers.h"
#include "../Input/Gamepad.h"
#include "../Graphics/TextManager.h"

UIMenu::UIMenu()
    : m_atlasTexture(nullptr)
    , m_atlasSlot(0)
    , m_hasBackground(false)
    , m_items(nullptr)
    , m_itemCount(0)
    , m_visible(false)
    , m_selectionMade(false)
    , m_selectedIndex(0)
    , m_inputCooldown(0.0f)
    , m_spriteRenderer(nullptr)
    , m_renderQueue(nullptr)
    , m_textManager(nullptr)
{
    m_background.u0 = m_background.v0 = 0.0f;
    m_background.u1 = m_background.v1 = 1.0f;
    m_background.x = m_background.y = 0.0f;
    m_background.w = m_background.h = 0.0f;
}

UIMenu::~UIMenu()
{
}

void UIMenu::SetAtlas(LPDIRECT3DTEXTURE9 texture, WORD textureSlot)
{
    m_atlasTexture = texture;
    m_atlasSlot = textureSlot;
}

void UIMenu::SetBackground(const BackgroundData& bg)
{
    m_background = bg;
    m_hasBackground = true;
}

void UIMenu::SetItems(const ItemData* items, int count)
{
    m_items = items;
    m_itemCount = count;
    m_selectedIndex = 0;
}

void UIMenu::SetRenderer(Graphics::SpriteRenderer* sr, Graphics::RenderQueue* rq)
{
    m_spriteRenderer = sr;
    m_renderQueue = rq;
}

void UIMenu::Show()
{
    m_visible = true;
    m_selectedIndex = 0;
    m_selectionMade = false;
    m_inputCooldown = 0.0f;
}

void UIMenu::Hide()
{
    m_visible = false;
}

void UIMenu::ResetSelection()
{
    m_selectionMade = false;
    m_selectedIndex = 0;
}

void UIMenu::Update(Input::Gamepad* pad, float deltaTime)
{
    if (!m_visible || !pad) return;

    m_inputCooldown -= deltaTime;

    if (m_inputCooldown <= 0.0f && m_itemCount > 0) {
        float sx, sy;
        pad->GetLeftStick(sx, sy);
        bool moved = false;

        if (sx > 0.3f || pad->IsButtonPressed(Input::GP_DPadRight)) {
            m_selectedIndex = (m_selectedIndex + 1) % m_itemCount;
            moved = true;
        } else if (sx < -0.3f || pad->IsButtonPressed(Input::GP_DPadLeft)) {
            m_selectedIndex = (m_selectedIndex - 1 + m_itemCount) % m_itemCount;
            moved = true;
        }

        if (moved) m_inputCooldown = 0.2f;
    }

    if (pad->IsButtonPressed(Input::GP_A) && m_itemCount > 0) {
        m_selectionMade = true;
    }
    if (pad->IsButtonPressed(Input::GP_B)) {
        Hide();
    }
}

void UIMenu::Render()
{
    if (!m_visible || !m_renderQueue || !m_atlasTexture) return;

    if (m_spriteRenderer) {
        m_spriteRenderer->SetTextureSlot(m_atlasSlot, m_atlasTexture);
    }

    // Background
    if (m_hasBackground) {
        Graphics::RenderCommand bgCmd = {};
        bgCmd.x = m_background.x;
        bgCmd.y = m_background.y;
        bgCmd.width = m_background.w;
        bgCmd.height = m_background.h;
        bgCmd.u0 = m_background.u0; bgCmd.v0 = m_background.v0;
        bgCmd.u1 = m_background.u1; bgCmd.v1 = m_background.v1;
        bgCmd.color = 0xFFFFFFFF;
        bgCmd.shaderID = SHADER_UI;
        bgCmd.textureID = m_atlasSlot;
        bgCmd.blendMode = 1;
        bgCmd.layer = LAYER_UI;
        bgCmd.depth = 10;
        m_renderQueue->Submit(bgCmd);
    }

    // Items
    for (int i = 0; i < m_itemCount; ++i) {
        const ItemData& item = m_items[i];

        // Selection highlight behind selected item
        if (i == m_selectedIndex) {
            Graphics::RenderCommand hlCmd = {};
            hlCmd.x = item.x - 4.0f;
            hlCmd.y = item.y - 4.0f;
            hlCmd.width = item.w + 8.0f;
            hlCmd.height = item.h + 8.0f;
            hlCmd.u0 = 0.5f; hlCmd.v0 = 0.5f;
            hlCmd.u1 = 0.5001f; hlCmd.v1 = 0.5001f;
            hlCmd.color = D3DCOLOR_ARGB(100, 255, 255, 0);
            hlCmd.shaderID = SHADER_UI;
            hlCmd.textureID = m_atlasSlot;
            hlCmd.blendMode = 1;
            hlCmd.layer = LAYER_UI;
            hlCmd.depth = 50;
            m_renderQueue->Submit(hlCmd);
        }

        // Item sprite
        Graphics::RenderCommand cmd = {};
        cmd.x = item.x;
        cmd.y = item.y;
        cmd.width = item.w;
        cmd.height = item.h;
        cmd.u0 = item.u0; cmd.v0 = item.v0;
        cmd.u1 = item.u1; cmd.v1 = item.v1;
        cmd.color = 0xFFFFFFFF;
        cmd.shaderID = SHADER_UI;
        cmd.textureID = m_atlasSlot;
        cmd.blendMode = 1;
        cmd.layer = LAYER_UI;
        cmd.depth = 100;
        m_renderQueue->Submit(cmd);

        // Item label
        if (m_textManager && item.label) {
            m_textManager->DrawTextCenteredToScreen(item.label,
                item.x + item.w * 0.5f, item.y + item.h + 4.0f,
                (i == m_selectedIndex) ? D3DCOLOR_ARGB(255, 255, 255, 0) : D3DCOLOR_ARGB(255, 200, 200, 200),
                0.07f);
        }
    }
}
