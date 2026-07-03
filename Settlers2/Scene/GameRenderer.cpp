#include "stdafx.h"
#include "GameRenderer.h"
#include "../World/Map.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/SpriteAtlas.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/TextureRegistry.h"
#include "../Graphics/TextManager.h"
#include "../Graphics/Camera.h"
#include "../Graphics/TileRenderer.h"
#include "../Graphics/ShaderManager.h"
#include "TextureSlots.h"
#include "Rendering/RenderContext.h"

namespace Scene {

// ─── Pass wrapper implementations ──────────────────────────────────────

void BuildingRenderPass::Execute(const RenderFrame& frame, const RenderContext& context, RenderCommandBuffer& buffer)
{
    m_renderer.Render(buffer, frame);
}

void SettlerRenderPass::Execute(const RenderFrame& frame, const RenderContext& context, RenderCommandBuffer& buffer)
{
    m_renderer.Render(buffer, frame);
}

// ─── GameRenderer ──────────────────────────────────────────────────────



// ─── GameRenderer constructor ──────────────────────────────────────────
GameRenderer::GameRenderer(
    TileRenderer*     tileRenderer,
    Renderer*         renderer,
    Camera* camera,
    World::Map*       map,
    TextManager*                textManager
)
    : m_tileRenderer(tileRenderer)
    , m_renderer(renderer)
    , m_camera(camera)
    , m_map(map)
    , m_textManager(textManager)
    , m_terrainPass(*tileRenderer)
    , m_buildingRenderPass(m_buildingRenderer)
    , m_settlerRenderPass(m_settlerRenderer)
    , m_huntingSpotPass(textManager)
    , m_confirmationMenuPass(textManager)
    , m_notificationPass(textManager)
    , m_townHallPanelPass(textManager)
    , m_logisticsDebugPass(textManager)
    , m_resourceHudPass(textManager)
    , m_bannerPass(textManager)
    , m_menuPass(textManager)
    , m_groundResourcePass(textManager)
{
    // Execute order: Background → Terrain → Buildings → RoadConnections → RoadPreview → PlacementPreview → Settlers → Wildlife → GroundResources → Workers → FlagResources → WorkSites → Overlays → HuntingSpots → UI → BuildingHighlight → TownHallPanel → ResourceHud → Banner → Cursor
    m_renderGraph.AddPass(&m_backgroundPass);
    m_renderGraph.AddPass(&m_terrainPass);
    m_renderGraph.AddPass(&m_buildingRenderPass);
    m_renderGraph.AddPass(&m_roadConnectionPass);
    m_renderGraph.AddPass(&m_roadPreviewPass);
    m_renderGraph.AddPass(&m_placementPreviewPass);
    m_renderGraph.AddPass(&m_settlerRenderPass);
    m_renderGraph.AddPass(&m_wildlifePass);
    m_renderGraph.AddPass(&m_groundResourcePass);
    m_renderGraph.AddPass(&m_workerPass);
    m_renderGraph.AddPass(&m_flagResourcePass);
    m_renderGraph.AddPass(&m_geologistOverlayPass);
    m_renderGraph.AddPass(&m_workSitePass);
    m_renderGraph.AddPass(&m_huntingSpotPass);
    m_renderGraph.AddPass(&m_confirmationMenuPass);
    m_renderGraph.AddPass(&m_notificationPass);
    m_renderGraph.AddPass(&m_menuPass);
    m_renderGraph.AddPass(&m_buildingHighlightPass);
    m_renderGraph.AddPass(&m_townHallPanelPass);
    m_renderGraph.AddPass(&m_resourceHudPass);
    m_renderGraph.AddPass(&m_bannerPass);
    m_renderGraph.AddPass(&m_logisticsDebugPass);
    m_renderGraph.AddPass(&m_cursorPass);
}

// ─── Render ────────────────────────────────────────────────────────────
void GameRenderer::Render(Graphics::RenderQueue* renderQueue, const RenderFrame& renderFrame)
{
    if (!m_tileRenderer || !m_map) {
        OutputDebugStringA("[GameRenderer::Render] Not ready, returning\n");
        return;
    }

    if (m_renderer) {
        m_renderer->Clear(0xFF000000); // Black
    }

    // ─── Set up atlas texture slots ────────────────────────────────────
    m_tileRenderer->ClearAtlasSlots();
    TextureRegistry& reg = TextureRegistry::instance();
    SpriteRenderer* spriteRenderer = m_renderer ? m_renderer->GetSpriteRenderer() : NULL;

    if (spriteRenderer) {
        WORD nextSlot = 1;
        for (int lt = 0; lt < static_cast<int>(World::LayerCount); ++lt) {
            World::TileLayer* layer = m_map->GetLayer(static_cast<World::LayerType>(lt));
            if (!layer) continue;
            int w = layer->GetWidth();
            int h = layer->GetHeight();
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    const World::Tile& tile = layer->GetTile(x, y);
                    if (tile.atlasName.empty()) continue;
                    if (m_tileRenderer->HasAtlasSlot(tile.atlasName)) continue;
                    LPDIRECT3DTEXTURE9 tex = reg.getTextureOrLoad(tile.atlasName);
                    if (!tex) continue;
                    LPDIRECT3DTEXTURE9 slotTex = tex;
                    std::tr1::shared_ptr<SpriteAtlas> atlas = reg.getAtlas(tile.atlasName);
                    if (atlas && atlas->GetTexture()) {
                        slotTex = atlas->GetTexture();
                    }
                    spriteRenderer->SetTextureSlot(nextSlot, slotTex);
                    m_tileRenderer->SetAtlasSlot(tile.atlasName, nextSlot);
                    nextSlot++;
                }
            }
        }
    }

    // Camera view-projection matrices
    if (m_camera) {
        m_camera->Update();
        D3DXMATRIX viewProj = m_camera->GetViewMatrix() * m_camera->GetProjectionMatrix();
        if (m_renderer) {
            Graphics::ShaderManager* sm = m_renderer->GetShaderManager();
            if (sm) {
                sm->UpdateGlobalMatrices(&m_camera->GetViewMatrix(), &m_camera->GetProjectionMatrix());
                sm->SetShaderMatrix(SHADER_TERRAIN, &viewProj);
                sm->SetShaderMatrix(SHADER_WORLD, &viewProj);
            }
        }
    }

    // Re-assert UI atlas texture slots
    if (spriteRenderer) {
        std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
        if (uiAtlas && uiAtlas->GetTexture()) {
            LPDIRECT3DTEXTURE9 uiTex = uiAtlas->GetTexture();
            spriteRenderer->SetTextureSlot(SLOT_UI_CURSOR, uiTex);
            spriteRenderer->SetTextureSlot(SLOT_UI_MENU_BG, uiTex);
            spriteRenderer->SetTextureSlot(SLOT_UI_MENU_CELL, uiTex);
            spriteRenderer->SetTextureSlot(SLOT_UI_TOWNHALL_PANEL, uiTex);
        }
        std::tr1::shared_ptr<SpriteAtlas> iconAtlas = reg.getAtlas("Icon");
        if (iconAtlas && iconAtlas->GetTexture()) {
            spriteRenderer->SetTextureSlot(SLOT_UI_MENU_ICON, iconAtlas->GetTexture());
        } else {
            reg.getTextureOrLoad("Icon");
            iconAtlas = reg.getAtlas("Icon");
            if (iconAtlas && iconAtlas->GetTexture())
                spriteRenderer->SetTextureSlot(SLOT_UI_MENU_ICON, iconAtlas->GetTexture());
            else
                spriteRenderer->SetTextureSlot(SLOT_UI_MENU_ICON, uiAtlas ? uiAtlas->GetTexture() : NULL);
        }
        std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
        if (buildingsAtlas && buildingsAtlas->GetTexture()) {
            spriteRenderer->SetTextureSlot(SLOT_BUILDINGS_HIGHLIGHT, buildingsAtlas->GetTexture());
        }

        // Background texture (loaded lazily by BackgroundPass, slot pre-bound)
        LPDIRECT3DTEXTURE9 bgTex = reg.getTextureOrLoad("background_game");
        if (bgTex) {
            spriteRenderer->SetTextureSlot(SLOT_BACKGROUND, bgTex);
        }

        // Streets atlas for road connection quads
        reg.getTextureOrLoad("streets");
        std::tr1::shared_ptr<SpriteAtlas> streetsAtlas = reg.getAtlas("streets");
        if (streetsAtlas && streetsAtlas->GetTexture()) {
            spriteRenderer->SetTextureSlot(SLOT_STREETS, streetsAtlas->GetTexture());
        }
    }

    // ─── Clear scene command buffer for this frame ─────────────────
    m_commandBuffer.Clear();

    // ─── Execute all registered render passes via RenderGraph ───────────
    // Each pass reads from RenderFrame + RenderContext and pushes to the buffer.
    // Order: TerrainPass → BuildingPass → RoadPreviewPass → PlacementPreviewPass → SettlerPass → WildlifePass → GroundResourcePass → WorkerPass → FlagResourcePass → GeologistOverlayPass → ConfirmationMenuPass → NotificationPass → CursorPass
    // Future: GamepadCursorPass, HudPass.
    if (spriteRenderer) {
        std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
        if (buildingsAtlas && buildingsAtlas->GetTexture()) {
            LPDIRECT3DTEXTURE9 buildingsTex = buildingsAtlas->GetTexture();
            spriteRenderer->SetTextureSlot(SLOT_BUILDINGS_HIGHLIGHT, buildingsTex);

            m_buildingRenderer.SetAtlases(
                buildingsAtlas.get(),
                SLOT_BUILDINGS_HIGHLIGHT
            );
        }

        reg.getTextureOrLoad("Units");
        std::tr1::shared_ptr<SpriteAtlas> unitsAtlas = reg.getAtlas("Units");
        if (unitsAtlas && unitsAtlas->GetTexture()) {
            LPDIRECT3DTEXTURE9 unitsTex = unitsAtlas->GetTexture();
            spriteRenderer->SetTextureSlot(SLOT_UNITS, unitsTex);

            std::tr1::shared_ptr<SpriteAtlas> iconAtlas = reg.getAtlas("Icon");
            m_settlerRenderer.SetAtlases(
                unitsAtlas.get(),
                iconAtlas.get(),
                SLOT_UNITS
            );
        }

        // Bind Icon atlas for flag resource pass
        {
            std::tr1::shared_ptr<SpriteAtlas> iconAtlas = reg.getAtlas("Icon");
            if (iconAtlas && iconAtlas->GetTexture()) {
                LPDIRECT3DTEXTURE9 iconTex = iconAtlas->GetTexture();
                spriteRenderer->SetTextureSlot(SLOT_FLAG_RESOURCES, iconTex);
            }
        }

        // Set pass texture slots (atlases already bound above)
        m_cursorPass.SetTextureSlot(SLOT_UI_CURSOR);
        m_flagResourcePass.SetTextureSlot(SLOT_FLAG_RESOURCES);
        m_groundResourcePass.SetTextureSlot(SLOT_FLAG_RESOURCES);
        m_workerPass.SetUnitSlot(SLOT_UNITS);
        m_workerPass.SetIconSlot(SLOT_UI_MENU_ICON);
        m_wildlifePass.SetTextureSlot(SLOT_UNITS);
        m_placementPreviewPass.SetTextureSlot(SLOT_BUILDINGS_HIGHLIGHT);
        m_roadPreviewPass.SetTextureSlot(SLOT_STREETS);
        m_geologistOverlayPass.SetTextureSlot(SLOT_UI_MENU_ICON);
        m_huntingSpotPass.SetTextureSlot(SLOT_UI_MENU_ICON);
        m_confirmationMenuPass.SetBgSlot(SLOT_UI_MENU_BG);
        m_confirmationMenuPass.SetIconSlot(SLOT_UI_MENU_ICON);
        m_notificationPass.SetTextureSlot(SLOT_UI_MENU_BG);
        m_buildingHighlightPass.SetTextureSlot(SLOT_BUILDINGS_HIGHLIGHT);
        m_townHallPanelPass.SetTextureSlot(SLOT_UI_TOWNHALL_PANEL);
        m_backgroundPass.SetTextureSlot(SLOT_BACKGROUND);
        m_roadConnectionPass.SetTextureSlot(SLOT_STREETS);
        m_workSitePass.SetTextureSlot(SLOT_BUILDINGS_HIGHLIGHT);
        m_resourceHudPass.SetTextureSlot(SLOT_UI_MENU_ICON);
        m_bannerPass.SetTextureSlot(SLOT_UI_MENU_BG);

        // Build per-frame render context
        RenderContext context;
        context.camera = m_camera;
        context.time = 0.0f;            // TODO: wire real time when needed
        context.debugOverlay = false;

        // Execute the render graph (all passes push to m_commandBuffer)
        m_renderGraph.Execute(renderFrame, context, m_commandBuffer);
    }

    // ─── Submit scene command buffer to graphics queue ────────────
    m_commandBuffer.SubmitToQueue(renderQueue);
}

} // namespace Scene
