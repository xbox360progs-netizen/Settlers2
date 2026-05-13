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
{
}

RadialMenu::~RadialMenu()
{
    Shutdown();
}

bool RadialMenu::Initialize()
{
    if (!m_device) {
        return false;
    }

    // Shader loading is now centralized in ShaderManager::Init()
    // RadialMenu no longer manages shader lifecycle

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

void RadialMenu::Render(SpriteRenderer* spriteRenderer)
{
    if (!m_visible || !spriteRenderer || !m_shaderManager) {
        return;
    }

    // Get a dummy texture from TextureRegistry (any texture will work since shader doesn't sample it)
    TextureRegistry& registry = TextureRegistry::instance();
    LPDIRECT3DTEXTURE9 dummyTexture = registry.getTextureOrLoad("menu_bd");
    if (!dummyTexture) {
        dummyTexture = registry.getTextureOrLoad("menu_cell");
    }
    if (!dummyTexture) {
        OutputDebugStringA("[RadialMenu] ERROR: No dummy texture available!\n");
        return;
    }

    // Set shader parameters before rendering
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
    m_shaderManager->CommitChanges();

    // Use SpriteRenderer with dummy texture for UI rendering
    spriteRenderer->Begin(SHADER_RADIALMENU, dummyTexture, 0.0f, 0, false);

    float menuRadius = kMenuSize * 0.5f;
    float size = menuRadius * 2.0f;

    // Draw quad with UV [0..1] - shader converts to [-1..1] for radial calculations
    spriteRenderer->Draw(
        m_screenX - menuRadius,
        m_screenY - menuRadius,
        size,
        size,
        0.0f, 0.0f, 1.0f, 1.0f,
        0xFFFFFFFF
    );

    spriteRenderer->End();
}

// Static callback for queue-based rendering
void RadialMenu::StaticDrawCallback(LPDIRECT3DDEVICE9 pDevice, ShaderManager* pShaderMgr, void* pUserData)
{
    RadialMenu* self = static_cast<RadialMenu*>(pUserData);
    if (!self) return;
    self->DrawRing(pDevice, pShaderMgr);
}

// Actual draw logic (called from queue via callback)
void RadialMenu::DrawRing(LPDIRECT3DDEVICE9 pDevice, ShaderManager* pShaderMgr)
{
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

    // Set shader parameters only (ShaderManager handles Begin/End pass)
    pShaderMgr->SetMatrix("WorldViewProjection", (float*)&matWVP);

    D3DXVECTOR4 menuParams((float)m_numSectors, (float)m_selectedIndex, m_innerRadius, m_outerRadius);
    D3DXVECTOR4 centerParams(m_centerRadius, 0.018f, 0.040f, 0.060f);

    pShaderMgr->SetVector("MenuParams", (float*)&menuParams);
    pShaderMgr->SetVector("CenterParams", (float*)&centerParams);
    pShaderMgr->SetVector("InnerColor", (float*)&m_innerColor);
    pShaderMgr->SetVector("OuterColor", (float*)&m_outerColor);
    pShaderMgr->SetVector("HighlightColor", (float*)&m_highlightColor);
    pShaderMgr->SetVector("LineColor", (float*)&m_lineColor);
    pShaderMgr->SetVector("CenterInnerColor", (float*)&m_centerInnerColor);
    pShaderMgr->SetVector("CenterOuterColor", (float*)&m_centerOuterColor);

    pShaderMgr->CommitChanges();

    m_quad->Render();

    // StateCache will handle state restoration automatically
    ShaderManager::StateCache* stateCache = pShaderMgr->GetStateCache();
    if (stateCache) stateCache->MarkDirty();
}

void RadialMenu::SetupRenderStates()
{
    if (!m_device || !m_shaderManager) {
        return;
    }

    // Use StateCache for centralized state management
    ShaderManager::StateCache* stateCache = m_shaderManager->GetStateCache();
    
    // Set render states through StateCache (tracks changes automatically)
    if (stateCache) {
        stateCache->SetRenderState(m_device, D3DRS_ALPHABLENDENABLE, TRUE);
        stateCache->SetRenderState(m_device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        stateCache->SetRenderState(m_device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        stateCache->SetRenderState(m_device, D3DRS_ZENABLE, FALSE);
        stateCache->SetRenderState(m_device, D3DRS_ZWRITEENABLE, FALSE);
        stateCache->SetRenderState(m_device, D3DRS_CULLMODE, D3DCULL_NONE);
    }
}

void RadialMenu::RestoreRenderStates()
{
    if (!m_device || !m_shaderManager) {
        return;
    }

    // StateCache will automatically reset states when needed
    // No manual restore needed - the queue system handles this
    ShaderManager::StateCache* stateCache = m_shaderManager->GetStateCache();
    stateCache->MarkDirty(); // Mark states as dirty for next batch
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

        // Use SHADER_UI for screen-space UI rendering
        spriteRenderer->Begin(SHADER_UI, texture, 0.1f, 0, true);

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

        spriteRenderer->End();
    }

    // Render center icon
    const MenuItem* centerItem = GetCenterItem();
    if (centerItem) {
        std::tr1::shared_ptr<SpriteAtlas> centerAtlas = TextureRegistry::instance().getAtlas(centerItem->atlasName);
        if (centerAtlas) {
            LPDIRECT3DTEXTURE9 centerTexture = centerAtlas->GetTexture();
            if (centerTexture) {
                spriteRenderer->Begin(SHADER_UI, centerTexture, 0.05f, 0, true); // Center icon: closest to viewer
                uint32_t centerSpriteIndex = centerItem->spriteIndex;
                const SpriteRegion* centerRegion = centerAtlas->GetRegion(centerSpriteIndex);
                if (centerRegion) {
                    const float centerDrawX = m_screenX - (centerRegion->width * 0.5f);
                    const float centerDrawY = m_screenY - (centerRegion->height * 0.5f);
                    spriteRenderer->Draw(centerDrawX, centerDrawY, (float)centerRegion->width, (float)centerRegion->height,
                                        centerRegion->u0, centerRegion->v0, centerRegion->u1, centerRegion->v1, 0xFFF6EBDD);
                }
                spriteRenderer->End();
            }
        }
    }

    // Restore GPU state after UI rendering (fixes sprite flickering on map)
    if (m_renderer) {
        m_renderer->RestoreFromUI();
    }

}
