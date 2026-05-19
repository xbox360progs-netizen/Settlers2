#include "stdafx.h"
#include "GridMenu.h"
#include "../Graphics/SpriteAtlas.h"
#include "../Graphics/Camera.h"
#include "../Input/Gamepad.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/ShaderManager.h"

namespace
{
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

const float GridMenu::kBaseCellSize = 80.0f;
const float GridMenu::kMenuScale = 1.2f;
const int GridMenu::kGridCols = 4;
const int GridMenu::kGridRows = 4;
const int GridMenu::kItemsPerPage = 16;
const float GridMenu::kInputDelayTime = 0.3f;

GridMenu::GridMenu()
    : m_atlasTexture(nullptr), m_backgroundTexture(nullptr)
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
    // REMOVED: LoadShaders() - GridMenu no longer manages shaders directly
    // SpriteRenderer handles all shader logic now

    // Note: Quad objects require LPDIRECT3DDEVICE9 for initialization
    // Since GridMenu no longer holds the device, Quad initialization must be deferred
    // or handled externally. For now, we'll skip Quad creation and rely on queue-based rendering.
    
    return true;
}

void GridMenu::Shutdown()
{
    // No Quad objects to clean up (removed D3D dependency)
    
    // Release texture references
    if (m_backgroundTexture) {
        m_backgroundTexture->Release();
        m_backgroundTexture = nullptr;
    }
    if (m_cellBackgroundTexture) {
        m_cellBackgroundTexture->Release();
        m_cellBackgroundTexture = nullptr;
    }
    if (m_atlasTexture) {
        m_atlasTexture->Release();
        m_atlasTexture = nullptr;
    }
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
    if (!m_renderer) { OutputDebugStringA("[GridMenu] renderer is NULL\n"); return; }

    ShaderManager* shaderManager = m_renderer->GetShaderManager();
    if (!shaderManager) { OutputDebugStringA("[GridMenu] shaderManager is NULL\n"); return; }

    // Use the new queue-based render path
    Render(m_spriteRenderer);
}

// New single queue-based render path (submits RenderCommand to ShaderManager)
void GridMenu::Render(SpriteRenderer* spriteRenderer)
{
    if (!m_visible || !spriteRenderer || !m_renderer) {
        return;
    }

    ShaderManager* shaderManager = m_renderer->GetShaderManager();
    if (!shaderManager) {
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

    // 1. Background (menu_bd) - full menu area (depth=0.15, behind cells, UI screen-space)
    if (m_backgroundTexture) {
        OutputDebugStringA("[GridMenu::Render] Submitting background command to queue\n");
        ShaderManager::RenderCommand cmd;
        cmd.pTexture = m_backgroundTexture;
        cmd.shaderID = SHADER_UI;
        cmd.vertexStart = 0;
        cmd.vertexCount = 4;
        cmd.primitiveCount = 2;
        cmd.batchType = 0;
        cmd.depth = 0.15f;
        cmd.layer = 2; // UI layer
        cmd.isUI = true;
        cmd.customDraw = NULL;
        cmd.customUserData = NULL;
        
        // Store position for actual rendering (would need vertex buffer update in full implementation)
        // For now, this demonstrates the queue submission pattern
        shaderManager->Submit(cmd);
    } else {
        OutputDebugStringA("[GridMenu::Render] WARNING: Background texture is NULL, skipping background render\n");
    }

    // 2. Cell backgrounds (menu_cell) - 4x4 grid (depth=0.12, behind icons, UI screen-space)
    if (m_cellBackgroundTexture) {
        OutputDebugStringA("[GridMenu::Render] Submitting cell background commands to queue\n");
        for (int row = 0; row < kGridRows; ++row) {
            for (int col = 0; col < kGridCols; ++col) {
                int localIndex = row * kGridCols + col;
                if (localIndex >= totalSprites) continue;
                
                ShaderManager::RenderCommand cmd;
                cmd.pTexture = m_cellBackgroundTexture;
                cmd.shaderID = SHADER_UI;
                cmd.vertexStart = 0;
                cmd.vertexCount = 4;
                cmd.primitiveCount = 2;
                cmd.batchType = 0;
                cmd.depth = 0.12f;
                cmd.layer = 2;
                cmd.isUI = true;
                cmd.customDraw = NULL;
                cmd.customUserData = NULL;
                
                shaderManager->Submit(cmd);
            }
        }
    } else {
        OutputDebugStringA("[GridMenu::Render] WARNING: Cell background texture is NULL, skipping cell backgrounds\n");
    }

    // 3. Icons from atlas (visible window) (depth=0.1, UI layer, UI screen-space)
    if (m_atlasTexture && !m_tileUVs.empty()) {
        sprintf(debugMsg, "[GridMenu::Render] Submitting %d icon commands to queue (texture=%p)\n", totalSprites, m_atlasTexture);
        OutputDebugStringA(debugMsg);
        for (int i = 0; i < totalSprites; ++i) {
            int row = i / kGridCols;
            int col = i % kGridCols;
            const TileUV& tileUV = m_tileUVs[i];
            float cellX = menuLeft + 16.0f + (col * cellSpacing);
            float cellY = menuTop + 32.0f + (row * cellSpacing);
            
            ShaderManager::RenderCommand cmd;
            cmd.pTexture = m_atlasTexture;
            cmd.shaderID = SHADER_UI;
            cmd.vertexStart = 0;
            cmd.vertexCount = 4;
            cmd.primitiveCount = 2;
            cmd.batchType = 0;
            cmd.depth = 0.1f;
            cmd.layer = 2;
            cmd.isUI = true;
            cmd.customDraw = NULL;
            cmd.customUserData = NULL;
            
            shaderManager->Submit(cmd);
        }
    } else {
        if (!m_atlasTexture) {
            OutputDebugStringA("[GridMenu::Render] WARNING: Atlas texture is NULL, skipping icons\n");
        }
        if (m_tileUVs.empty()) {
            OutputDebugStringA("[GridMenu::Render] WARNING: No tile UVs available, skipping icons\n");
        }
    }
}
