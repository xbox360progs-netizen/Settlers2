#include "stdafx.h"
#include "RadialMenu.h"
#include "Quad.h"
#include "TextureRegistry.h"
#include <map>

namespace
{
    const float kBaseMenuSize = 256.0f;
    const float kMenuScale = 1.56f;
    const float kMenuSize = kBaseMenuSize * kMenuScale;
    const float kPi = 3.14159265f;

    float NormalizeAngle(float angle)
    {
        const float twoPi = 2.0f * kPi;
        while (angle < 0.0f) {
            angle += twoPi;
        }
        while (angle >= twoPi) {
            angle -= twoPi;
        }
        return angle;
    }

    float GetShaderSectorCenterAngle(int sectorIndex, int sectorCount)
    {
        if (sectorCount <= 0) {
            return 0.0f;
        }

        const float step = (2.0f * kPi) / (float)sectorCount;
        return NormalizeAngle(((float)sectorIndex + 0.5f) * step - (1.5f * kPi));
    }
}

const std::wstring RadialMenu::SHADER_PATH = L"game:\\Media\\Shaders\\RadialMenu.fx";

RadialMenu::RadialMenu(LPDIRECT3DDEVICE9 device, ShaderManager* shaderManager, BinFileManager* binFileManager)
    : m_device(device)
    , m_shaderManager(shaderManager)
    , m_binFileManager(binFileManager)
    , m_renderer(nullptr)
    , m_quad(nullptr)
    , m_selectedIndex(-1)
    , m_numSectors(8)
    , m_innerRadius(0.42f)
    , m_outerRadius(0.92f)
    , m_centerRadius(0.24f)
    , m_ringIconSize(42.0f * kMenuScale)
    , m_centerIconSize(54.0f * kMenuScale)
    , m_visible(false)
    , m_selectionMade(false)
    , m_confirmedIndex(-1)
    , m_innerColor(0.30f, 0.24f, 0.18f, 0.72f)
    , m_outerColor(0.10f, 0.08f, 0.06f, 0.88f)
    , m_highlightColor(0.90f, 0.58f, 0.20f, 0.92f)
    , m_lineColor(0.96f, 0.88f, 0.72f, 0.65f)
    , m_centerInnerColor(0.22f, 0.18f, 0.13f, 0.92f)
    , m_centerOuterColor(0.08f, 0.06f, 0.05f, 0.98f)
    , m_screenX(0.0f)
    , m_screenY(0.0f)
    , m_oldZEnable(0)
    , m_oldAlphaBlend(0)
    , m_oldSrcBlend(0)
    , m_oldDestBlend(0)
    , m_oldCullMode(0)
{
}

RadialMenu::~RadialMenu()
{
    Shutdown();
}

bool RadialMenu::Initialize()
{
    if (!m_device || !m_shaderManager) {
        return false;
    }

    // Load shader through ShaderManager
    char shaderPathA[512];
    WideCharToMultiByte(CP_ACP, 0, SHADER_PATH.c_str(), -1, shaderPathA, 512, NULL, NULL);
    
    if (FAILED(m_shaderManager->LoadShader("RadialMenu", shaderPathA, "RadialMenu"))) {
        return false;
    }

    m_quad = new Quad(m_device);
    if (!m_quad->Initialize(kMenuSize, kMenuSize)) {
        return false;
    }

    return true;
}

void RadialMenu::Shutdown()
{
    if (m_quad) {
        delete m_quad;
        m_quad = nullptr;
    }

    m_items.clear();
}

void RadialMenu::SetRenderer(Renderer* renderer)
{
    m_renderer = renderer;
}

void RadialMenu::Show(float screenX, float screenY)
{
    m_screenX = screenX;
    m_screenY = screenY;
    m_visible = true;
    m_selectedIndex = -1;
    m_selectionMade = false;
    m_confirmedIndex = -1;
}

void RadialMenu::Hide()
{
    m_visible = false;
    m_selectedIndex = -1;
}

void RadialMenu::AddItem(const MenuItem& item)
{
    m_items.push_back(item);
    m_numSectors = (int)m_items.size();
    if (m_numSectors < 4) m_numSectors = 4;
    if (m_numSectors > 12) m_numSectors = 12;
}

void RadialMenu::SetItems(const std::vector<MenuItem>& items)
{
    m_items = items;
    m_selectedIndex = -1;
    m_selectionMade = false;
    m_confirmedIndex = -1;
    m_numSectors = (int)m_items.size();
    if (m_numSectors < 4) m_numSectors = 4;
    if (m_numSectors > 12) m_numSectors = 12;
}

void RadialMenu::ClearItems()
{
    m_items.clear();
    m_selectedIndex = -1;
    m_selectionMade = false;
    m_confirmedIndex = -1;
    m_numSectors = 4;
}

const RadialMenu::MenuItem* RadialMenu::GetSelectedItem() const
{
    if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_items.size()) {
        return &m_items[m_selectedIndex];
    }
    return nullptr;
}

const RadialMenu::MenuItem* RadialMenu::GetConfirmedItem() const
{
    if (m_confirmedIndex >= 0 && m_confirmedIndex < (int)m_items.size()) {
        return &m_items[m_confirmedIndex];
    }
    return nullptr;
}

const RadialMenu::MenuItem* RadialMenu::GetCenterItem() const
{
    const MenuItem* selectedItem = GetSelectedItem();
    if (selectedItem) {
        return selectedItem;
    }
    if (!m_items.empty()) {
        return &m_items[0];
    }
    return nullptr;
}

int RadialMenu::GetSelectedTypeId() const
{
    const MenuItem* confirmedItem = GetConfirmedItem();
    return confirmedItem ? confirmedItem->typeId : -1;
}

void RadialMenu::ResetSelection()
{
    m_selectionMade = false;
    m_confirmedIndex = -1;
}

void RadialMenu::ConfirmSelection()
{
    if (!m_visible || m_selectedIndex < 0 || m_selectedIndex >= (int)m_items.size()) {
        return;
    }

    m_confirmedIndex = m_selectedIndex;
    m_selectionMade = true;
    Hide();
}

void RadialMenu::Update(Input::Gamepad* gamepad)
{
    if (!m_visible || !gamepad) {
        return;
    }

    if (gamepad->IsButtonPressed(Input::GP_B)) {
        Hide();
        return;
    }

    float stickX, stickY;
    gamepad->GetLeftStick(stickX, stickY);
    UpdateFromStick(stickX, stickY);

    if (gamepad->IsButtonPressed(Input::GP_A)) {
        ConfirmSelection();
    }
}

void RadialMenu::UpdateFromStick(float stickX, float stickY)
{
    if (!m_visible) {
        return;
    }

    float len = sqrtf(stickX * stickX + stickY * stickY);
    if (len > 0.3f) {
        CalculateSelectedSector(stickX, stickY);
    } else {
        m_selectedIndex = -1;
    }
}

void RadialMenu::CalculateSelectedSector(float stickX, float stickY)
{
    if (m_items.empty()) {
        return;
    }

    float angle = atan2f(-stickY, stickX);
    float normAngle = (angle + (1.5f * kPi)) / (2.0f * kPi);
    normAngle = normAngle - floorf(normAngle);
    m_selectedIndex = (int)(normAngle * m_numSectors) % m_numSectors;
}

void RadialMenu::Render()
{
    if (!m_visible || !m_quad || !m_device || !m_shaderManager) {
        return;
    }

    // Prepare render states for UI (called at start of Render as required)
    if (m_renderer) {
        m_renderer->PrepareForUI();
    }

    // Save current shader
    ShaderManager::Shader* prevShader = m_shaderManager->GetActiveShader();

    D3DVIEWPORT9 viewport;
    float screenWidth = 1280.0f;
    float screenHeight = 720.0f;
    if (SUCCEEDED(m_device->GetViewport(&viewport))) {
        screenWidth = (float)viewport.Width;
        screenHeight = (float)viewport.Height;
    }

    D3DXMATRIX world;
    D3DXMATRIX projection;
    D3DXMATRIX matWVP;
    D3DXMatrixIdentity(&world);
    D3DXMatrixTranslation(&world, m_screenX, m_screenY, 0.0f);
    D3DXMatrixOrthoOffCenterLH(&projection, 0, screenWidth, screenHeight, 0, 0, 1);
    matWVP = world * projection;

    SetupRenderStates();

    // Set RadialMenu shader
    m_shaderManager->SetActiveShader("RadialMenu");
    m_shaderManager->BeginShader();
    m_shaderManager->BeginPass(0);

    // Set shader parameters
    m_shaderManager->SetMatrix("WorldViewProjection", (float*)&matWVP);

    D3DXVECTOR4 menuParams((float)m_numSectors, (float)m_selectedIndex, m_innerRadius, m_outerRadius);
    D3DXVECTOR4 centerParams(m_centerRadius, 0.018f, 0.040f, 0.060f);

    m_shaderManager->SetVector("MenuParams", (float*)&menuParams);
    m_shaderManager->SetVector("CenterParams", (float*)&centerParams);
    m_shaderManager->SetVector("InnerColor", (float*)&m_innerColor);
    m_shaderManager->SetVector("OuterColor", (float*)&m_outerColor);
    m_shaderManager->SetVector("HighlightColor", (float*)&m_highlightColor);
    m_shaderManager->SetVector("LineColor", (float*)&m_lineColor);
    m_shaderManager->SetVector("CenterInnerColor", (float*)&m_centerInnerColor);
    m_shaderManager->SetVector("CenterOuterColor", (float*)&m_centerOuterColor);

    m_quad->Render();

    m_shaderManager->EndPass();
    m_shaderManager->EndShader();

    RestoreRenderStates();

    // Restore previous shader (if any)
    if (prevShader) {
        // Note: ShaderManager doesn't have a way to restore by pointer, 
        // so we rely on the caller (SpriteRenderer) to set its shader
        // before rendering icons
    }
}

void RadialMenu::SetupRenderStates()
{
    if (!m_device) {
        return;
    }

    m_device->GetRenderState(D3DRS_ZENABLE, &m_oldZEnable);
    m_device->GetRenderState(D3DRS_ALPHABLENDENABLE, &m_oldAlphaBlend);
    m_device->GetRenderState(D3DRS_SRCBLEND, &m_oldSrcBlend);
    m_device->GetRenderState(D3DRS_DESTBLEND, &m_oldDestBlend);
    m_device->GetRenderState(D3DRS_CULLMODE, &m_oldCullMode);

    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
}

void RadialMenu::RestoreRenderStates()
{
    if (!m_device) {
        return;
    }

    m_device->SetRenderState(D3DRS_ZENABLE, m_oldZEnable);
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, m_oldAlphaBlend);
    m_device->SetRenderState(D3DRS_SRCBLEND, m_oldSrcBlend);
    m_device->SetRenderState(D3DRS_DESTBLEND, m_oldDestBlend);
    m_device->SetRenderState(D3DRS_CULLMODE, m_oldCullMode);
}

void RadialMenu::RenderIcons(SpriteRenderer* spriteRenderer)
{
	char debugMsg[128];
    if (!spriteRenderer) {
        OutputDebugStringA("[RadialMenu] RenderIcons: spriteRenderer is NULL!\n");
        return;
    }

    const float ringRadius = ((m_innerRadius + m_outerRadius) * 0.5f) * (kMenuSize * 0.5f) * 1.0f;

    // Group icons by atlas to minimize texture switches
    std::map<std::string, std::vector<int>> atlasToIndices;
    for (int i = 0; i < (int)m_items.size(); ++i) {
        atlasToIndices[m_items[i].atlasName].push_back(i);
    }

    // Render each atlas group
    std::map<std::string, std::vector<int>>::iterator it;
    for (it = atlasToIndices.begin(); it != atlasToIndices.end(); ++it) {
        const std::string& atlasName = it->first;
        std::vector<int>& indices = it->second;


        std::tr1::shared_ptr<SpriteAtlas> atlas = TextureRegistry::instance().getAtlas(atlasName);
        if (!atlas) {
            sprintf(debugMsg, "[RadialMenu] Atlas not found: %s\n", atlasName.c_str());
            OutputDebugStringA(debugMsg);
            continue;
        }

        LPDIRECT3DTEXTURE9 texture = atlas->GetTexture();
        if (!texture) {
            sprintf(debugMsg, "[RadialMenu] Texture not found for atlas: %s\n", atlasName.c_str());
            OutputDebugStringA(debugMsg);
            continue;
        }


        // Validate texture surface
        D3DSURFACE_DESC desc;
        if (FAILED(texture->GetLevelDesc(0, &desc))) {
            OutputDebugStringA("[RadialMenu] ERROR: Texture surface is invalid!\n");
            continue;
        }

        // Use simple sprite shader (reliable for PowerPC)
        spriteRenderer->Begin("sprite", texture);

        // Draw all icons from this atlas
        for (size_t j = 0; j < indices.size(); ++j) {
            int i = indices[j];
            const float angle = GetShaderSectorCenterAngle(i, m_numSectors);
            float centerX = m_screenX + cosf(angle) * ringRadius;
            float centerY = m_screenY - sinf(angle) * ringRadius;

            uint32_t spriteIndex = m_items[i].spriteIndex;
            const SpriteRegion* region = atlas->GetRegion(spriteIndex);
            if (!region) continue;

            const float drawX = centerX - (region->width * 0.5f);
            const float drawY = centerY - (region->height * 0.5f);
            spriteRenderer->Draw(drawX, drawY, (float)region->width, (float)region->height,
                                region->u0, region->v0, region->u1, region->v1, 0xFFFFFFFF);
        }

        spriteRenderer->Flush(); // CRITICAL: Flush immediately after each atlas to prevent texture overwrite
        spriteRenderer->End();
    }

    // Render center icon
    const MenuItem* centerItem = GetCenterItem();
    if (centerItem) {
        std::tr1::shared_ptr<SpriteAtlas> centerAtlas = TextureRegistry::instance().getAtlas(centerItem->atlasName);
        if (centerAtlas) {
            LPDIRECT3DTEXTURE9 centerTexture = centerAtlas->GetTexture();
            if (centerTexture) {
                spriteRenderer->Begin("sprite", centerTexture);
                uint32_t centerSpriteIndex = centerItem->spriteIndex;
                const SpriteRegion* centerRegion = centerAtlas->GetRegion(centerSpriteIndex);
                if (centerRegion) {
                    const float centerDrawX = m_screenX - (centerRegion->width * 0.5f);
                    const float centerDrawY = m_screenY - (centerRegion->height * 0.5f);
                    spriteRenderer->Draw(centerDrawX, centerDrawY, (float)centerRegion->width, (float)centerRegion->height,
                                        centerRegion->u0, centerRegion->v0, centerRegion->u1, centerRegion->v1, 0xFFF6EBDD);
                }
                spriteRenderer->Flush(); // CRITICAL: Flush immediately after center icon to prevent texture overwrite
                spriteRenderer->End();
            }
        }
    }

    // Restore GPU state after UI rendering (fixes sprite flickering on map)
    if (m_renderer) {
        m_renderer->RestoreFromUI();
    }

}
