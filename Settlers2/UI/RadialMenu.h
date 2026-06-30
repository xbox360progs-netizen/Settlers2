#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include <string>
#include "../Graphics/ShaderManager.h"
#include "../Graphics/SpriteAtlas.h"
#include "../Graphics/BinFileManager.h"
#include "../Graphics/RenderQueue.h"
#include "../Input/Gamepad.h"
#include "UiMessageId.h"
#include "UiAction.h"
#include "MenuModel.h"

using Graphics::ShaderManager;

class Quad;

class RadialMenu
{
public:
    struct MenuItem {
        UiMessageId labelId;
        UI::UiAction action;
        std::string spriteName;

        MenuItem(UiMessageId label, const UI::UiAction& act, const std::string& sprite)
            : labelId(label), action(act), spriteName(sprite) {}
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
    UI::UiAction GetSelectedAction() const;

    void ResetSelection();
    void ConfirmSelection();
    bool HasSelection() const { return m_selectionMade; }

    void Update(Input::Gamepad* gamepad);
    void UpdateFromStick(float stickX, float stickY);

    void Render();
    void RenderIcons(Graphics::RenderQueue* renderQueue);
    void RenderIconsDirect(LPDIRECT3DDEVICE9 device, Graphics::ShaderManager* shaderManager, LPDIRECT3DVERTEXDECLARATION9 spriteVertexDecl);

    void SetIconTextureSlot(WORD slot) { m_iconTextureSlot = slot; }

    void SetInnerColor(float r, float g, float b, float a) { m_innerColor = D3DXVECTOR4(r, g, b, a); }
    void SetOuterColor(float r, float g, float b, float a) { m_outerColor = D3DXVECTOR4(r, g, b, a); }
    void SetHighlightColor(float r, float g, float b, float a) { m_highlightColor = D3DXVECTOR4(r, g, b, a); }
    void SetLineColor(float r, float g, float b, float a) { m_lineColor = D3DXVECTOR4(r, g, b, a); }

private:
    void CalculateSelectedSector(float stickX, float stickY);

    LPDIRECT3DDEVICE9 m_device;
    ShaderManager* m_shaderManager;
    BinFileManager* m_binFileManager;
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
    bool m_selectionLatched;
    int m_confirmedIndex;

    D3DXVECTOR4 m_innerColor;
    D3DXVECTOR4 m_outerColor;
    D3DXVECTOR4 m_highlightColor;
    D3DXVECTOR4 m_lineColor;
    D3DXVECTOR4 m_centerInnerColor;
    D3DXVECTOR4 m_centerOuterColor;

    float m_screenX;
    float m_screenY;

    WORD m_iconTextureSlot;

    std::vector<MenuItem> m_items;
    UI::MenuModel m_menuModel;
};
