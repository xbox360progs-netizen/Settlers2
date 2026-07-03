#pragma once
#include <vector>
#include <string>
#include <memory>
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/RenderQueue.h"
#include "MenuModel.h"

class Camera;
class SpriteAtlas;
class Renderer;
class TextManager;
namespace Input { class Gamepad; }
namespace UI { class LocalizationService; }

using Graphics::SpriteRenderer;

class GridMenu
{
public:
    struct TileUV {
        float u0, v0, u1, v1;
    };
    // Set all tile UVs for atlas-based rendering and start index
    void SetAllTileUVs(const std::vector<TileUV>& allUVs);
    void SetWindowStart(int start);
    int GetWindowStart() const { return m_atlasStart; }
    unsigned int GetTotalTileCount() const { return (unsigned int)m_atlasTotal; }
    void UpdateTileWindow();
    void NextWindow();
    void PrevWindow();
private:
    static const float kBaseCellSize;
    static const float kMenuScale;
    static const int kGridCols;
    static const int kGridRows;
    static const int kItemsPerPage;
    static const float kInputDelayTime;
    
    LPDIRECT3DTEXTURE9 m_atlasTexture;
    LPDIRECT3DTEXTURE9 m_backgroundTexture;
    SpriteRenderer* m_spriteRenderer;
    Renderer* m_renderer;
    Graphics::RenderQueue* m_renderQueue;
    LPDIRECT3DTEXTURE9 m_cellBackgroundTexture;

    TileUV m_backgroundUV;
    TileUV m_cellUV;
    std::vector<int> m_spriteIndices;
    std::vector<TileUV> m_tileUVs;
    std::shared_ptr<SpriteAtlas> m_iconAtlas;

    // Atlas windowing support
    std::vector<TileUV> m_allTileUVs;
    int m_atlasStart;
    int m_atlasTotal;
    int m_windowSize;
    // keep existing resolution for window by 4-step as requested
    int m_windowStep;

    int m_selectedIndex;
    int m_currentPage;
    int m_totalPages;
    int m_itemsPerPage;
    bool m_visible;
    bool m_selectionMade;
    float m_inputDelayTimer;

    float m_menuWidth;
    float m_menuHeight;
    float m_cellWidth;
    float m_cellHeight;
    float m_screenX;
    float m_screenY;

    void UpdateFromStick(float stickX, float stickY);
    void UpdateTileUVsForCurrentPage();

    WORD m_backgroundSlot;
    WORD m_cellSlot;
    WORD m_atlasSlot;

    float m_cellSpacingX;
    float m_cellSpacingY;
    float m_cellPadding;
    float m_cellVisualWidth;
    float m_cellVisualHeight;

    // Menu model for item data (labelId, action, enabled, visible)
    UI::MenuModel m_menuModel;
    const UI::LocalizationService* m_locService;

    // Backward compat: raw string labels for EditorScene (UI6 target)
    std::vector<std::string> m_cellLabels;
    TextManager* m_textManager;

    // Resolve text for a cell at global index
    const char* GetLabelText(int globalIndex) const;

public:
    GridMenu();
    ~GridMenu();

    bool Initialize();
    void Shutdown();
    
    void Show(float screenX, float screenY);
    void Hide();
    bool IsVisible() const { return m_visible; }

    void SetAtlasTexture(LPDIRECT3DTEXTURE9 atlasTexture);
    void SetBackgroundTexture(LPDIRECT3DTEXTURE9 backgroundTexture);
    void SetCellBackgroundTexture(LPDIRECT3DTEXTURE9 cellBackgroundTexture);
    void SetSpriteIndices(const std::vector<int>& spriteIndices);
    void SetTileUVs(const std::vector<TileUV>& tileUVs);
    void SetIconAtlas(std::shared_ptr<SpriteAtlas> atlas);
    void SetCellSpacing(float x, float y) { m_cellSpacingX = x; m_cellSpacingY = y; }
    void SetCellPadding(float padding) { m_cellPadding = padding; }
    void SetCellVisualSize(float w, float h) { m_cellVisualWidth = w; m_cellVisualHeight = h; }
    void SetCellLabels(const std::vector<std::string>& labels) { m_cellLabels = labels; }
    void SetTextManager(TextManager* tm) { m_textManager = tm; }
    void SetSpriteRenderer(SpriteRenderer* spriteRenderer);
    void SetRenderer(class Renderer* renderer);
    void SetRenderQueue(Graphics::RenderQueue* renderQueue) { m_renderQueue = renderQueue; }
    void SetTextureSlots(WORD bgSlot, WORD cellSlot, WORD atlasSlot) { m_backgroundSlot = bgSlot; m_cellSlot = cellSlot; m_atlasSlot = atlasSlot; }
    // Read-only accessors for rendering
    float GetScreenX() const { return m_screenX; }
    float GetScreenY() const { return m_screenY; }
    float GetMenuWidth() const { return m_menuWidth; }
    float GetMenuHeight() const { return m_menuHeight; }
    float GetCellSpacingX() const { return m_cellSpacingX; }
    float GetCellSpacingY() const { return m_cellSpacingY; }
    float GetCellPadding() const { return m_cellPadding; }
    float GetCellVisualWidth() const { return m_cellVisualWidth; }
    float GetCellVisualHeight() const { return m_cellVisualHeight; }
    int GetSelectedIndex() const { return m_selectedIndex; }
    const std::vector<TileUV>& GetTileUVs() const { return m_tileUVs; }
    const TileUV& GetBackgroundUV() const { return m_backgroundUV; }
    const TileUV& GetCellUV() const { return m_cellUV; }
    WORD GetBackgroundSlot() const { return m_backgroundSlot; }
    WORD GetCellSlot() const { return m_cellSlot; }
    WORD GetAtlasSlot() const { return m_atlasSlot; }
    static int GetGridCols() { return kGridCols; }
    static int GetGridRows() { return kGridRows; }
    // Background/cell UV sub-rects (for atlas-based bg/cell sprites)
    void SetBackgroundUV(const TileUV& uv) { m_backgroundUV = uv; }
    void SetCellUV(const TileUV& uv) { m_cellUV = uv; }
    void SetMenuSize(float w, float h) { m_menuWidth = w; m_menuHeight = h; }
    // New convenience: set all textures in one call (background, cell background, atlas)
    void SetTextures(LPDIRECT3DTEXTURE9 backgroundTexture, LPDIRECT3DTEXTURE9 cellBackgroundTexture, LPDIRECT3DTEXTURE9 atlasTexture);
    // New paging controls for atlas window
    // Sets a full list of tile UVs for the atlas and resets window to start

    // --- New MenuModel API (UI5b) ---
    void SetLocalizationService(const UI::LocalizationService* svc) { m_locService = svc; }
    void SetMenuItems(const UI::MenuItem* items, int count);
    UI::UiAction GetSelectedAction() const;

    void Update(Input::Gamepad* input, float deltaTime);
    void Render(const Camera* camera);
    void Render();
    
    bool HasSelection() const { return m_selectionMade; }
    int GetSelectedSpriteIndex() const;

    // Directly set tile UVs and global sprite indices for group-based loading
    void SetTileData(const std::vector<TileUV>& uvs, const std::vector<int>& globalIndices);
    void ResetSelection();
    
    void NextPage();
    void PrevPage();
    void ConfirmSelection();
    int GetCurrentPage() const { return m_currentPage; }
    int GetTotalPages() const { return m_totalPages; }
};
