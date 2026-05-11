#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include <string>
#include "ShaderManager.h"
#include "SpriteRenderer.h"
#include "SpriteAtlas.h"
#include "BinFileManager.h"
#include "../Input/Gamepad.h"

class Quad;
class Renderer;

class RadialMenu
{
public:
    struct MenuItem {
        std::wstring name;
        std::string atlasName;
        uint32_t spriteIndex;
        int typeId;

        MenuItem(const std::string& atlas, uint32_t spriteIdx, int type = -1)
            : name(L""), atlasName(atlas), spriteIndex(spriteIdx), typeId(type) {}

        MenuItem(const std::wstring& itemName, int type, uint32_t spriteIdx)
            : name(itemName), atlasName("icon_menu"), spriteIndex(spriteIdx), typeId(type) {}
    };

    RadialMenu(LPDIRECT3DDEVICE9 device, ShaderManager* shaderManager, BinFileManager* binFileManager);
    ~RadialMenu();

    bool Initialize();
    void Shutdown();

    void Show(float screenX, float screenY);
    void Hide();
    bool IsVisible() const { return m_visible; }

    void AddItem(const MenuItem& item);
    void SetItems(const std::vector<MenuItem>& items);
    void ClearItems();

    const MenuItem* GetSelectedItem() const;
    const MenuItem* GetConfirmedItem() const;
    const MenuItem* GetCenterItem() const;
    int GetSelectedTypeId() const;

    void ResetSelection();
    void ConfirmSelection();
    bool HasSelection() const { return m_selectionMade; }

    void Update(Input::Gamepad* gamepad);
    void UpdateFromStick(float stickX, float stickY);

    void Render();
    void RenderIcons(SpriteRenderer* spriteRenderer);
    void SetRenderer(Renderer* renderer);
    
    // Queue-based rendering: static callback and instance method
    static void StaticDrawCallback(LPDIRECT3DDEVICE9 pDevice, ShaderManager* pShaderMgr, void* pUserData);
    void DrawRing(LPDIRECT3DDEVICE9 pDevice, ShaderManager* pShaderMgr);

    // Color customization
    void SetInnerColor(float r, float g, float b, float a) { m_innerColor = D3DXVECTOR4(r, g, b, a); }
    void SetOuterColor(float r, float g, float b, float a) { m_outerColor = D3DXVECTOR4(r, g, b, a); }
    void SetHighlightColor(float r, float g, float b, float a) { m_highlightColor = D3DXVECTOR4(r, g, b, a); }
    void SetLineColor(float r, float g, float b, float a) { m_lineColor = D3DXVECTOR4(r, g, b, a); }

private:
    void CalculateSelectedSector(float stickX, float stickY);
    void SetupRenderStates();
    void RestoreRenderStates();

    LPDIRECT3DDEVICE9 m_device;
    ShaderManager* m_shaderManager;
    BinFileManager* m_binFileManager;
    Renderer* m_renderer;
    Quad* m_quad;

    int m_selectedIndex;
    int m_numSectors;
    float m_innerRadius;
    float m_outerRadius;
    float m_centerRadius;
    float m_ringIconSize;
    float m_centerIconSize;

    bool m_visible;
    bool m_selectionMade;
    int m_confirmedIndex;

    D3DXVECTOR4 m_innerColor;
    D3DXVECTOR4 m_outerColor;
    D3DXVECTOR4 m_highlightColor;
    D3DXVECTOR4 m_lineColor;
    D3DXVECTOR4 m_centerInnerColor;
    D3DXVECTOR4 m_centerOuterColor;

    float m_screenX;
    float m_screenY;

    // Old state tracking removed - now using StateCache
    // DWORD m_oldZEnable;
    // DWORD m_oldAlphaBlend;
    // DWORD m_oldSrcBlend;
    // DWORD m_oldDestBlend;
    // DWORD m_oldCullMode;

    std::vector<MenuItem> m_items;
};
