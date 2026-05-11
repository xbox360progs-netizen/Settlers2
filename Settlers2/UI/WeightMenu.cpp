#include "stdafx.h"
#include "WeightMenu.h"
#include "../Graphics/Texture.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/TextManager.h"
#include "../Input/InputManager.h"

namespace UI {

WeightMenu::WeightMenu()
    : m_isVisible(false)
    , m_selectedWeight(World::Weight_Land)
    , m_animationTime(0.0f)
    , m_backgroundTexture(nullptr)
    , m_dpadCrossTexture(nullptr)
    , m_spriteRenderer(nullptr)
    , m_textManager(nullptr)
    , m_position(640.0f, 360.0f)
    , m_scale(1.0f)
{
}

WeightMenu::~WeightMenu()
{
}

bool WeightMenu::Initialize(Texture* background, Texture* dpadCross, SpriteRenderer* renderer, TextManager* textManager)
{
    m_backgroundTexture = background;
    m_dpadCrossTexture = dpadCross;
    m_spriteRenderer = renderer;
    m_textManager = textManager;
    
    return (m_backgroundTexture != nullptr && m_dpadCrossTexture != nullptr && 
            m_spriteRenderer != nullptr && m_textManager != nullptr);
}

void WeightMenu::Update(float deltaTime)
{
    if (!m_isVisible) {
        return;
    }

    m_animationTime += deltaTime;

    // Handle D-pad input for weight selection
    // This will be handled by the InputManager in EditorScene
    // The menu just displays the current selection and waits for the scene to update it
}

void WeightMenu::Render()
{
    if (!m_isVisible || !m_spriteRenderer) {
        return;
    }

    // Render background
    if (m_backgroundTexture) {
        D3DXVECTOR3 bgPos(m_position.x, m_position.y, 0.0f);
        // TODO: Submit render command for background texture
    }

    // Render D-pad cross
    if (m_dpadCrossTexture) {
        D3DXVECTOR3 crossPos(m_position.x, m_position.y, 0.01f);
        // TODO: Submit render command for D-pad cross texture
    }

    // Render weight labels
    if (m_textManager) {
        // Up: Block (3)
        m_textManager->DrawTextToScreen("Block: 3", m_position.x, m_position.y - 100.0f, 0xFFFF0000, 1.0f);
        // Down: Deep (0)
        m_textManager->DrawTextToScreen("Deep: 0", m_position.x, m_position.y + 100.0f, 0xFF0000FF, 1.0f);
        // Left: Shallow (1)
        m_textManager->DrawTextToScreen("Shallow: 1", m_position.x - 100.0f, m_position.y, 0xFF00FFFF, 1.0f);
        // Right: Land (2)
        m_textManager->DrawTextToScreen("Land: 2", m_position.x + 100.0f, m_position.y, 0xFF00FF00, 1.0f);
    }
}

void WeightMenu::Open()
{
    m_isVisible = true;
    m_animationTime = 0.0f;
}

void WeightMenu::Close()
{
    m_isVisible = false;
}

} // namespace UI
