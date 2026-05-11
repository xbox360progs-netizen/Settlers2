#pragma once
#include <vector>
#include <string>
#include <memory>

class Camera;
class SpriteAtlas;
class SpriteRenderer;
class Renderer;
namespace Input { class Gamepad; }

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
    LPDIRECT3DTEXTURE9 m_cellBackgroundTexture;

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

    int m_selectedSpriteIndex;
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
    void SetSpriteRenderer(class SpriteRenderer* spriteRenderer);
    void SetRenderer(class Renderer* renderer);
    // New convenience: set all textures in one call (background, cell background, atlas)
    void SetTextures(LPDIRECT3DTEXTURE9 backgroundTexture, LPDIRECT3DTEXTURE9 cellBackgroundTexture, LPDIRECT3DTEXTURE9 atlasTexture);
    // New paging controls for atlas window
    // Sets a full list of tile UVs for the atlas and resets window to start

    void Update(Input::Gamepad* input, float deltaTime);
    void Render(const Camera* camera);
    // New render path using SpriteRenderer directly
    void Render(class SpriteRenderer* spriteRenderer);
    
    bool HasSelection() const { return m_selectionMade; }
    int GetSelectedSpriteIndex() const {
        if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_tileUVs.size()) {
            return m_atlasStart + m_selectedIndex;
        }
        return -1;
    }
    void ResetSelection();
    
    void NextPage();
    void PrevPage();
    void ConfirmSelection();
};
