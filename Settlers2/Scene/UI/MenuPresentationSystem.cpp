#include "stdafx.h"
#include "MenuPresentationSystem.h"
#include "../../UI/GridMenu.h"
#include "../../UI/UIMenu.h"
#include <algorithm>

namespace Scene {

MenuPresentationSystem::MenuPresentationSystem()
    : m_buildMenu(NULL)
    , m_flagMenu(NULL)
{
}

MenuPresentationSystem::~MenuPresentationSystem()
{
}

void MenuPresentationSystem::SetMenus(const GridMenu* buildMenu, const UIMenu* flagMenu)
{
    m_buildMenu = buildMenu;
    m_flagMenu = flagMenu;
}

void MenuPresentationSystem::BuildGridMenuQuads(RenderMenuPanel& panel) {
    if (!m_buildMenu) return;
    panel.buildMenuVisible = m_buildMenu->IsVisible();
    if (!panel.buildMenuVisible) return;

    float menuLeft = m_buildMenu->GetScreenX() - (m_buildMenu->GetMenuWidth() * 0.5f);
    float menuTop = m_buildMenu->GetScreenY() - (m_buildMenu->GetMenuHeight() * 0.5f);
    float cellSpacingX = m_buildMenu->GetCellSpacingX();
    float cellSpacingY = m_buildMenu->GetCellSpacingY();

    int cols = GridMenu::GetGridCols();
    int rows = GridMenu::GetGridRows();
    int totalSprites = (std::min)((int)m_buildMenu->GetTileUVs().size(), cols * rows);

    float gridWidth = cols * cellSpacingX;
    float gridHeight = rows * cellSpacingY;
    float gridOffsetX = (m_buildMenu->GetMenuWidth() - gridWidth) * 0.5f;
    float gridOffsetY = (m_buildMenu->GetMenuHeight() - gridHeight) * 0.5f;

    int& qc = panel.buildQuadCount;

    // 1. Background
    {
        RenderMenuQuad& q = panel.quads[qc++];
        q.x = menuLeft; q.y = menuTop;
        q.w = m_buildMenu->GetMenuWidth(); q.h = m_buildMenu->GetMenuHeight();
        const GridMenu::TileUV& bgUV = m_buildMenu->GetBackgroundUV();
        q.u0 = bgUV.u0; q.v0 = bgUV.v0; q.u1 = bgUV.u1; q.v1 = bgUV.v1;
        q.textureSlot = m_buildMenu->GetBackgroundSlot();
        q.depthLayer = 10;
    }

    // 2. Cell backgrounds + icons
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            int localIndex = row * cols + col;
            if (localIndex >= totalSprites) continue;

            float cellX = menuLeft + gridOffsetX + (col * cellSpacingX);
            float cellY = menuTop + gridOffsetY + (row * cellSpacingY);
            float cellOffsetX = (cellSpacingX - m_buildMenu->GetCellVisualWidth()) * 0.5f;
            float cellOffsetY = (cellSpacingY - m_buildMenu->GetCellVisualHeight()) * 0.5f;

            // Cell background
            {
                RenderMenuQuad& q = panel.quads[qc++];
                q.x = cellX + cellOffsetX;
                q.y = cellY + cellOffsetY;
                q.w = m_buildMenu->GetCellVisualWidth();
                q.h = m_buildMenu->GetCellVisualHeight();
                const GridMenu::TileUV& cUV = m_buildMenu->GetCellUV();
                q.u0 = cUV.u0; q.v0 = cUV.v0; q.u1 = cUV.u1; q.v1 = cUV.v1;
                q.textureSlot = m_buildMenu->GetCellSlot();
                q.depthLayer = 20;
            }

            // Icon
            {
                const GridMenu::TileUV& tUV = m_buildMenu->GetTileUVs()[localIndex];
                RenderMenuQuad& q = panel.quads[qc++];
                q.x = cellX + cellOffsetX + m_buildMenu->GetCellPadding();
                q.y = cellY + cellOffsetY + m_buildMenu->GetCellPadding();
                q.w = m_buildMenu->GetCellVisualWidth() - m_buildMenu->GetCellPadding() * 2.0f;
                q.h = m_buildMenu->GetCellVisualHeight() - m_buildMenu->GetCellPadding() * 2.0f;
                q.u0 = tUV.u0; q.v0 = tUV.v0; q.u1 = tUV.u1; q.v1 = tUV.v1;
                q.textureSlot = m_buildMenu->GetAtlasSlot();
                q.depthLayer = 100;
            }
        }
    }

    // 3. Selection highlight
    int selIdx = m_buildMenu->GetSelectedIndex();
    if (selIdx >= 0 && selIdx < totalSprites) {
        int selRow = selIdx / cols;
        int selCol = selIdx % cols;
        float selX = menuLeft + gridOffsetX + (selCol * cellSpacingX);
        float selY = menuTop + gridOffsetY + (selRow * cellSpacingY);
        float cellOffsetX = (cellSpacingX - m_buildMenu->GetCellVisualWidth()) * 0.5f;
        float cellOffsetY = (cellSpacingY - m_buildMenu->GetCellVisualHeight()) * 0.5f;

        float highlightSizeX = m_buildMenu->GetCellVisualWidth() + 4.0f;
        float highlightSizeY = m_buildMenu->GetCellVisualHeight() + 4.0f;
        float hOffX = (highlightSizeX - m_buildMenu->GetCellVisualWidth()) * 0.5f;
        float hOffY = (highlightSizeY - m_buildMenu->GetCellVisualHeight()) * 0.5f;

        // Yellow border
        {
            const GridMenu::TileUV& cUV = m_buildMenu->GetCellUV();
            RenderMenuQuad& q = panel.quads[qc++];
            q.x = selX + cellOffsetX - hOffX;
            q.y = selY + cellOffsetY - hOffY;
            q.w = highlightSizeX; q.h = highlightSizeY;
            q.u0 = cUV.u0; q.v0 = cUV.v0; q.u1 = cUV.u1; q.v1 = cUV.v1;
            q.textureSlot = m_buildMenu->GetCellSlot();
            q.color = 0xCCFFFF00;
            q.depthLayer = 150;
        }

        // White glow behind selected icon
        {
            const GridMenu::TileUV& tUV = m_buildMenu->GetTileUVs()[selIdx];
            RenderMenuQuad& q = panel.quads[qc++];
            q.x = selX + cellOffsetX - 6.0f;
            q.y = selY + cellOffsetY - 4.0f;
            q.w = m_buildMenu->GetCellVisualWidth() + 12.0f;
            q.h = m_buildMenu->GetCellVisualHeight() + 8.0f;
            q.u0 = tUV.u0; q.v0 = tUV.v0; q.u1 = tUV.u1; q.v1 = tUV.v1;
            q.textureSlot = m_buildMenu->GetAtlasSlot();
            q.color = 0x60FFFFFF;
            q.depthLayer = 110;
        }

        // Selected icon slightly larger
        {
            const GridMenu::TileUV& tUV = m_buildMenu->GetTileUVs()[selIdx];
            RenderMenuQuad& q = panel.quads[qc++];
            q.x = selX + cellOffsetX - 4.0f;
            q.y = selY + cellOffsetY - 2.0f;
            q.w = m_buildMenu->GetCellVisualWidth() + 8.0f;
            q.h = m_buildMenu->GetCellVisualHeight() + 4.0f;
            q.u0 = tUV.u0; q.v0 = tUV.v0; q.u1 = tUV.u1; q.v1 = tUV.v1;
            q.textureSlot = m_buildMenu->GetAtlasSlot();
            q.depthLayer = 130;
        }
    }
}

void MenuPresentationSystem::BuildFlagMenuQuads(RenderMenuPanel& panel) {
    if (!m_flagMenu) return;
    panel.flagMenuVisible = m_flagMenu->IsVisible();
    if (!panel.flagMenuVisible) return;

    int& qc = panel.flagQuadCount;
    int& lc = panel.flagLabelCount;

    // Background
    {
        const UIMenu::BackgroundData& bg = m_flagMenu->GetBackground();
        RenderMenuQuad& q = panel.quads[qc++];
        q.x = bg.x; q.y = bg.y; q.w = bg.w; q.h = bg.h;
        q.u0 = bg.u0; q.v0 = bg.v0; q.u1 = bg.u1; q.v1 = bg.v1;
        q.textureSlot = m_flagMenu->GetAtlasSlot();
        q.depthLayer = 10;
    }

    // Items
    int itemCount = m_flagMenu->GetItemCount();
    const UIMenu::ItemData* items = m_flagMenu->GetItems();
    int selIdx = m_flagMenu->GetSelectedIndex();

    for (int i = 0; i < itemCount; ++i) {
        const UIMenu::ItemData& item = items[i];

        // Selection highlight
        if (i == selIdx) {
            RenderMenuQuad& q = panel.quads[qc++];
            q.x = item.x - 4.0f;
            q.y = item.y - 4.0f;
            q.w = item.w + 8.0f;
            q.h = item.h + 8.0f;
            q.u0 = 0.5f; q.v0 = 0.5f; q.u1 = 0.5001f; q.v1 = 0.5001f;
            q.textureSlot = m_flagMenu->GetAtlasSlot();
            q.color = 0x64FFFF00; // D3DCOLOR_ARGB(100, 255, 255, 0)
            q.depthLayer = 50;
        }

        // Item sprite
        {
            RenderMenuQuad& q = panel.quads[qc++];
            q.x = item.x; q.y = item.y; q.w = item.w; q.h = item.h;
            q.u0 = item.u0; q.v0 = item.v0; q.u1 = item.u1; q.v1 = item.v1;
            q.textureSlot = m_flagMenu->GetAtlasSlot();
            q.depthLayer = 60;
        }

        // Item label
        if (item.label && item.label[0] != '\0') {
            RenderMenuLabel& lbl = panel.labels[lc++];
            lbl.x = item.x + item.w * 0.5f;
            lbl.y = item.y + item.h + 4.0f;
            lbl.color = (i == selIdx) ? 0xFFFFFF00 : 0xFFC8C8C8;
            lbl.size = 0.07f;
            strncpy_s(lbl.text, sizeof(lbl.text), item.label, _TRUNCATE);
        }
    }
}

void MenuPresentationSystem::BuildRenderFrame(RenderMenuPanel& panel) {
    panel.buildQuadCount = 0;
    panel.buildLabelCount = 0;
    panel.flagQuadCount = 0;
    panel.flagLabelCount = 0;
    panel.buildMenuVisible = false;
    panel.flagMenuVisible = false;

    BuildGridMenuQuads(panel);
    BuildFlagMenuQuads(panel);
}

} // namespace Scene
