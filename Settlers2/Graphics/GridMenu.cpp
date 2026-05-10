#include "stdafx.h"
#include "GridMenu.h"
#include "../Graphics/SpriteAtlas.h"
#include "../Graphics/Quad.h"
#include "../Graphics/Camera.h"
#include "../Input/Gamepad.h"
// SpriteRenderer header is located in Shaders directory in this project
#include "../Shaders/SpriteRenderer.h"

namespace
{
    const std::wstring kShaderPaths[] = {
        L"game:\\Media\\Shaders\\GridMenu.fx",
        L"D:\\Media\\Shaders\\GridMenu.fx"
    };

    bool TryGetAtlasFrameUV(const std::shared_ptr<SpriteAtlas>& atlas, int index, D3DXVECTOR4& outUV)
    {
        if (!atlas || index < 0) return false;

        uint32_t count = atlas->GetRegionCount();
        if ((size_t)index < count) {
            const SpriteRegion* region = atlas->GetRegion((uint32_t)index);
            if (!region) return false;
            outUV.x = region->u0;
            outUV.y = region->v0;
            outUV.z = region->u1;
            outUV.w = region->v1;
            return outUV.z > outUV.x && outUV.w > outUV.y;
        }

        return false;
    }
}

const std::wstring GridMenu::SHADER_PATH = L"game:\\Media\\Shaders\\GridMenu.fx";
const float GridMenu::kBaseCellSize = 80.0f;
const float GridMenu::kMenuScale = 1.2f;
const int GridMenu::kGridCols = 4;
const int GridMenu::kGridRows = 4;
const int GridMenu::kItemsPerPage = 16;
const float GridMenu::kInputDelayTime = 0.3f;

GridMenu::GridMenu(LPDIRECT3DDEVICE9 device)
    : m_device(device)
    , m_vs(nullptr), m_ps(nullptr)
    , m_vsConsts(nullptr), m_psConsts(nullptr)
    , m_highlightQuad(nullptr), m_backgroundQuad(nullptr)
    , m_selectionBorderQuad(nullptr)
    , m_atlasTexture(nullptr), m_backgroundTexture(nullptr)
    , m_spriteRenderer(nullptr)
    , m_renderer(nullptr)
    , m_cellBackgroundTexture(nullptr)
    , m_selectedSpriteIndex(-1)
    , m_selectedIndex(0)
    , m_currentPage(0)
    , m_totalPages(1)
    , m_itemsPerPage(kItemsPerPage)
    , m_visible(false), m_selectionMade(false)
    , m_inputDelayTimer(0.0f)
    , m_menuWidth(kGridCols * kBaseCellSize + 32.0f)
    , m_menuHeight(kGridRows * kBaseCellSize + 64.0f)
    , m_cellWidth(kBaseCellSize)
    , m_cellHeight(kBaseCellSize)
    , m_screenX(0.0f), m_screenY(0.0f)
    , m_atlasStart(0)
    , m_atlasTotal(0)
    , m_windowSize(16)
    , m_windowStep(16)
{
}

GridMenu::~GridMenu()
{
    Shutdown();
}

bool GridMenu::Initialize()
{
    if (!m_device) return false;
    // REMOVED: LoadShaders() - GridMenu no longer manages shaders directly
    // SpriteRenderer handles all shader logic now

    m_backgroundQuad = new Quad(m_device);
    if (!m_backgroundQuad->Initialize(m_menuWidth, m_menuHeight)) return false;

    m_cellQuads.resize(kItemsPerPage);
    for (int i = 0; i < kItemsPerPage; i++) {
        m_cellQuads[i] = new Quad(m_device);
        if (!m_cellQuads[i]->Initialize(m_cellWidth, m_cellHeight)) return false;
    }

    m_highlightQuad = new Quad(m_device);
    if (!m_highlightQuad->Initialize(m_cellWidth * 1.1f, m_cellHeight * 1.0f)) return false;

    return true;
}

void GridMenu::Shutdown()
{
    for (size_t i = 0; i < m_cellQuads.size(); i++) {
        if (m_cellQuads[i]) {
            delete m_cellQuads[i];
            m_cellQuads[i] = nullptr;
        }
    }
    m_cellQuads.clear();

    if (m_backgroundQuad) delete m_backgroundQuad;
    if (m_highlightQuad) delete m_highlightQuad;
    if (m_selectionBorderQuad) delete m_selectionBorderQuad;

    if (m_atlasTexture) m_atlasTexture->Release();
    if (m_backgroundTexture) m_backgroundTexture->Release();
    if (m_cellBackgroundTexture) m_cellBackgroundTexture->Release();

    m_spriteIndices.clear();
    m_tileUVs.clear();
}

void GridMenu::Show(float screenX, float screenY)
{
    m_screenX = screenX;
    m_screenY = screenY;
    m_visible = true;
    m_selectedIndex = 0;
    m_currentPage = 0;
    m_selectionMade = false;
    m_selectedSpriteIndex = -1;
}

void GridMenu::Hide()
{
    m_visible = false;
    ResetSelection();
}

void GridMenu::ResetSelection()
{
    m_selectionMade = false;
    m_selectedSpriteIndex = -1;
    m_selectedIndex = 0;
}

void GridMenu::SetAtlasTexture(LPDIRECT3DTEXTURE9 atlasTexture)
{
    if (m_atlasTexture) m_atlasTexture->Release();
    m_atlasTexture = atlasTexture;
    if (m_atlasTexture) m_atlasTexture->AddRef();
}

void GridMenu::SetBackgroundTexture(LPDIRECT3DTEXTURE9 backgroundTexture)
{
    if (m_backgroundTexture) m_backgroundTexture->Release();
    m_backgroundTexture = backgroundTexture;
    if (m_backgroundTexture) m_backgroundTexture->AddRef();
}

void GridMenu::SetCellBackgroundTexture(LPDIRECT3DTEXTURE9 cellBackgroundTexture)
{
    if (m_cellBackgroundTexture) m_cellBackgroundTexture->Release();
    m_cellBackgroundTexture = cellBackgroundTexture;
    if (m_cellBackgroundTexture) m_cellBackgroundTexture->AddRef();
}

void GridMenu::SetSpriteIndices(const std::vector<int>& spriteIndices)
{
    m_spriteIndices = spriteIndices;
    m_totalPages = (int)m_spriteIndices.size() / m_itemsPerPage;
    if ((int)m_spriteIndices.size() % m_itemsPerPage > 0) {
        m_totalPages++;
    }
    if (m_totalPages <= 0) m_totalPages = 1;
    
    m_selectedIndex = 0;
    m_currentPage = 0;
    m_selectedSpriteIndex = -1;
}

void GridMenu::SetTileUVs(const std::vector<TileUV>& tileUVs)
{
    m_tileUVs = tileUVs;
}

void GridMenu::SetIconAtlas(std::shared_ptr<SpriteAtlas> atlas)
{
    m_iconAtlas = atlas;
}

void GridMenu::SetTextures(LPDIRECT3DTEXTURE9 backgroundTexture, LPDIRECT3DTEXTURE9 cellBackgroundTexture, LPDIRECT3DTEXTURE9 atlasTexture)
{
    // Background texture
    if (m_backgroundTexture) m_backgroundTexture->Release();
    m_backgroundTexture = backgroundTexture;
    if (m_backgroundTexture) m_backgroundTexture->AddRef();

    // Cell background texture
    if (m_cellBackgroundTexture) m_cellBackgroundTexture->Release();
    m_cellBackgroundTexture = cellBackgroundTexture;
    if (m_cellBackgroundTexture) m_cellBackgroundTexture->AddRef();

    // Atlas texture for sprites
    if (m_atlasTexture) m_atlasTexture->Release();
    m_atlasTexture = atlasTexture;
    if (m_atlasTexture) m_atlasTexture->AddRef();
}

void GridMenu::SetAllTileUVs(const std::vector<TileUV>& allUVs)
{
    m_allTileUVs = allUVs;
    m_atlasStart = 0;
    m_atlasTotal = (int)allUVs.size();
    // Build initial window
    UpdateTileWindow();
}

void GridMenu::SetWindowStart(int start)
{
    if (start < 0) start = 0;
    m_atlasStart = start;
    if (m_atlasStart > m_atlasTotal - m_windowSize) m_atlasStart = max(0, m_atlasTotal - m_windowSize);
    UpdateTileWindow();
}

void GridMenu::UpdateTileWindow()
{
    m_tileUVs.clear();
    m_spriteIndices.clear();
    int end = m_atlasStart + m_windowSize;
    if (end > m_atlasTotal) end = m_atlasTotal;
    for (int i = m_atlasStart; i < end; ++i) {
        m_tileUVs.push_back(m_allTileUVs[i]);
        m_spriteIndices.push_back(i); // map to global index
    }
    // total pages for UI (optional use by external caller)
    if (m_windowSize > 0) {
        m_totalPages = (m_atlasTotal + m_windowSize - 1) / m_windowSize;
    } else {
        m_totalPages = 1;
    }
}

void GridMenu::NextWindow()
{
    if (m_atlasTotal <= m_windowSize) return;
    int next = m_atlasStart + m_windowStep;
    if (next >= m_atlasTotal) {
        next = 0;
    }
    if (next != m_atlasStart) {
        m_atlasStart = next;
        UpdateTileWindow();
    }
}

void GridMenu::PrevWindow()
{
    if (m_atlasTotal <= m_windowSize) return;
    int prev = m_atlasStart - m_windowStep;
    if (prev < 0) {
        prev = ((m_atlasTotal - 1) / m_windowStep) * m_windowStep;
        if (prev >= m_atlasTotal) prev = 0;
    }
    if (prev != m_atlasStart) {
        m_atlasStart = prev;
        UpdateTileWindow();
    }
}

void GridMenu::UpdateFromStick(float stickX, float stickY)
{
    if (!m_visible) return;

    float deadzone = 0.3f;
    bool moved = false;
    
    if (fabsf(stickX) > deadzone) {
        if (stickX > 0) {
            int row = m_selectedIndex / kGridCols;
            int col = m_selectedIndex % kGridCols;
            if (col < kGridCols - 1) {
                m_selectedIndex++;
                moved = true;
            }
        } else {
            int row = m_selectedIndex / kGridCols;
            int col = m_selectedIndex % kGridCols;
            if (col > 0) {
                m_selectedIndex--;
                moved = true;
            }
        }
    }
    
    if (fabsf(stickY) > deadzone && !moved) {
        if (stickY < 0) {
            int row = m_selectedIndex / kGridCols;
            int col = m_selectedIndex % kGridCols;
            if (row < kGridRows - 1) {
                m_selectedIndex += kGridCols;
                moved = true;
            }
        } else {
            int row = m_selectedIndex / kGridCols;
            int col = m_selectedIndex % kGridCols;
            if (row > 0) {
                m_selectedIndex -= kGridCols;
                moved = true;
            }
        }
    }
    
    int itemsOnCurrentPage = m_itemsPerPage;
    if ((m_currentPage + 1) * m_itemsPerPage > (int)m_spriteIndices.size()) {
        itemsOnCurrentPage = (int)m_spriteIndices.size() - m_currentPage * m_itemsPerPage;
    }
    if (itemsOnCurrentPage < 0) itemsOnCurrentPage = 0;
    
    if (m_selectedIndex >= itemsOnCurrentPage) {
        m_selectedIndex = itemsOnCurrentPage > 0 ? itemsOnCurrentPage - 1 : 0;
    }
    if (m_selectedIndex < 0) {
        m_selectedIndex = 0;
    }
}

void GridMenu::NextPage()
{
    if (m_currentPage < m_totalPages - 1) {
        m_currentPage++;
        m_selectedIndex = 0;
    }
}

void GridMenu::PrevPage()
{
    if (m_currentPage > 0) {
        m_currentPage--;
        m_selectedIndex = 0;
    }
}

void GridMenu::ConfirmSelection()
{
    if (!m_visible) return;

    if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_spriteIndices.size()) {
        m_selectedSpriteIndex = m_spriteIndices[m_selectedIndex];
        m_selectionMade = true;
    }
}

void GridMenu::Update(Input::Gamepad* input, float deltaTime)
{
    if (!m_visible || !input) return;

    m_inputDelayTimer += deltaTime;
    if (m_inputDelayTimer >= kInputDelayTime) {
        float stickX = 0.0f;
        float stickY = 0.0f;
        input->GetLeftStick(stickX, stickY);
        UpdateFromStick(stickX, stickY);
        if (fabsf(stickX) > 0.3f || fabsf(stickY) > 0.3f) {
            m_inputDelayTimer = 0.0f;
        }
    }
    if (input->IsButtonPressed(Input::GP_A)) ConfirmSelection();
    if (input->IsButtonPressed(Input::GP_B)) Hide();
}

// Setter for integration with SpriteRenderer-based rendering path
void GridMenu::SetSpriteRenderer(SpriteRenderer* spriteRenderer)
{
    m_spriteRenderer = spriteRenderer;
}

void GridMenu::SetRenderer(Renderer* renderer)
{
    m_renderer = renderer;
}

void GridMenu::Render(const Camera* camera)
{
    if (!m_visible) { OutputDebugStringA("[GridMenu] not visible\n"); return; }
    if (!m_spriteRenderer) { OutputDebugStringA("[GridMenu] spriteRenderer is NULL\n"); return; }

    // Prepare render states for UI (called at start of Render as required)
    if (m_renderer) {
        m_renderer->PrepareForUI();
    }

    // If SpriteRenderer-based rendering is available, render the background via SpriteRenderer
    if (m_spriteRenderer && m_backgroundTexture) {
        float screenX = m_screenX;
        float screenY = m_screenY;

        // CORRECT RENDER ORDER: Begin -> Background -> Flush -> Icons -> End
        m_spriteRenderer->Begin("sprite_constant_instanced", m_backgroundTexture);
        // m_spriteRenderer->Draw(screenX - (m_menuWidth * 0.5f), screenY - (m_menuHeight * 0.5f), m_menuWidth, m_menuHeight, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF);
        // m_spriteRenderer->Flush();

        // Start rendering without background first
        m_spriteRenderer->Begin("sprite_constant_instanced", m_cellBackgroundTexture ? m_cellBackgroundTexture : m_atlasTexture);

        // Draw per-cell backgrounds (if available)
        if (m_cellBackgroundTexture) {
            float cellSpacing = (kBaseCellSize + 48.0f) * 1.2f;
            for (int row = 0; row < kGridRows; ++row) {
                for (int col = 0; col < kGridCols; ++col) {
                    int localIndex = row * kGridCols + col;
                    if (localIndex >= (int)m_spriteIndices.size()) continue;
                    float cellX = screenX - (m_menuWidth * 0.5f) + (col * cellSpacing) + (m_cellWidth * 0.5f);
                    float cellY = screenY + (m_menuHeight * 0.5f) - 32.0f - (row * cellSpacing) - (m_cellHeight * 0.5f);
                    m_spriteRenderer->Draw(cellX, cellY, m_cellWidth, m_cellHeight, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF);
                }
            }
        }

        // CRITICAL: Flush before icons even if same texture
        m_spriteRenderer->Flush();

        // Draw icons from atlas
        if (m_atlasTexture && !m_tileUVs.empty()) {
            m_spriteRenderer->Begin("sprite_constant_instanced", m_atlasTexture);
            float cellSpacing = (kBaseCellSize + 48.0f) * 1.2f;
            for (int row = 0; row < kGridRows; ++row) {
                for (int col = 0; col < kGridCols; ++col) {
                    int localIndex = row * kGridCols + col;
                    if (localIndex >= (int)m_spriteIndices.size()) continue;
                    int spriteIndex = m_spriteIndices[localIndex];
                    if (spriteIndex < 0 || spriteIndex >= (int)m_tileUVs.size()) continue;

                    const TileUV& tileUV = m_tileUVs[localIndex];
                    float cellX = screenX - (m_menuWidth * 0.5f) + (col * cellSpacing) + (m_cellWidth * 0.5f);
                    float cellY = screenY + (m_menuHeight * 0.5f) - 32.0f - (row * cellSpacing) - (m_cellHeight * 0.5f);

                    m_spriteRenderer->Draw(cellX, cellY, m_cellWidth, m_cellHeight, tileUV.u0, tileUV.v0, tileUV.u1, tileUV.v1, 0xFFFFFFFF);
                }
            }
        }

        m_spriteRenderer->End();
        return;
    }

    // REMOVED: Old direct device render path - GridMenu now uses only SpriteRenderer
    // All texture management is handled by SpriteRenderer via unified shader
}

// New single SpriteRenderer-based render path (explicit rendering order)
void GridMenu::Render(SpriteRenderer* spriteRenderer)
{
    if (!m_visible || !spriteRenderer) {
        return;
    }

    char debugMsg[256];
    sprintf(debugMsg, "[GridMenu::Render] atlasTexture=%p, tileUVs.size()=%d, selectedIndex=%d, visible=%d, screenX=%.1f, screenY=%.1f\n",
            m_atlasTexture, (int)m_tileUVs.size(), m_selectedIndex, m_visible, m_screenX, m_screenY);
    OutputDebugStringA(debugMsg);

    float menuLeft = m_screenX - (m_menuWidth * 0.5f);
    float menuTop = m_screenY - (m_menuHeight * 0.5f);
    float cellSpacing = (kBaseCellSize + 48.0f) * kMenuScale;
    int totalSprites = min((int)m_tileUVs.size(), kItemsPerPage);

    sprintf(debugMsg, "[GridMenu::Render] menuDims=%.1fx%.1f, menuLeft=%.1f, menuTop=%.1f, cellSpacing=%.1f, totalSprites=%d\n",
            m_menuWidth, m_menuHeight, menuLeft, menuTop, cellSpacing, totalSprites);
    OutputDebugStringA(debugMsg);

    // 1. Background (menu_bd) - full menu area
    if (m_backgroundTexture) {
        OutputDebugStringA("[GridMenu::Render] Drawing background\n");
        spriteRenderer->Begin("sprite_constant_instanced", m_backgroundTexture);
        spriteRenderer->Draw(menuLeft, menuTop, m_menuWidth, m_menuHeight, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF);
        spriteRenderer->End();
    }

    // 2. Cell backgrounds (menu_cell) - 4x4 grid
    if (m_cellBackgroundTexture) {
        OutputDebugStringA("[GridMenu::Render] Drawing cell backgrounds\n");
        spriteRenderer->Begin("sprite_constant_instanced", m_cellBackgroundTexture);
        for (int row = 0; row < kGridRows; ++row) {
            for (int col = 0; col < kGridCols; ++col) {
                int localIndex = row * kGridCols + col;
                if (localIndex >= totalSprites) continue;
                float cellX = menuLeft + 16.0f + (col * cellSpacing);
                float cellY = menuTop + 32.0f + (row * cellSpacing);
                spriteRenderer->Draw(cellX, cellY, m_cellWidth, m_cellHeight, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF);
            }
        }
        spriteRenderer->End();
    }

    // 3. Icons from atlas (visible window)
    if (m_atlasTexture && !m_tileUVs.empty()) {
        sprintf(debugMsg, "[GridMenu::Render] Drawing %d icons from atlas (texture=%p)\n", totalSprites, m_atlasTexture);
        OutputDebugStringA(debugMsg);
        spriteRenderer->Begin("sprite_constant_instanced", m_atlasTexture);
        for (int i = 0; i < totalSprites; ++i) {
            int row = i / kGridCols;
            int col = i % kGridCols;
            const TileUV& tileUV = m_tileUVs[i];
            float cellX = menuLeft + 16.0f + (col * cellSpacing);
            float cellY = menuTop + 32.0f + (row * cellSpacing);
            
            // Debug first sprite only
            if (i == 0) {
                sprintf(debugMsg, "[GridMenu::Render] Icon[0]: pos=(%.1f, %.1f), size=%.1fx%.1f, UV=(%.3f,%.3f,%.3f,%.3f)\n",
                        cellX, cellY, m_cellWidth, m_cellHeight, tileUV.u0, tileUV.v0, tileUV.u1, tileUV.v1);
                OutputDebugStringA(debugMsg);
            }
            
            spriteRenderer->Draw(cellX, cellY, m_cellWidth, m_cellHeight, tileUV.u0, tileUV.v0, tileUV.u1, tileUV.v1, 0xFFFFFFFF);
        }
        spriteRenderer->End();
    }

    // 4. Highlight selected tile
    if (m_atlasTexture && m_selectedIndex >= 0 && m_selectedIndex < totalSprites) {
        OutputDebugStringA("[GridMenu::Render] Drawing highlight\n");
        const TileUV& selectedUV = m_tileUVs[m_selectedIndex];
        int row = m_selectedIndex / kGridCols;
        int col = m_selectedIndex % kGridCols;
        float highlightX = menuLeft + 16.0f + (col * cellSpacing);
        float highlightY = menuTop + 32.0f + (row * cellSpacing);
        float highlightW = m_cellWidth * 1.1f;
        float highlightH = m_cellHeight * 1.1f;
        spriteRenderer->Begin("sprite_constant_instanced", m_atlasTexture);
        spriteRenderer->Draw(highlightX, highlightY, highlightW, highlightH, selectedUV.u0, selectedUV.v0, selectedUV.u1, selectedUV.v1, 0xFFFFFF00);
        spriteRenderer->End();
    }
}

// Note: Removed duplicate Render(SpriteRenderer*) implementation to avoid conflict
