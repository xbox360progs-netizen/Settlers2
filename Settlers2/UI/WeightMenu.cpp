#include "stdafx.h"
#include "WeightMenu.h"
#include "../Graphics/Texture.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/TextManager.h"
#include "../Graphics/RenderLayers.h"
#include "../Input/Gamepad.h"

namespace UI {

WeightMenu::WeightMenu()
    : m_isVisible(false)
    , m_selectedWeight(World::Weight_Land)
    , m_animationTime(0.0f)
    , m_backgroundTexture(nullptr)
    , m_dpadCrossTexture(nullptr)
    , m_spriteRenderer(nullptr)
    , m_textManager(nullptr)
    , m_renderQueue(nullptr)
    , m_position(640.0f, 360.0f)
    , m_scale(1.0f)
    , m_bgSlot(0)
    , m_dpadSlot(0)
    , m_dpadUV(0.0f, 0.0f, 1.0f, 1.0f)
    , m_bgUV(0.0f, 0.0f, 1.0f, 1.0f)
    , m_isPlacementMode(false)
{
}

WeightMenu::~WeightMenu()
{
}

bool WeightMenu::Initialize(SpriteRenderer* renderer, TextManager* textManager)
{
    m_spriteRenderer = renderer;
    m_textManager = textManager;
    return true;
}

void WeightMenu::SetTextures(LPDIRECT3DTEXTURE9 background, LPDIRECT3DTEXTURE9 dpadCross)
{
    m_backgroundTexture = background;
    m_dpadCrossTexture = dpadCross;
    if (m_spriteRenderer) {
        if (m_backgroundTexture) m_spriteRenderer->SetTextureSlot(m_bgSlot, m_backgroundTexture);
        if (m_dpadCrossTexture) m_spriteRenderer->SetTextureSlot(m_dpadSlot, m_dpadCrossTexture);
    }
}

void WeightMenu::Update(Input::Gamepad* input, float deltaTime)
{
    if (!m_isVisible) return;
    m_animationTime += deltaTime;

    if (input && input->IsButtonPressed(Input::GP_B)) {
        Close();
    }
}

void WeightMenu::Render()
{
    if (!m_isVisible || !m_renderQueue) return;

    // Background (2x size, menu_Grid from UI atlas)
    if (m_backgroundTexture) {
        char buf[128];
        sprintf(buf, "[GPU Debug] WeightMenu Rendering. bgSlot: %d, tex: %p\n", m_bgSlot, m_backgroundTexture);
        OutputDebugStringA(buf);
        Graphics::RenderCommand cmd = {};
        cmd.x = m_position.x - 300.0f;
        cmd.y = m_position.y - 300.0f;
        cmd.width = 600.0f;
        cmd.height = 600.0f;
        cmd.u0 = m_bgUV.x; cmd.v0 = m_bgUV.y;
        cmd.u1 = m_bgUV.z; cmd.v1 = m_bgUV.w;
        cmd.color = 0xFFFFFFFF;
        cmd.shaderID = SHADER_UI;
        cmd.textureID = m_bgSlot;
        cmd.blendMode = 1;
        cmd.layer = LAYER_UI;
        cmd.depth = 70;
        cmd.sortKey = Graphics::BuildSortKey(LAYER_UI, 1, SHADER_UI, m_bgSlot, 70);
        m_renderQueue->Submit(cmd);
    }

    // D-pad cross in center
    if (m_dpadCrossTexture) {
        Graphics::RenderCommand cmd = {};
        cmd.x = m_position.x - 64.0f;
        cmd.y = m_position.y - 64.0f;
        cmd.width = 128.0f;
        cmd.height = 128.0f;
        cmd.u0 = m_dpadUV.x; cmd.v0 = m_dpadUV.y;
        cmd.u1 = m_dpadUV.z; cmd.v1 = m_dpadUV.w;
        cmd.color = 0xFFFFFFFF;
        cmd.shaderID = SHADER_UI;
        cmd.textureID = m_dpadSlot;
        cmd.blendMode = 1;
        cmd.layer = LAYER_UI;
        cmd.depth = 90;
        cmd.sortKey = Graphics::BuildSortKey(LAYER_UI, 1, SHADER_UI, m_dpadSlot, 90);
        m_renderQueue->Submit(cmd);
    }

    // Labels around the D-pad (screen-space text)
    if (m_textManager) {
        float s = 0.25f;
        float textDepth = 0.1f;
        if (m_isPlacementMode) {
            // Placement mode: only 2 options (Up=Occupied, Down=Free)
            bool isOccupied = (m_selectedWeight == World::Weight_Block);
            bool isFree = (m_selectedWeight == World::Weight_Land);
            m_textManager->DrawString("OCCUPIED", m_position.x - 34.0f, m_position.y - 180.0f,
                isOccupied ? 0xFFFF4444 : 0xFF882222, s, FONT_MENU, true, FONT_STYLE_NORMAL, textDepth);
            m_textManager->DrawString("FREE", m_position.x - 16.0f, m_position.y + 155.0f,
                isFree ? 0xFF44FF44 : 0xFF227722, s, FONT_MENU, true, FONT_STYLE_NORMAL, textDepth);
        } else {
            // Each weight has its own color; active is bright, inactive is dim
            bool isBlock = (m_selectedWeight == World::Weight_Block);
            bool isDeep = (m_selectedWeight == World::Weight_Deep);
            bool isShallow = (m_selectedWeight == World::Weight_Shallow);
            bool isLand = (m_selectedWeight == World::Weight_Land);
            m_textManager->DrawString("BLOCK", m_position.x - 24.0f, m_position.y - 180.0f,
                isBlock ? 0xFFFF4444 : 0xFF882222, s, FONT_MENU, true, FONT_STYLE_NORMAL, textDepth);
            m_textManager->DrawString("DEEP", m_position.x - 24.0f, m_position.y + 155.0f,
                isDeep ? 0xFF4488FF : 0xFF224477, s, FONT_MENU, true, FONT_STYLE_NORMAL, textDepth);
            m_textManager->DrawString("SHALLOW", m_position.x - 190.0f, m_position.y - 8.0f,
                isShallow ? 0xFF44FFFF : 0xFF227777, s, FONT_MENU, true, FONT_STYLE_NORMAL, textDepth);
            m_textManager->DrawString("LAND", m_position.x + 140.0f, m_position.y - 8.0f,
                isLand ? 0xFF44FF44 : 0xFF227722, s, FONT_MENU, true, FONT_STYLE_NORMAL, textDepth);
        }
    }
}

void WeightMenu::Open(BYTE activeWeight)
{
    m_selectedWeight = activeWeight;
    m_isVisible = true;
    m_animationTime = 0.0f;
}

void WeightMenu::Close()
{
    m_isVisible = false;
}

} // namespace UI
