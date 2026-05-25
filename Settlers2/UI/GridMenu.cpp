#include "stdafx.h"
#include "GridMenu.h"
#include "../Graphics/SpriteAtlas.h"
#include "../Graphics/Camera.h"
#include "../Input/Gamepad.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/RenderLayers.h"

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

const float GridMenu::kBaseCellSize = 48.0f;
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
    , m_menuWidth(704.0f)
    , m_menuHeight(768.0f)
    , m_cellWidth(kBaseCellSize)
    , m_cellHeight(kBaseCellSize)
    , m_screenX(0.0f), m_screenY(0.0f)
    , m_atlasStart(0)
    , m_atlasTotal(0)
    , m_windowSize(16)
    , m_windowStep(16)
    , m_backgroundSlot(0)
    , m_cellSlot(0)
    , m_atlasSlot(0)
    , m_cellSpacingX(119.0f)
    , m_cellSpacingY(74.0f)
{
    m_backgroundUV.u0 = 0.0f; m_backgroundUV.v0 = 0.0f; m_backgroundUV.u1 = 1.0f; m_backgroundUV.v1 = 1.0f;
    m_cellUV.u0 = 0.0f; m_cellUV.v0 = 0.0f; m_cellUV.u1 = 1.0f; m_cellUV.v1 = 1.0f;
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

void GridMenu::SetTileData(const std::vector<TileUV>& uvs, const std::vector<int>& globalIndices)
{
    m_allTileUVs = uvs;
    m_spriteIndices = globalIndices;
    m_atlasStart = 0;
    m_atlasTotal = (int)uvs.size();
    m_windowSize = kItemsPerPage;
    m_currentPage = 0;
    m_selectedIndex = 0;
    m_totalPages = (m_atlasTotal + kItemsPerPage - 1) / kItemsPerPage;
    if (m_totalPages < 1) m_totalPages = 1;
    UpdateTileUVsForCurrentPage();
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
            if (row > 0) {
                m_selectedIndex -= kGridCols;
                moved = true;
            }
        } else {
            int row = m_selectedIndex / kGridCols;
            int col = m_selectedIndex % kGridCols;
            if (row < kGridRows - 1) {
                m_selectedIndex += kGridCols;
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

void GridMenu::UpdateTileUVsForCurrentPage()
{
    m_tileUVs.clear();
    int start = m_currentPage * kItemsPerPage;
    int end = start + kItemsPerPage;
    if (end > m_atlasTotal) end = m_atlasTotal;
    for (int i = start; i < end; ++i) {
        m_tileUVs.push_back(m_allTileUVs[i]);
    }
}

void GridMenu::NextPage()
{
    if (m_currentPage < m_totalPages - 1) {
        m_currentPage++;
        m_selectedIndex = 0;
        UpdateTileUVsForCurrentPage();
    }
}

void GridMenu::PrevPage()
{
    if (m_currentPage > 0) {
        m_currentPage--;
        m_selectedIndex = 0;
        UpdateTileUVsForCurrentPage();
    }
}

void GridMenu::ConfirmSelection()
{
    if (!m_visible) return;

    int globalIndex = m_currentPage * kItemsPerPage + m_selectedIndex;
    if (globalIndex >= 0 && globalIndex < (int)m_spriteIndices.size()) {
        m_selectedSpriteIndex = m_spriteIndices[globalIndex];
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

    Render();
}

// Queue-based render path
void GridMenu::Render()
{
    if (!m_visible || !m_renderQueue) {
        return;
    }

    char debugMsg[256];
    sprintf(debugMsg, "[GridMenu::Render] atlasTexture=%p, tileUVs.size()=%d, selectedIndex=%d, visible=%d, screenX=%.1f, screenY=%.1f\n",
            m_atlasTexture, (int)m_tileUVs.size(), m_selectedIndex, m_visible, m_screenX, m_screenY);
    OutputDebugStringA(debugMsg);

    float menuLeft = m_screenX - (m_menuWidth * 0.5f);
    float menuTop = m_screenY - (m_menuHeight * 0.5f);
    float cellSpacingX = m_cellSpacingX;
    float cellSpacingY = m_cellSpacingY;
    int totalSprites = min((int)m_tileUVs.size(), kItemsPerPage);

    float gridWidth = kGridCols * cellSpacingX;
    float gridHeight = kGridRows * cellSpacingY;
    float gridOffsetX = (m_menuWidth - gridWidth) * 0.5f;
    float gridOffsetY = (m_menuHeight - gridHeight) * 0.5f;

    sprintf(debugMsg, "[GridMenu::Render] menuDims=%.1fx%.1f, menuLeft=%.1f, menuTop=%.1f, cellSpacingX=%.1f cellSpacingY=%.1f, totalSprites=%d gridOff=%.1f,%.1f\n",
            m_menuWidth, m_menuHeight, menuLeft, menuTop, cellSpacingX, cellSpacingY, totalSprites, gridOffsetX, gridOffsetY);
    OutputDebugStringA(debugMsg);

    // 1. Background (menu_background_cell from maptiles) - full menu area (depth=150, behind cells, UI layer)
    if (m_backgroundTexture) {
        OutputDebugStringA("[GridMenu::Render] Submitting background command to queue\n");
        Graphics::RenderCommand cmd = {};
        cmd.x = menuLeft;
        cmd.y = menuTop;
        cmd.width = m_menuWidth;
        cmd.height = m_menuHeight;
        cmd.u0 = m_backgroundUV.u0; cmd.v0 = m_backgroundUV.v0;
        cmd.u1 = m_backgroundUV.u1; cmd.v1 = m_backgroundUV.v1;
        cmd.color = 0xFFFFFFFF;
        cmd.shaderID = SHADER_UI;
        cmd.textureID = m_backgroundSlot;
        cmd.blendMode = 1;
        cmd.layer = LAYER_UI;
        cmd.depth = 150;
        cmd.sortKey = Graphics::BuildSortKey(LAYER_UI, 1, SHADER_UI, m_backgroundSlot, 150);

        m_renderQueue->Submit(cmd);
    } else {
        OutputDebugStringA("[GridMenu::Render] WARNING: Background texture is NULL, skipping background render\n");
    }

    // 2. Cell backgrounds (menu_cell from maptiles) - 4x4 grid (depth=120, behind icons, UI layer)
    if (m_cellBackgroundTexture) {
        OutputDebugStringA("[GridMenu::Render] Submitting cell background commands to queue\n");
        for (int row = 0; row < kGridRows; ++row) {
            for (int col = 0; col < kGridCols; ++col) {
                int localIndex = row * kGridCols + col;
                if (localIndex >= totalSprites) continue;

                float cellX = menuLeft + gridOffsetX + (col * cellSpacingX);
                float cellY = menuTop + gridOffsetY + (row * cellSpacingY);

                Graphics::RenderCommand cmd = {};
                cmd.x = cellX;
                cmd.y = cellY;
                cmd.width = cellSpacingX - 2.0f;
                cmd.height = cellSpacingY - 2.0f;
                cmd.u0 = m_cellUV.u0; cmd.v0 = m_cellUV.v0;
                cmd.u1 = m_cellUV.u1; cmd.v1 = m_cellUV.v1;
                cmd.color = 0xFFFFFFFF;
                cmd.shaderID = SHADER_UI;
                cmd.textureID = m_cellSlot;
                cmd.blendMode = 1;
                cmd.layer = LAYER_UI;
                cmd.depth = 120;
                cmd.sortKey = Graphics::BuildSortKey(LAYER_UI, 1, SHADER_UI, m_cellSlot, 120);

                m_renderQueue->Submit(cmd);
            }
        }
    } else {
        OutputDebugStringA("[GridMenu::Render] WARNING: Cell background texture is NULL, skipping cell backgrounds\n");
    }

    // 3. Icons from atlas (visible window) (depth=100, UI layer)
    if (m_atlasTexture && !m_tileUVs.empty()) {
        sprintf(debugMsg, "[GridMenu::Render] Submitting %d icon commands to queue\n", totalSprites);
        OutputDebugStringA(debugMsg);
        for (int i = 0; i < totalSprites; i++) {
            int row = i / kGridCols;
            int col = i % kGridCols;
            const TileUV& tileUV = m_tileUVs[i];
            float cellX = menuLeft + gridOffsetX + (col * cellSpacingX);
            float cellY = menuTop + gridOffsetY + (row * cellSpacingY);

            Graphics::RenderCommand cmd = {};
            cmd.x = cellX;
            cmd.y = cellY;
            cmd.width = cellSpacingX - 2.0f;
            cmd.height = cellSpacingY - 2.0f;
            cmd.u0 = tileUV.u0; cmd.v0 = tileUV.v0;
            cmd.u1 = tileUV.u1; cmd.v1 = tileUV.v1;
            cmd.color = 0xFFFFFFFF;
            cmd.shaderID = SHADER_UI;
            cmd.textureID = m_atlasSlot;
            cmd.blendMode = 1;
            cmd.layer = LAYER_UI;
            cmd.depth = 100;
            cmd.sortKey = Graphics::BuildSortKey(LAYER_UI, 1, SHADER_UI, m_atlasSlot, 100);

            m_renderQueue->Submit(cmd);
        }
    } else {
        if (!m_atlasTexture) {
            OutputDebugStringA("[GridMenu::Render] WARNING: Atlas texture is NULL, skipping icons\n");
        }
        if (m_tileUVs.empty()) {
            OutputDebugStringA("[GridMenu::Render] WARNING: No tile UVs available, skipping icons\n");
        }
    }

    // 4. Selected cell highlight (depth=90, on top of everything else)
    if (m_selectedIndex >= 0 && m_selectedIndex < totalSprites) {
        int selRow = m_selectedIndex / kGridCols;
        int selCol = m_selectedIndex % kGridCols;
        float selX = menuLeft + gridOffsetX + (selCol * cellSpacingX);
        float selY = menuTop + gridOffsetY + (selRow * cellSpacingY);

        float highlightSizeX = cellSpacingX + 2.0f;
        float highlightSizeY = cellSpacingY + 2.0f;
        float highlightOffsetX = (highlightSizeX - (cellSpacingX - 2.0f)) * 0.5f;
        float highlightOffsetY = (highlightSizeY - (cellSpacingY - 2.0f)) * 0.5f;

        Graphics::RenderCommand cmd = {};
        cmd.x = selX - highlightOffsetX;
        cmd.y = selY - highlightOffsetY;
        cmd.width = highlightSizeX;
        cmd.height = highlightSizeY;
        cmd.u0 = m_cellUV.u0; cmd.v0 = m_cellUV.v0;
        cmd.u1 = m_cellUV.u1; cmd.v1 = m_cellUV.v1;
        cmd.color = 0xCCFFFF00;
        cmd.shaderID = SHADER_UI;
        cmd.textureID = m_cellSlot;
        cmd.blendMode = 1;
        cmd.layer = LAYER_UI;
        cmd.depth = 90;
        cmd.sortKey = Graphics::BuildSortKey(LAYER_UI, 1, SHADER_UI, m_cellSlot, 90);
        m_renderQueue->Submit(cmd);

        // White glow behind selected icon
        if (m_selectedIndex < (int)m_tileUVs.size()) {
            const TileUV& tileUV = m_tileUVs[m_selectedIndex];
            float glowPadX = 6.0f;
            float glowPadY = 4.0f;
            Graphics::RenderCommand glowCmd = {};
            glowCmd.x = selX - glowPadX;
            glowCmd.y = selY - glowPadY;
            glowCmd.width = cellSpacingX - 2.0f + glowPadX * 2.0f;
            glowCmd.height = cellSpacingY - 2.0f + glowPadY * 2.0f;
            glowCmd.u0 = tileUV.u0; glowCmd.v0 = tileUV.v0;
            glowCmd.u1 = tileUV.u1; glowCmd.v1 = tileUV.v1;
            glowCmd.color = 0x60FFFFFF;
            glowCmd.shaderID = SHADER_UI;
            glowCmd.textureID = m_atlasSlot;
            glowCmd.blendMode = 1;
            glowCmd.layer = LAYER_UI;
            glowCmd.depth = 85;
            glowCmd.sortKey = Graphics::BuildSortKey(LAYER_UI, 1, SHADER_UI, m_atlasSlot, 85);
            m_renderQueue->Submit(glowCmd);
        }

        // Draw selected icon slightly larger on top
        if (m_selectedIndex < (int)m_tileUVs.size()) {
            const TileUV& tileUV = m_tileUVs[m_selectedIndex];
            float iconPadX = 4.0f;
            float iconPadY = 2.0f;
            Graphics::RenderCommand iconCmd = {};
            iconCmd.x = selX - iconPadX;
            iconCmd.y = selY - iconPadY;
            iconCmd.width = cellSpacingX - 2.0f + iconPadX * 2.0f;
            iconCmd.height = cellSpacingY - 2.0f + iconPadY * 2.0f;
            iconCmd.u0 = tileUV.u0; iconCmd.v0 = tileUV.v0;
            iconCmd.u1 = tileUV.u1; iconCmd.v1 = tileUV.v1;
            iconCmd.color = 0xFFFFFFFF;
            iconCmd.shaderID = SHADER_UI;
            iconCmd.textureID = m_atlasSlot;
            iconCmd.blendMode = 1;
            iconCmd.layer = LAYER_UI;
            iconCmd.depth = 80;
            iconCmd.sortKey = Graphics::BuildSortKey(LAYER_UI, 1, SHADER_UI, m_atlasSlot, 80);
            m_renderQueue->Submit(iconCmd);
        }
    }
}
