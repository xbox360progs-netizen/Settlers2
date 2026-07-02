#include "stdafx.h"
#include "GameRenderer.h"
#include "FrameContext.h"
#include "../Graphics/RenderCommandBuilder.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/SpriteAtlas.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/TextureRegistry.h"
#include "../Logic/CoordinateSystem.h"
#include "../Logic/EconomyManager.h"
#include "../World/Map.h"
#include "../World/FlagManager.h"
#include "../World/CarrierManager.h"
#include "../World/ConstructionManager.h"
#include "../World/WorkerManager.h"
#include "../Graphics/TextManager.h"
#include "../Graphics/Camera.h"
#include "../Graphics/TileRenderer.h"
#include "../Graphics/ShaderManager.h"
#include "../UI/GridMenu.h"
#include "../UI/UIMenu.h"
#include "BuildingPlacement.h"
#include "PlacementController.h"
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
    Camera*           camera,
    World::Map*       map,
    World::FlagManager*         flagManager,
    World::CarrierManager*      carrierManager,
    World::ConstructionManager* constructionManager,
    World::WorkerManager*       workerManager,
    Logic::EconomyManager*      economyManager,
    PlacementController*        placement,
    GridMenu*                   buildMenu,
    UIMenu*                     flagMenu,
    TextManager*                textManager
)
    : m_tileRenderer(tileRenderer)
    , m_renderer(renderer)
    , m_camera(camera)
    , m_map(map)
    , m_flagManager(flagManager)
    , m_carrierManager(carrierManager)
    , m_constructionManager(constructionManager)
    , m_workerManager(workerManager)
    , m_economyManager(economyManager)
    , m_placement(placement)
    , m_buildMenu(buildMenu)
    , m_flagMenu(flagMenu)
    , m_textManager(textManager)
    , m_groundWoodIconIdx(-1)
    , m_groundWoodIconLoaded(false)
    , m_terrainPass(*tileRenderer)
    , m_buildingRenderPass(m_buildingRenderer)
    , m_settlerRenderPass(m_settlerRenderer)
{
    // Execute order: Terrain → Buildings → RoadPreview → PlacementPreview → Settlers → Wildlife → FlagResources → Overlays → UI → Cursor
    m_renderGraph.AddPass(&m_terrainPass);
    m_renderGraph.AddPass(&m_buildingRenderPass);
    m_renderGraph.AddPass(&m_roadPreviewPass);
    m_renderGraph.AddPass(&m_placementPreviewPass);
    m_renderGraph.AddPass(&m_settlerRenderPass);
    m_renderGraph.AddPass(&m_wildlifePass);
    m_renderGraph.AddPass(&m_flagResourcePass);
    m_renderGraph.AddPass(&m_geologistOverlayPass);
    m_renderGraph.AddPass(&m_confirmationMenuPass);
    m_renderGraph.AddPass(&m_notificationPass);
    m_renderGraph.AddPass(&m_cursorPass);
}

// ─── Render ────────────────────────────────────────────────────────────
void GameRenderer::Render(Graphics::RenderQueue* renderQueue, const FrameContext& frame, const RenderFrame& renderFrame)
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
    }

    // ─── Full-screen background ────────────────────────────────────────
    {
        LPDIRECT3DTEXTURE9 bgTex = reg.getTextureOrLoad("background_game");
        if (bgTex && spriteRenderer) {
            spriteRenderer->SetTextureSlot(SLOT_BACKGROUND, bgTex);
            Graphics::RenderCommandBuilder()
                .UIElement(0, 0, 1280, 720, 0, 0, 1, 1, SLOT_BACKGROUND, 0)
                .Layer(LAYER_EFFECTS)
                .Submit(renderQueue);
        }
    }

    // ─── E/W connection quads for committed road tiles ─────────────────
    {
        World::TileLayer* roadsLayer = m_map ? m_map->GetLayer(World::Roads) : NULL;
        if (roadsLayer) {
            TextureRegistry& regLoc = TextureRegistry::instance();
            std::tr1::shared_ptr<SpriteAtlas> streetsAtlas = regLoc.getAtlas("streets");
            if (streetsAtlas && streetsAtlas->GetTexture()) {
                spriteRenderer->SetTextureSlot(SLOT_STREETS, streetsAtlas->GetTexture());
                const std::vector<uint32_t>* group = streetsAtlas->GetGroup("street_1");
                if (group && !group->empty()) {
                    uint32_t ewIdx = (*group)[0];
                    const SpriteRegion* ewRegion = streetsAtlas->GetRegion(ewIdx);
                    if (ewRegion) {
                        CoordinateSystem& coords = CoordinateSystem::GetInstance();
                        int rw = roadsLayer->GetWidth();
                        int rh = roadsLayer->GetHeight();
                        for (int y = 0; y < rh; ++y) {
                            for (int x = 0; x < rw - 1; ++x) {
                                const World::Tile& t1 = roadsLayer->GetTile(x, y);
                                if (t1.regionIndex < 0 || t1.atlasName != "streets") continue;
                                const World::Tile& t2 = roadsLayer->GetTile(x + 1, y);
                                if (t2.regionIndex < 0 || t2.atlasName != "streets") continue;
                                float wx1, wy1, wx2, wy2;
                                coords.NodeTileToWorld(x, y, wx1, wy1);
                                coords.NodeTileToWorld(x + 1, y, wx2, wy2);
                                float cx = (wx1 + wx2) * 0.5f;
                                float cy = (wy1 + wy2) * 0.5f;
                                float dx = (float)fabs(wx2 - wx1);
                                Graphics::RenderCommandBuilder()
                                    .WorldSprite(cx - dx * 0.5f, cy - 3.0f,
                                        dx, 6.0f,
                                        ewRegion->u0, ewRegion->v0, ewRegion->u1, ewRegion->v1,
                                        SLOT_STREETS, static_cast<WORD>(30000 + y * 400))
                                    .Submit(renderQueue);
                            }
                        }
                    }
                }
            }
        }
    }

    // ─── Clear scene command buffer for this frame ─────────────────
    m_commandBuffer.Clear();

    // ─── Execute all registered render passes via RenderGraph ───────────
    // Each pass reads from RenderFrame + RenderContext and pushes to the buffer.
    // Order: TerrainPass → BuildingPass → RoadPreviewPass → PlacementPreviewPass → SettlerPass → WildlifePass → FlagResourcePass → GeologistOverlayPass → ConfirmationMenuPass → CursorPass
    // Future: NotificationPass, GamepadCursorPass, HudPass.
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
        m_wildlifePass.SetTextureSlot(SLOT_UNITS);
        m_placementPreviewPass.SetTextureSlot(SLOT_BUILDINGS_HIGHLIGHT);
        m_roadPreviewPass.SetTextureSlot(SLOT_STREETS);
        m_geologistOverlayPass.SetTextureSlot(SLOT_UI_MENU_ICON);
        m_confirmationMenuPass.SetBgSlot(SLOT_UI_MENU_BG);
        m_confirmationMenuPass.SetIconSlot(SLOT_UI_MENU_ICON);
        m_notificationPass.SetTextureSlot(SLOT_UI_MENU_BG);

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

    // ─── Render build menu ─────────────────────────────────────────────
    if (m_buildMenu && frame.input.menuActive) {
        m_buildMenu->Render();
    }

    // ─── Confirmation menu text (stage 8B1 bridge — removed with CommandBuffer text support) ───
    if (renderFrame.ui.confirmation.visible && m_textManager) {
        float cx = 640.0f;
        float yOff = 200.0f;
        m_textManager->DrawTextCenteredToScreen("Геолог", cx, yOff + 54.0f, D3DCOLOR_ARGB(255, 255, 255, 220), 0.095f, FONT_MENU, FONT_STYLE_NORMAL, LAYER_UI);
        m_textManager->DrawTextCenteredToScreen("Отправить геолога для поиска полезных ископаемых",
            cx, yOff + 162.0f, D3DCOLOR_ARGB(255, 200, 200, 200), 0.08f, FONT_MENU, FONT_STYLE_NORMAL, LAYER_UI);
    }

    // ─── Notification text (stage 8B2 bridge — removed with CommandBuffer text support) ───
    if (!renderFrame.ui.notifications.empty() && m_textManager) {
        float startX = 1280.0f - 280.0f + 10.0f;
        float startY = 20.0f;
        float boxH = 60.0f;
        for (size_t i = 0; i < renderFrame.ui.notifications.size(); ++i) {
            const RenderNotification& n = renderFrame.ui.notifications[i];
            if (!n.isActive) continue;
            float yPos = startY + n.offsetY;
            m_textManager->DrawString(n.title, startX, yPos + 4.0f, D3DCOLOR_ARGB(255, 255, 200, 80), 0.07f);
            m_textManager->DrawString(n.line1, startX, yPos + 22.0f, D3DCOLOR_ARGB(255, 220, 220, 220), 0.06f);
            if (n.line2[0] != '\0') {
                m_textManager->DrawString(n.line2, startX, yPos + 38.0f, D3DCOLOR_ARGB(255, 180, 180, 180), 0.055f);
            }
        }
    }

    // ─── Render flag menu ──────────────────────────────────────────────
    if (m_flagMenu && frame.input.flagMenuActive) {
        m_flagMenu->Render();
    }

    // ─── Hunting spots overlay ─────────────────────────────────────────
    if (frame.input.flagMenuActive && m_flagManager && m_map && m_textManager) {
        World::Flag* flag = m_flagManager->GetFlagAt(m_placement->GetConfirmTargetX(), m_placement->GetConfirmTargetY());
        if (flag && flag->building && flag->building->type == World::Hunter) {
            Logic::ResourceRegistry* registry = m_map->GetResourceRegistry();
            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            std::tr1::shared_ptr<SpriteAtlas> iconAtlas = TextureRegistry::instance().getAtlas("Icon");
            if (registry && iconAtlas) {
                uint32_t deerIcon = iconAtlas->GetIndex("r_deer");
                if (deerIcon != 0xFFFFFFFF) {
                    const SpriteRegion* deerR = iconAtlas->GetRegion(deerIcon);
                    const std::vector<Vector2i>& spawners = registry->GetWorldResources(World::ResourceType_WildlifeSpawner_Deer);
                    for (size_t si = 0; si < spawners.size(); ++si) {
                        const World::ResourceNode& node = m_map->GetResourceNode(spawners[si].x, spawners[si].y);
                        if (node.type != World::ResourceType_WildlifeSpawner_Deer) continue;
                        float wx, wy;
                        coords.NodeTileToWorld((float)spawners[si].x, (float)spawners[si].y, wx, wy);
                        if (deerR) {
                            float iconSize = 20.0f;
                            Graphics::RenderCommandBuilder()
                                .WorldSprite(wx - iconSize * 0.5f, wy - iconSize,
                                    iconSize, iconSize,
                                    deerR->u0, deerR->v0, deerR->u1, deerR->v1,
                                    SLOT_UI_MENU_ICON, static_cast<WORD>(0.99f * 65535.0f))
                                .Color(D3DCOLOR_ARGB(200, 255, 255, 255))
                                .Layer(LAYER_FOREGROUND)
                                .Submit(renderQueue);
                        }
                        char buf[8];
                        _snprintf(buf, sizeof(buf), "%d", node.amount);
                        m_textManager->DrawTextToWorld(buf, wx, wy - 28.0f, D3DCOLOR_ARGB(255, 255, 255, 0), 0.07f);
                    }
                }
            }
        }
    }

    // ─── Ground resource overlays ──────────────────────────────────────
    {
        TextureRegistry& reg2 = TextureRegistry::instance();
        std::tr1::shared_ptr<SpriteAtlas> iconAtlas = reg2.getAtlas("Icon");
        if (iconAtlas && m_map && m_textManager) {
            if (!m_groundWoodIconLoaded) {
                uint32_t idx = iconAtlas->GetIndex("r_exotic_wood");
                m_groundWoodIconIdx = (idx != 0xFFFFFFFF) ? (int)idx : -1;
                m_groundWoodIconLoaded = true;
            }
            int n = m_map->GetGroundResourceCount();
            if (n > 0) {
                CoordinateSystem& coords = CoordinateSystem::GetInstance();
                int iconIdx = m_groundWoodIconIdx;
                const SpriteRegion* iconR = (iconIdx >= 0) ? iconAtlas->GetRegion(iconIdx) : NULL;
                for (int gi = 0; gi < n; ++gi) {
                    World::GroundResource* gr = m_map->GetGroundResource(gi);
                    if (!gr) continue;
                    float wx, wy;
                    coords.NodeTileToWorld(gr->pos.x, gr->pos.y, wx, wy);
                    if (iconR) {
                        float iconSize = 24.0f;
                        Graphics::RenderCommandBuilder()
                            .WorldSprite(wx - iconSize * 0.5f, wy - iconSize - 8.0f,
                                iconSize, iconSize,
                                iconR->u0, iconR->v0, iconR->u1, iconR->v1,
                                SLOT_UI_MENU_ICON, static_cast<WORD>(0.99f * 65535.0f))
                            .Color(D3DCOLOR_ARGB(220, 255, 255, 255))
                            .Layer(LAYER_FOREGROUND)
                            .Submit(renderQueue);
                    }
                    char buf[16];
                    _snprintf(buf, sizeof(buf), "%d", gr->amount);
                    m_textManager->DrawTextToWorld(buf, wx, wy - 40.0f, D3DCOLOR_ARGB(255, 255, 255, 0), 0.08f);
                }
            }
        }
    }

    if (!frame.input.menuActive && !frame.input.roadMenuActive && !frame.input.flagMenuActive && !frame.input.geologistMenuActive && !frame.input.townHallPanelOpen) {
        // ─── Town hall highlight ───────────────────────────────────────
        if (frame.input.cursorOnTownHall) {
            std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
            if (buildingsAtlas) {
                uint32_t thIdx = buildingsAtlas->GetIndex("b_townhall");
                if (thIdx != 0xFFFFFFFF) {
                    const SpriteRegion* r = buildingsAtlas->GetRegion(thIdx);
                    if (r) {
                        float wx, wy;
                        CoordinateSystem::GetInstance().NodeTileToWorld(10, 8, wx, wy);
                        Graphics::RenderCommandBuilder()
                            .WorldSprite(wx - r->pivotX, wy - r->pivotY,
                                (float)r->width, (float)r->height,
                                r->u0, r->v0, r->u1, r->v1,
                                SLOT_BUILDINGS_HIGHLIGHT, static_cast<WORD>(0.99f * 65535.0f))
                            .Color(D3DCOLOR_ARGB(80, 255, 255, 255))
                            .Layer(LAYER_FOREGROUND)
                            .Submit(renderQueue);
                    }
                }
            }
        }
    }

    // ─── Town hall info panel ──────────────────────────────────────────
    if (frame.input.townHallPanelOpen && frame.overlay.townHallPanelBgIdx >= 0) {
        LPDIRECT3DTEXTURE9 uiTex = NULL;
        std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
        if (uiAtlas) uiTex = uiAtlas->GetTexture();
        if (spriteRenderer && uiTex) {
            float screenW = 1280.0f;
            float screenH = 720.0f;
            float panelLeft = (screenW - frame.overlay.townHallPanelW) * 0.5f;
            float panelTop = (screenH - frame.overlay.townHallPanelH) * 0.5f;

            Graphics::RenderCommandBuilder()
                .UIElement(panelLeft, panelTop,
                    frame.overlay.townHallPanelW, frame.overlay.townHallPanelH,
                    frame.overlay.townHallPanelU0, frame.overlay.townHallPanelV0,
                    frame.overlay.townHallPanelU1, frame.overlay.townHallPanelV1,
                    SLOT_UI_TOWNHALL_PANEL, 10)
                .Submit(renderQueue);

            if (m_textManager && m_economyManager) {
                float tx = panelLeft + 40.0f;
                float ty = panelTop + 30.0f;
                float lineH = 28.0f;
                char buf[64];

                _snprintf(buf, sizeof(buf), "Wood: %d", m_economyManager->GetTotalStock(World::ResourceType_Wood));
                m_textManager->DrawString(buf, tx, ty, 0xFFFFFFFF, 0.08f); ty += lineH;
                _snprintf(buf, sizeof(buf), "Planks: %d", m_economyManager->GetTotalStock(World::ResourceType_Planks));
                m_textManager->DrawString(buf, tx, ty, 0xFFFFFFFF, 0.08f); ty += lineH;
                _snprintf(buf, sizeof(buf), "Stone: %d", m_economyManager->GetTotalStock(World::ResourceType_Stone));
                m_textManager->DrawString(buf, tx, ty, 0xFFFFFFFF, 0.08f); ty += lineH;
                _snprintf(buf, sizeof(buf), "Fish: %d", m_economyManager->GetTotalStock(World::ResourceType_Fish));
                m_textManager->DrawString(buf, tx, ty, 0xFFFFFFFF, 0.08f); ty += lineH;
                _snprintf(buf, sizeof(buf), "Meat: %d", m_economyManager->GetTotalStock(World::ResourceType_Meat));
                m_textManager->DrawString(buf, tx, ty, 0xFFFFFFFF, 0.08f); ty += lineH;
                _snprintf(buf, sizeof(buf), "Coal: %d", m_economyManager->GetTotalStock(World::ResourceType_Coal));
                m_textManager->DrawString(buf, tx, ty, 0xFFFFFFFF, 0.08f); ty += lineH;
            }
        }
    }

    // ─── Highlight all buildings when town hall panel is open ──────────
    if (frame.input.townHallPanelOpen && m_flagManager) {
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
        if (buildingsAtlas) {
            for (size_t fi = 0; fi < m_flagManager->GetCount(); ++fi) {
                World::Flag* flag = m_flagManager->GetFlag(fi);
                if (!flag || !flag->building) continue;
                uint32_t sprIdx;
                if (flag->building->IsDepleted() && flag->building->m_depletedSpriteIdx >= 0) {
                    sprIdx = (uint32_t)flag->building->m_depletedSpriteIdx;
                } else {
                    const char* spriteName = BuildingPlacementManager::GetBuildingSpriteName(flag->building->type);
                    if (!spriteName || !*spriteName) continue;
                    sprIdx = buildingsAtlas->GetIndex(spriteName);
                }
                if (sprIdx == 0xFFFFFFFF) continue;
                const SpriteRegion* r = buildingsAtlas->GetRegion(sprIdx);
                if (!r) continue;
                int bldX = flag->building->pos.x;
                int bldY = flag->building->pos.y;
                float wx, wy;
                coords.NodeTileToWorld(bldX, bldY, wx, wy);
                Graphics::RenderCommandBuilder()
                    .WorldSprite(wx - r->pivotX, wy - r->pivotY,
                        (float)r->width, (float)r->height,
                        r->u0, r->v0, r->u1, r->v1,
                        SLOT_BUILDINGS_HIGHLIGHT, static_cast<WORD>(0.99f * 65535.0f))
                    .Color(D3DCOLOR_ARGB(80, 255, 255, 255))
                    .Layer(LAYER_FOREGROUND)
                    .Submit(renderQueue);
            }
        }
    }

    // ─── Work-site sprites ─────────────────────────────────────────────
    {
        std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
        if (buildingsAtlas && m_economyManager) {
            LPDIRECT3DTEXTURE9 buildingsTex = buildingsAtlas->GetTexture();
            if (spriteRenderer && buildingsTex)
                spriteRenderer->SetTextureSlot(SLOT_BUILDINGS_HIGHLIGHT, buildingsTex);
            for (int i = 0; i < m_economyManager->GetBuildingCount(); ++i) {
                World::Building* b = m_economyManager->GetBuilding(i);
                if (!b) continue;
                Vector2i wsPos;
                const char* wsSpriteName = NULL;
                if (!b->GetWorkSiteRenderInfo(wsPos, wsSpriteName)) continue;
                if (!wsSpriteName || !*wsSpriteName) continue;
                uint32_t sprIdx = buildingsAtlas->GetIndex(wsSpriteName);
                if (sprIdx == 0xFFFFFFFF) {
                    std::string lowerName = wsSpriteName;
                    for (size_t ci = 0; ci < lowerName.size(); ++ci)
                        if (lowerName[ci] >= 'A' && lowerName[ci] <= 'Z')
                            lowerName[ci] = lowerName[ci] - 'A' + 'a';
                    sprIdx = buildingsAtlas->GetIndex(lowerName.c_str());
                }
                if (sprIdx == 0xFFFFFFFF) continue;
                const SpriteRegion* r = buildingsAtlas->GetRegion(sprIdx);
                if (!r) continue;
                float wx, wy;
                CoordinateSystem::GetInstance().NodeTileToWorld(wsPos.x, wsPos.y, wx, wy);
                Graphics::RenderCommandBuilder()
                    .WorldSprite(wx - r->pivotX, wy - r->pivotY,
                        (float)r->width, (float)r->height,
                        r->u0, r->v0, r->u1, r->v1,
                        SLOT_BUILDINGS_HIGHLIGHT, static_cast<WORD>(0.97f * 65535.0f))
                    .Layer(LAYER_EFFECTS)
                    .Submit(renderQueue);
            }
        }
    }

    // ─── Resource HUD bar ──────────────────────────────────────────────
    if (frame.overlay.resourceHudLoaded) {
        TextureRegistry& reg2 = TextureRegistry::instance();
        std::tr1::shared_ptr<SpriteAtlas> resIconAtlas = reg2.getAtlas("Icon");
        if (resIconAtlas) {
            float barX = 100.0f;
            float barY = 6.0f;
            float iconSize = 28.0f;
            float spacing = 60.0f;

            for (int i = 0; i < OverlayFrameState::RESOURCE_HUD_COUNT; ++i) {
                if (frame.overlay.resourceHud[i].iconIdx < 0) continue;

                const SpriteRegion* r = resIconAtlas->GetRegion(frame.overlay.resourceHud[i].iconIdx);
                if (!r) continue;

                Graphics::RenderCommandBuilder()
                    .UIElement(barX, barY,
                        iconSize, iconSize,
                        r->u0, r->v0, r->u1, r->v1,
                        SLOT_UI_MENU_ICON, 200)
                    .Layer(LAYER_FOREGROUND)
                    .Submit(renderQueue);

                if (m_economyManager) {
                    char buf[32];
                    _snprintf(buf, sizeof(buf), "%d", m_economyManager->GetTotalStock(frame.overlay.resourceHud[i].type));
                    float textX = barX + iconSize + 4.0f;
                    float textY = barY + (iconSize - 14.0f) * 0.5f;
                    m_textManager->DrawString(buf, textX, textY, 0xFFFFFFFF, 0.07f);
                }

                barX += spacing;
            }
        }
    }


    // ─── Gamepad UI ────────────────────────────────────────────────────
    PushUiToQueue(renderQueue, frame);

    // ─── Notification banner + status text ─────────────────────────────
    if (m_textManager && frame.input.statusText[0] != '\0') {
        float screenW = 1280.0f;
        float screenH = 720.0f;
        float textY = screenH - 40.0f;
        if (frame.overlay.bannerLoaded && spriteRenderer) {
            TextureRegistry& regB = TextureRegistry::instance();
            std::tr1::shared_ptr<SpriteAtlas> uiAtlasB = regB.getAtlas("ui");
            if (uiAtlasB && uiAtlasB->GetTexture()) {
                spriteRenderer->SetTextureSlot(SLOT_UI_MENU_BG, uiAtlasB->GetTexture());
            }
        }
        if (frame.overlay.bannerLoaded && frame.overlay.bannerSlideX < 1280.0f) {
            Graphics::RenderCommandBuilder()
                .UIElement(frame.overlay.bannerSlideX, textY - frame.overlay.bannerH,
                    frame.overlay.bannerW, frame.overlay.bannerH,
                    frame.overlay.bannerU0, frame.overlay.bannerV0, frame.overlay.bannerU1, frame.overlay.bannerV1,
                    SLOT_UI_MENU_BG, 0)
                .Layer(LAYER_EFFECTS)
                .Submit(renderQueue);
        }
        float textX = frame.overlay.bannerSlideX + 40.0f;
        m_textManager->DrawString(frame.input.statusText, textX, textY - frame.overlay.bannerH + 4.0f + 25.0f, 0xFFFFFFFF, 0.096f);
    }

    // ─── Logistics debug overlay ───────────────────────────────────────
    if (frame.input.logisticsDebug && m_textManager) {
        CoordinateSystem& coords = CoordinateSystem::GetInstance();

        if (m_flagManager) {
            char buf[64];
            for (size_t fi = 0; fi < m_flagManager->GetCount(); ++fi) {
                World::Flag* flag = m_flagManager->GetFlag(fi);
                if (!flag) continue;
                float wx, wy;
                coords.NodeTileToWorld(flag->pos.x, flag->pos.y, wx, wy);

                buf[0] = '\0';
                int avail = 0;
                for (int si = 0; si < 8; ++si) {
                    if (flag->slots[si].type != World::ResourceType_None && flag->slots[si].amount > 0) {
                        char tag[8];
                        switch (flag->slots[si].type) {
                            case World::ResourceType_Wood:   _snprintf(tag, sizeof(tag), "W:%d ", flag->slots[si].amount); break;
                            case World::ResourceType_Planks: _snprintf(tag, sizeof(tag), "P:%d ", flag->slots[si].amount); break;
                            case World::ResourceType_Stone:  _snprintf(tag, sizeof(tag), "S:%d ", flag->slots[si].amount); break;
                            case World::ResourceType_Fish:   _snprintf(tag, sizeof(tag), "F:%d ", flag->slots[si].amount); break;
                            case World::ResourceType_Meat:   _snprintf(tag, sizeof(tag), "M:%d ", flag->slots[si].amount); break;
                            case World::ResourceType_Coal:   _snprintf(tag, sizeof(tag), "C:%d ", flag->slots[si].amount); break;
                            default:
                                _snprintf(tag, sizeof(tag), "%s:%d ",
                                    World::ResourceTypeToString(flag->slots[si].type), flag->slots[si].amount);
                                break;
                        }
                        avail += _snprintf(buf + avail, sizeof(buf) - avail, "%s", tag);
                        if (avail >= (int)sizeof(buf) - 2) break;
                    }
                }
                _snprintf(buf + avail, sizeof(buf) - avail, "~%d", flag->id);

                float ty = wy + 12.0f;
                if (flag->hasBuilding) ty += 20.0f;
                m_textManager->DrawString(buf, wx - 30.0f, ty, D3DCOLOR_ARGB(220, 255, 255, 200), 0.06f, FONT_DEBUG, FONT_STYLE_NORMAL, 0.05f, LAYER_EFFECTS);
            }
        }

        if (m_carrierManager) {
            char buf[64];
            for (int ci = 0; ci < m_carrierManager->GetCarrierCount(); ++ci) {
                World::Carrier* carrier = m_carrierManager->GetCarrier(ci);
                if (!carrier) continue;

                const Vector2i* pathTiles = NULL;
                int pathCount = 0;
                float ep = 0.0f;

                if (World::IsTransitState(carrier->state)) {
                    if (carrier->transitCount < 2) continue;
                    pathTiles = carrier->transitTiles;
                    pathCount = (int)carrier->transitCount;
                    ep = carrier->transitProgress;
                } else {
                    if (!carrier->road || carrier->road->tileCount < 2) continue;
                    pathTiles = carrier->road->tiles;
                    pathCount = (int)carrier->road->tileCount;
                    ep = carrier->ep;
                }

                int pathLen = pathCount - 1;
                if (ep < 0.0f) ep = 0.0f;
                if (ep > (float)pathLen) ep = (float)pathLen;
                int idx = (int)ep;
                float frac = ep - (float)idx;
                if (idx >= pathLen) { idx = pathLen - 1; frac = 1.0f; }
                if (idx < 0) { idx = 0; frac = 0.0f; }

                const Vector2i& tileA = pathTiles[idx];
                const Vector2i& tileB = pathTiles[idx + 1];

                float cx, cy, nx, ny;
                coords.NodeTileToWorld(tileA.x, tileA.y, cx, cy);
                coords.NodeTileToWorld(tileB.x, tileB.y, nx, ny);
                float wx = cx + (nx - cx) * frac;
                float wy = cy + (ny - cy) * frac;

                const char* cargoName = "Idle";
                if (carrier->m_cargo) {
                    cargoName = World::ResourceTypeToString(carrier->m_cargo->type);
                }

                if (carrier->road) {
                    _snprintf(buf, sizeof(buf), "%s %u<->%u", cargoName,
                        carrier->m_roadEndpointA ? carrier->m_roadEndpointA->id : 0,
                        carrier->m_roadEndpointB ? carrier->m_roadEndpointB->id : 0);
                } else {
                    _snprintf(buf, sizeof(buf), "%s (transit)", cargoName);
                }
                m_textManager->DrawString(buf, wx - 20.0f, wy - 20.0f, D3DCOLOR_ARGB(220, 200, 255, 200), 0.05f, FONT_DEBUG, FONT_STYLE_NORMAL, 0.05f, LAYER_EFFECTS);
            }
        }

        m_textManager->DrawTextToScreen("LOGISTICS DEBUG ON (Back=toggle)", 10.0f, 10.0f, D3DCOLOR_ARGB(180, 255, 255, 255), 0.08f);
    }
}

// ─── PushUiToQueue (gamepad cursor) ────────────────────────────────────
void GameRenderer::PushUiToQueue(Graphics::RenderQueue* renderQueue, const FrameContext& frame)
{
    if (!m_textManager) return;

    TextureRegistry& reg = TextureRegistry::instance();

    // ── Gamepad cursor ────────────────────────────────────────────────
    if (frame.input.gamepadActive) {
        std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
        if (uiAtlas) {
            uint32_t cursorIdx = uiAtlas->GetIndex("cursor");
            if (cursorIdx != 0xFFFFFFFF) {
                const SpriteRegion* cursorRegion = uiAtlas->GetRegion(cursorIdx);
                if (cursorRegion) {
                    RenderQueue* rq = m_renderer ? m_renderer->GetRenderQueue() : NULL;
                    if (rq) {
                        float wx, wy;
                        CoordinateSystem::GetInstance().NodeTileToWorld(
                            frame.input.gamepadCursorX, frame.input.gamepadCursorY, wx, wy);

                        SpriteRenderer* sr = m_renderer->GetSpriteRenderer();
                        if (sr) {
                            LPDIRECT3DTEXTURE9 uiTex = uiAtlas->GetTexture();
                            if (uiTex) sr->SetTextureSlot(SLOT_UI_CURSOR, uiTex);
                        }

                        Graphics::RenderCommandBuilder()
                            .WorldSprite(wx - cursorRegion->pivotX, wy - cursorRegion->pivotY,
                                (float)cursorRegion->width, (float)cursorRegion->height,
                                cursorRegion->u0, cursorRegion->v0, cursorRegion->u1, cursorRegion->v1,
                                SLOT_UI_CURSOR, static_cast<WORD>(0.99f * 65535.0f))
                            .Color(D3DCOLOR_ARGB(255, 0, 255, 0))
                            .Layer(LAYER_FOREGROUND)
                            .Submit(rq);
                    }
                }
            }
        }
    }

}



} // namespace Scene
