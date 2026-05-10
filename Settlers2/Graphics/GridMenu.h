#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include <string>
#include <memory>

class Quad;
class Camera;
class SpriteAtlas;
class SpriteRenderer;
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
    
    LPDIRECT3DDEVICE9 m_device;
    IDirect3DVertexShader9* m_vs;
    IDirect3DPixelShader9* m_ps;
    ID3DXConstantTable* m_vsConsts;
    ID3DXConstantTable* m_psConsts;
    
    Quad* m_highlightQuad;
    Quad* m_backgroundQuad;
    std::vector<Quad*> m_cellQuads;
    Quad* m_selectionBorderQuad;

    LPDIRECT3DTEXTURE9 m_atlasTexture;    
    LPDIRECT3DTEXTURE9 m_backgroundTexture; 
    SpriteRenderer* m_spriteRenderer; 
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

    bool LoadShaders();
    void ReleaseShaders();
    void UpdateFromStick(float stickX, float stickY);

public:
    static const std::wstring SHADER_PATH;

    GridMenu(LPDIRECT3DDEVICE9 device);
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
