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
#include "../World/RoadManager.h"
#include "../World/ConstructionManager.h"
#include "../World/WorkerManager.h"
#include "../World/WildlifeSystem.h"
#include "../Graphics/TextManager.h"
#include "../Graphics/Camera.h"
#include "../Graphics/TileRenderer.h"
#include "../Graphics/ShaderManager.h"
#include "../UI/GridMenu.h"
#include "../UI/UIMenu.h"
#include "BuildingPlacement.h"
#include "PlacementController.h"
#include "RoadController.h"
#include "TextureSlots.h"

namespace Scene {



// ─── GameRenderer constructor ──────────────────────────────────────────
GameRenderer::GameRenderer(
    TileRenderer*     tileRenderer,
    Renderer*         renderer,
    Camera*           camera,
    World::Map*       map,
    World::FlagManager*         flagManager,
    World::CarrierManager*      carrierManager,
    World::RoadManager*         roadManager,
    World::ConstructionManager* constructionManager,
    World::WorkerManager*       workerManager,
    World::WildlifeSystem*      wildlife,
    Logic::EconomyManager*      economyManager,
    PlacementController*        placement,
    RoadController*             roadController,
    GridMenu*                   buildMenu,
    UIMenu*                     flagMenu,
    UIMenu*                     geologistMenu,
    TextManager*                textManager
)
    : m_tileRenderer(tileRenderer)
    , m_renderer(renderer)
    , m_camera(camera)
    , m_map(map)
    , m_flagManager(flagManager)
    , m_carrierManager(carrierManager)
    , m_roadManager(roadManager)
    , m_constructionManager(constructionManager)
    , m_workerManager(workerManager)
    , m_wildlife(wildlife)
    , m_economyManager(economyManager)
    , m_placement(placement)
    , m_roadController(roadController)
    , m_buildMenu(buildMenu)
    , m_flagMenu(flagMenu)
    , m_geologistMenu(geologistMenu)
    , m_textManager(textManager)
    , m_groundWoodIconIdx(-1)
    , m_groundWoodIconLoaded(false)
{
}

// ─── Render ────────────────────────────────────────────────────────────
void GameRenderer::Render(Graphics::RenderQueue* renderQueue, const FrameContext& frame)
{
    if (!m_tileRenderer || !m_map) {
        OutputDebugStringA("[GameRenderer::Render] Not ready, returning\n");
        return;
    }

    if (m_renderer) {
        m_renderer->Clear(0xFF000000); // Black
    }

    m_tileRenderer->SetRenderQueue(renderQueue);

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

    m_tileRenderer->RenderMap();

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

    // ─── Render cursor or placement preview ────────────────────────────
    if (m_placement->GetState() == PLACESTATE_PLACE_FLAG && !m_placement->IsIdle()) {
        PlacementData pd = m_placement->GetPlacementData(frame.input.cursorTileX, frame.input.cursorTileY);

        if (pd.spriteRegion) {
            float wx, wy;
            CoordinateSystem::GetInstance().NodeTileToWorld(pd.buildX, pd.buildY, wx, wy);

            if (spriteRenderer) {
                std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
                if (buildingsAtlas && buildingsAtlas->GetTexture())
                    spriteRenderer->SetTextureSlot(SLOT_BUILDINGS_HIGHLIGHT, buildingsAtlas->GetTexture());
            }

            Graphics::RenderCommandBuilder()
                .WorldSprite(wx - pd.spriteRegion->pivotX, wy - pd.spriteRegion->pivotY,
                    (float)pd.spriteRegion->width, (float)pd.spriteRegion->height,
                    pd.spriteRegion->u0, pd.spriteRegion->v0,
                    pd.spriteRegion->u1, pd.spriteRegion->v1,
                    SLOT_BUILDINGS_HIGHLIGHT, static_cast<WORD>(0.98f * 65535.0f))
                .Color(pd.valid ? 0xAAFFFFFF : 0x44FF4444)
                .Layer(LAYER_FOREGROUND)
                .Submit(renderQueue);
        }
    } else if (!frame.input.menuActive && !frame.input.roadMenuActive && !frame.input.flagMenuActive && !frame.input.geologistMenuActive) {
        RenderCursor(renderQueue, frame);
    }

    // ─── Render flags ──────────────────────────────────────────────────
    if (m_flagManager && !m_flagManager->GetFlagPairs().empty() && spriteRenderer) {
        std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
        if (buildingsAtlas && buildingsAtlas->GetTexture()) {
            LPDIRECT3DTEXTURE9 buildingsTex = buildingsAtlas->GetTexture();
            spriteRenderer->SetTextureSlot(SLOT_BUILDINGS_HIGHLIGHT, buildingsTex);

            uint32_t flagIdx = buildingsAtlas->GetIndex("flag");
            const SpriteRegion* flagRegion = buildingsAtlas->GetRegion(flagIdx);
            if (flagRegion) {
                CoordinateSystem& coords = CoordinateSystem::GetInstance();
                const std::vector<std::pair<int,int> >& pairs = m_flagManager->GetFlagPairs();
                for (size_t fi = 0; fi < pairs.size(); ++fi) {
                    int fx = pairs[fi].first;
                    int fy = pairs[fi].second;
                    float wx, wy;
                    coords.NodeTileToWorld(fx, fy, wx, wy);
                    Graphics::RenderCommandBuilder()
                        .WorldSprite(wx - flagRegion->pivotX, wy - flagRegion->pivotY,
                            (float)flagRegion->width, (float)flagRegion->height,
                            flagRegion->u0, flagRegion->v0, flagRegion->u1, flagRegion->v1,
                            SLOT_BUILDINGS_HIGHLIGHT, static_cast<WORD>(30010 + fy * 400))
                        .Submit(renderQueue);
                }
            }
        }
    }

    // ─── Render resource icons on flags ────────────────────────────────
    if (m_flagManager && spriteRenderer) {
        std::tr1::shared_ptr<SpriteAtlas> iconAtlas = reg.getAtlas("Icon");
        if (iconAtlas && iconAtlas->GetTexture()) {
            LPDIRECT3DTEXTURE9 iconTex = iconAtlas->GetTexture();
            spriteRenderer->SetTextureSlot(SLOT_FLAG_RESOURCES, iconTex);

            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            for (size_t fi = 0; fi < m_flagManager->GetCount(); ++fi) {
                World::Flag* flag = m_flagManager->GetFlag(fi);
                if (!flag) continue;
                float fx, fy;
                coords.NodeTileToWorld(flag->pos.x, flag->pos.y, fx, fy);
                int iconY = 0;
                for (int si = 0; si < 8; ++si) {
                    if (flag->slots[si].type == World::ResourceType_None || flag->slots[si].amount <= 0) continue;
                    const char* iconName = NULL;
                    switch (flag->slots[si].type) {
                        case World::ResourceType_Wood:   iconName = "r_wood"; break;
                        case World::ResourceType_Planks: iconName = "r_planks"; break;
                        case World::ResourceType_Stone:  iconName = "r_stone"; break;
                        case World::ResourceType_Fish:   iconName = "r_fish"; break;
                        case World::ResourceType_Meat:   iconName = "r_meat"; break;
                        case World::ResourceType_Bread:  iconName = "r_bread"; break;
                        case World::ResourceType_Coal:   iconName = "r_coal"; break;
                        case World::ResourceType_IronOre: iconName = "r_ironore"; break;
                        case World::ResourceType_GoldOre: iconName = "r_goldore"; break;
                        case World::ResourceType_IronBar: iconName = "r_ironbar"; break;
                        case World::ResourceType_GoldBar: iconName = "r_goldbar"; break;
                        default: break;
                    }
                    if (!iconName) continue;
                    uint32_t idx = iconAtlas->GetIndex(iconName);
                    if (idx == 0xFFFFFFFF) continue;
                    const SpriteRegion* r = iconAtlas->GetRegion(idx);
                    if (!r) continue;

                    Graphics::RenderCommandBuilder()
                        .WorldSprite(fx - r->pivotX * 0.5f, fy - r->pivotY * 0.5f - 30.0f + iconY * -16.0f,
                            r->width * 0.5f, r->height * 0.5f,
                            r->u0, r->v0, r->u1, r->v1,
                            SLOT_FLAG_RESOURCES, static_cast<WORD>(30011 + flag->pos.y * 400 + iconY))
                        .Submit(renderQueue);
                    iconY--;
                }
            }
        }
    }

    // ─── Render carriers and builders ──────────────────────────────────
    if (spriteRenderer) {
        reg.getTextureOrLoad("Units");
        std::tr1::shared_ptr<SpriteAtlas> unitsAtlas = reg.getAtlas("Units");
        if (unitsAtlas && unitsAtlas->GetTexture()) {
            LPDIRECT3DTEXTURE9 unitsTex = unitsAtlas->GetTexture();
            spriteRenderer->SetTextureSlot(SLOT_UNITS, unitsTex);
            CoordinateSystem& coords = CoordinateSystem::GetInstance();

            auto unitsSpriteIndex = [](bool isCarrier, bool hasCargo, int dx, int dy) -> int {
                if (isCarrier) {
                    if (hasCargo) {
                        if (dy < 0) return (dx >= 0) ? 9 : 11;
                        if (dy > 0) return (dx >= 0) ? 8 : 10;
                        return (dx >= 0) ? 8 : 11;
                    } else {
                        if (dy < 0) return (dx < 0) ? 2 : 0;
                        if (dy > 0) return (dx < 0) ? 3 : 1;
                        return (dx >= 0) ? 1 : 3;
                    }
                } else {
                    return (dx >= 0) ? 4 : 5;
                }
            };

            static int carrierLogFrame = 0;
            carrierLogFrame++;
            bool logCarriers = (carrierLogFrame % 60 == 0);
            if (m_carrierManager) {
                for (int ci = 0; ci < m_carrierManager->GetCarrierCount(); ++ci) {
                    World::Carrier* carrier = m_carrierManager->GetCarrier(ci);
                    if (!carrier) continue;

                    const Vector2i* pathTiles = NULL;
                    int pathCount = 0;
                    float ep = 0.0f;
                    float walkDir = carrier->walkDir;

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

                    int dx = (walkDir > 0.0f) ? (tileB.x - tileA.x) : (tileA.x - tileB.x);
                    int dy = (walkDir > 0.0f) ? (tileB.y - tileA.y) : (tileA.y - tileB.y);
                    bool hasCargo = (carrier->m_cargo != NULL);
                    int spriteIdx = unitsSpriteIndex(true, hasCargo, dx, dy);

                    if (logCarriers) {
                        const char* stateNames[] = { "WalkingToPost", "Working", "ReturningHome" };
                        const char* sn = (carrier->state >= 0 && carrier->state < 3) ? stateNames[carrier->state] : "?";
                        const char* cn = carrier->m_cargo ? World::ResourceTypeToString(carrier->m_cargo->type) : "empty";
                        char dbg[256];
                        _snprintf(dbg, sizeof(dbg),
                            "[CARRIER] %d: state=%s ep=%.1f dir=%.1f sprite=%d cargo=%s path=%d tiles=(%d,%d)-(%d,%d)\n",
                            ci, sn, ep, walkDir, spriteIdx, cn, pathLen,
                            tileA.x, tileA.y, tileB.x, tileB.y);
                        OutputDebugStringA(dbg);
                    }

                    float wx0, wy0, wx1, wy1;
                    coords.NodeTileToWorld(tileA.x, tileA.y, wx0, wy0);
                    coords.NodeTileToWorld(tileB.x, tileB.y, wx1, wy1);
                    float wx = wx0 + (wx1 - wx0) * frac;
                    float wy = wy0 + (wy1 - wy0) * frac;

                    const SpriteRegion* r = unitsAtlas->GetRegion(spriteIdx);
                    if (!r) continue;

                    Graphics::RenderCommandBuilder()
                        .WorldSprite(wx - r->pivotX, wy - r->pivotY,
                            (float)r->width, (float)r->height,
                            r->u0, r->v0, r->u1, r->v1,
                            SLOT_UNITS, static_cast<WORD>(30020 + tileA.y * 400))
                        .Submit(renderQueue);

                    if (carrier->m_cargo) {
                        const char* cargoIconName = World::ResourceTypeToIconName(carrier->m_cargo->type);
                        if (cargoIconName && cargoIconName[0]) {
                            std::tr1::shared_ptr<SpriteAtlas> cargoAtlas = reg.getAtlas("Icon");
                            if (cargoAtlas) {
                                uint32_t cargoIdx = cargoAtlas->GetIndex(cargoIconName);
                                if (cargoIdx != 0xFFFFFFFF) {
                                    const SpriteRegion* cargoR = cargoAtlas->GetRegion(cargoIdx);
                                    if (cargoR) {
                                        float cargoSize = 16.0f;
                                        Graphics::RenderCommandBuilder()
                                            .WorldSprite(wx - cargoSize * 0.5f, wy - r->pivotY - cargoSize,
                                                cargoSize, cargoSize,
                                                cargoR->u0, cargoR->v0, cargoR->u1, cargoR->v1,
                                                SLOT_UI_MENU_ICON, static_cast<WORD>(30030 + tileA.y * 400))
                                            .Submit(renderQueue);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Building workers
            if (m_flagManager) {
                for (size_t fi = 0; fi < m_flagManager->GetCount(); ++fi) {
                    World::Flag* flag = m_flagManager->GetFlag((int)fi);
                    if (!flag || !flag->building) continue;

                    float wx, wy;
                    int wSpriteIdx = -1;
                    bool moving = false;

                    if (flag->building->GetWorkerRenderInfo(wx, wy, wSpriteIdx)) {
                        moving = true;
                        coords.NodeTileToWorld(wx, wy, wx, wy);
                    }
                    if (wSpriteIdx < 0) continue;
                    const SpriteRegion* wr = unitsAtlas->GetRegion(wSpriteIdx);
                    if (!wr) continue;

                    Graphics::RenderCommandBuilder()
                        .WorldSprite(wx - wr->pivotX, wy - wr->pivotY,
                            (float)wr->width, (float)wr->height,
                            wr->u0, wr->v0, wr->u1, wr->v1,
                            SLOT_UNITS, static_cast<WORD>(30020 + (moving ? (int)(wy + 0.5f) : flag->building->pos.y) * 400))
                        .Submit(renderQueue);
                }
            }

            // Arriving workers
            if (m_workerManager && unitsAtlas) {
                for (int wi = 0; wi < m_workerManager->GetActiveCount(); ++wi) {
                    const World::Worker* w = m_workerManager->GetWorkerByActiveIdx(wi);
                    if (w->state != World::WorkerState_MovingToJob) continue;
                    float wx = w->posX;
                    float wy = w->posY;
                    int spriteIdx = 4;
                    coords.NodeTileToWorld(wx, wy, wx, wy);
                    const SpriteRegion* wr = unitsAtlas->GetRegion(spriteIdx);
                    if (!wr) continue;
                    Graphics::RenderCommandBuilder()
                        .WorldSprite(wx - wr->pivotX, wy - wr->pivotY,
                            (float)wr->width, (float)wr->height,
                            wr->u0, wr->v0, wr->u1, wr->v1,
                            SLOT_UNITS, static_cast<WORD>(30020 + (int)(wy + 0.5f) * 400))
                        .Submit(renderQueue);
                }
            }

            // Builders
            if (m_constructionManager) {
                const std::vector<World::ConstructionSite*>& sites = m_constructionManager->GetAllSites();
                for (size_t si = 0; si < sites.size(); ++si) {
                    World::ConstructionSite* site = sites[si];
                    if (!site || !site->flag) continue;
                    if (site->builderState == World::Builder_None) continue;

                    float wx, wy;
                    int spriteIdx = 4;

                    if (site->builderState == World::Builder_Walking || site->builderState == World::Builder_Returning) {
                        if (site->builderRouteCount < 2) continue;
                        uint32_t fromIdx = site->builderRouteIndex;
                        uint32_t toIdx = fromIdx + 1;
                        if (fromIdx >= site->builderRouteCount - 1) {
                            size_t lastIdx = site->builderRouteCount - 1;
                            World::Flag* f = site->builderRoute[lastIdx];
                            coords.NodeTileToWorld(f->pos.x, f->pos.y, wx, wy);
                        } else {
                            World::Flag* fromFlag = site->builderRoute[fromIdx];
                            World::Flag* toFlag = site->builderRoute[toIdx];
                            World::Road* road = m_roadManager ? m_roadManager->GetRoadBetween(fromFlag, toFlag) : NULL;
                            if (road && road->tileCount >= 2) {
                                int tc = (int)road->tileCount;
                                float pathLen = (float)(tc - 1);
                                float pos = site->builderEp;
                                if (pos < 0.0f) pos = 0.0f;
                                if (pos > pathLen) pos = pathLen;
                                int tileIdx = (int)pos;
                                float frac = pos - (float)tileIdx;
                                if (tileIdx >= tc - 1) { tileIdx = tc - 2; frac = 1.0f; }
                                if (tileIdx < 0) { tileIdx = 0; frac = 0.0f; }
                                const Vector2i& tileA = road->tiles[tileIdx];
                                const Vector2i& tileB = road->tiles[tileIdx + 1];
                                float wx0, wy0, wx1, wy1;
                                coords.NodeTileToWorld(tileA.x, tileA.y, wx0, wy0);
                                coords.NodeTileToWorld(tileB.x, tileB.y, wx1, wy1);
                                wx = wx0 + (wx1 - wx0) * frac;
                                wy = wy0 + (wy1 - wy0) * frac;
                                int bdx = tileB.x - tileA.x;
                                int bdy = tileB.y - tileA.y;
                                spriteIdx = unitsSpriteIndex(false, false, bdx, bdy);
                            } else {
                                float wx0, wy0, wx1, wy1;
                                coords.NodeTileToWorld(fromFlag->pos.x, fromFlag->pos.y, wx0, wy0);
                                coords.NodeTileToWorld(toFlag->pos.x, toFlag->pos.y, wx1, wy1);
                                float t = (1.0f > 0.0f) ? site->builderEp / 1.0f : 0.0f;
                                if (t < 0.0f) t = 0.0f;
                                if (t > 1.0f) t = 1.0f;
                                wx = wx0 + (wx1 - wx0) * t;
                                wy = wy0 + (wy1 - wy0) * t;
                                int bdx = toFlag->pos.x - fromFlag->pos.x;
                                int bdy = toFlag->pos.y - fromFlag->pos.y;
                                spriteIdx = unitsSpriteIndex(false, false, bdx, bdy);
                            }
                        }
                    } else if (site->builderState == World::Builder_Building) {
                        coords.NodeTileToWorld(site->flag->pos.x, site->flag->pos.y, wx, wy);
                    } else {
                        coords.NodeTileToWorld(site->flag->pos.x, site->flag->pos.y, wx, wy);
                    }

                    const SpriteRegion* r = unitsAtlas->GetRegion(spriteIdx);
                    if (!r) continue;

                    Graphics::RenderCommandBuilder()
                        .WorldSprite(wx - r->pivotX, wy - r->pivotY,
                            (float)r->width, (float)r->height,
                            r->u0, r->v0, r->u1, r->v1,
                            SLOT_UNITS, static_cast<WORD>(30020 + site->flag->pos.y * 400))
                        .Submit(renderQueue);
                }
            }
        }
    }

    // ─── Render wildlife ───────────────────────────────────────────────
    if (m_wildlife) {
        const std::vector<World::Animal>& animals = m_wildlife->GetAllAnimals();
        if (!animals.empty()) {
            reg.getTextureOrLoad("Units");
            std::tr1::shared_ptr<SpriteAtlas> unitsAtlas = reg.getAtlas("Units");
            if (unitsAtlas && unitsAtlas->GetTexture()) {
                LPDIRECT3DTEXTURE9 unitsTex = unitsAtlas->GetTexture();
                spriteRenderer->SetTextureSlot(SLOT_UNITS, unitsTex);
                const std::vector<uint32_t>* animalGroup = unitsAtlas->GetGroup("Animals");
                if (animalGroup && !animalGroup->empty()) {
                    CoordinateSystem& coords = CoordinateSystem::GetInstance();
                    for (size_t i = 0; i < animals.size(); ++i) {
                        const World::Animal& a = animals[i];
                        if (a.state != World::AnimalState_Alive) continue;
                        if (a.type < 0 || a.type >= World::AnimalType_Count) continue;

                        int rawIdx = (int)a.type;
                        int dirIdx = World::VelocityToDirIndex(a.vx, a.vy);
                        int dirSpriteIdx = rawIdx * World::AnimalDirSpriteCount() + dirIdx;
                        int spriteIdx;
                        if (dirSpriteIdx < (int)animalGroup->size()) {
                            spriteIdx = dirSpriteIdx;
                        } else if (rawIdx < (int)animalGroup->size()) {
                            spriteIdx = rawIdx;
                        } else {
                            continue;
                        }
                        uint32_t regionIdx = (*animalGroup)[spriteIdx];
                        const SpriteRegion* r = unitsAtlas->GetRegion(regionIdx);
                        if (!r) continue;
                        float wx, wy;
                        coords.NodeTileToWorld(a.x, a.y, wx, wy);
                        Graphics::RenderCommandBuilder()
                            .WorldSprite(wx - r->pivotX, wy - r->pivotY,
                                (float)r->width, (float)r->height,
                                r->u0, r->v0, r->u1, r->v1,
                                SLOT_UNITS, static_cast<WORD>(30005 + (int)(a.y + 0.5f) * 400))
                            .Submit(renderQueue);
                    }
                }
            }
        }
    }

    // ─── Render road preview ───────────────────────────────────────────
    const std::vector<std::pair<int,int> >& roadPath = m_roadController->GetPreviewPath();
    if (m_placement->GetState() == PLACESTATE_PLACE_ROAD && !roadPath.empty()) {
        std::tr1::shared_ptr<SpriteAtlas> streetsAtlas = reg.getAtlas("streets");
        if (!streetsAtlas) return;
        CoordinateSystem& coords = CoordinateSystem::GetInstance();

        float flagAlignOffsetX = 0.0f;
        {   std::tr1::shared_ptr<SpriteAtlas> ba = reg.getAtlas("Buildings");
            uint32_t fi = ba->GetIndex("flag"); const SpriteRegion* fr = ba->GetRegion(fi);
            if (fr) { const std::vector<uint32_t>* rg = streetsAtlas->GetGroup("street_1");
            if (rg && !rg->empty()) { const SpriteRegion* rr = streetsAtlas->GetRegion((*rg)[0]);
            if (rr) { flagAlignOffsetX = (fr->width * 0.5f - fr->pivotX) - (rr->width * 0.5f - rr->pivotX); }}}}

        for (size_t i = 0; i < m_roadController->GetPreviewPath().size(); ++i) {
            int px = m_roadController->GetPreviewPath()[i].first;
            int py = m_roadController->GetPreviewPath()[i].second;
            World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
            int pattern = RoadController::CalcPatternAt(px, py, roadsLayer, m_roadController->GetPreviewPath());

            char groupBuf[16];
            const char* groupName = groupBuf;
            switch (pattern) {
                case 0:  groupName = "street_1"; break;
                case 1:  groupName = "street_1"; break;
                case 2:  groupName = "street_2"; break;
                case 3:  _snprintf(groupBuf, sizeof(groupBuf), "street_%d", 3); break;
                case 4:  groupName = "street_1"; break;
                case 5:  groupName = "street_5"; break;
                case 6:  _snprintf(groupBuf, sizeof(groupBuf), "street_%d", 6); break;
                case 7:  _snprintf(groupBuf, sizeof(groupBuf), "street_%d", 7); break;
                case 8:  groupName = "street_2"; break;
                case 9:  _snprintf(groupBuf, sizeof(groupBuf), "street_%d", 9); break;
                case 10: groupName = "street_2"; break;
                case 11: _snprintf(groupBuf, sizeof(groupBuf), "street_%d", 11); break;
                case 12: _snprintf(groupBuf, sizeof(groupBuf), "street_%d", 12); break;
                case 13: _snprintf(groupBuf, sizeof(groupBuf), "street_%d", 13); break;
                case 14: _snprintf(groupBuf, sizeof(groupBuf), "street_%d", 14); break;
                case 15: _snprintf(groupBuf, sizeof(groupBuf), "street_%d", 15); break;
            }

            const std::vector<uint32_t>* group = streetsAtlas->GetGroup(groupName);
            if (!group || group->empty()) {
                group = streetsAtlas->GetGroup("street_1");
                if (!group || group->empty()) continue;
            }

            uint32_t regionIdx = (*group)[0];
            const SpriteRegion* region = streetsAtlas->GetRegion(regionIdx);
            if (!region) continue;

            float wx, wy;
            coords.NodeTileToWorld(px, py, wx, wy);

            Graphics::RenderCommandBuilder()
                .WorldSprite(wx - region->pivotX + flagAlignOffsetX, wy - region->pivotY,
                    (float)region->width, (float)region->height,
                    region->u0, region->v0, region->u1, region->v1,
                    SLOT_STREETS, static_cast<WORD>(0.98f * 65535.0f))
                .Color(D3DCOLOR_ARGB(160, 255, 255, 255))
                .Layer(LAYER_FOREGROUND)
                .Submit(renderQueue);
        }

        const std::vector<uint32_t>* ewGroup = streetsAtlas->GetGroup("street_1");
        if (ewGroup && !ewGroup->empty()) {
            uint32_t ewIdx = (*ewGroup)[0];
            const SpriteRegion* ewRegion = streetsAtlas->GetRegion(ewIdx);
            if (ewRegion) {
                for (size_t i = 0; i + 1 < m_roadController->GetPreviewPath().size(); ++i) {
                    int x1 = m_roadController->GetPreviewPath()[i].first;
                    int y1 = m_roadController->GetPreviewPath()[i].second;
                    int x2 = m_roadController->GetPreviewPath()[i + 1].first;
                    int y2 = m_roadController->GetPreviewPath()[i + 1].second;
                    if (abs(x1 - x2) == 1 && y1 == y2) {
                        float wx1, wy1, wx2, wy2;
                        coords.NodeTileToWorld(x1, y1, wx1, wy1);
                        coords.NodeTileToWorld(x2, y2, wx2, wy2);
                        float cx = (wx1 + wx2) * 0.5f;
                        float cy = (wy1 + wy2) * 0.5f;
                        float dx = (float)fabs(wx2 - wx1);
                        Graphics::RenderCommandBuilder()
                            .WorldSprite(cx - dx * 0.5f + flagAlignOffsetX, cy - 3.0f,
                                dx, 6.0f,
                                ewRegion->u0, ewRegion->v0, ewRegion->u1, ewRegion->v1,
                                SLOT_STREETS, static_cast<WORD>(0.98f * 65535.0f))
                            .Color(D3DCOLOR_ARGB(160, 255, 255, 255))
                            .Layer(LAYER_FOREGROUND)
                            .Submit(renderQueue);
                    }
                }
            }
        }
    }

    // ─── Render auto-path preview ──────────────────────────────────────
    if (m_placement->GetState() == PLACESTATE_PLACE_ROAD && !m_roadController->GetAutoPath().empty()) {
        std::tr1::shared_ptr<SpriteAtlas> streetsAtlas = reg.getAtlas("streets");
        if (streetsAtlas) {
            CoordinateSystem& coords = CoordinateSystem::GetInstance();

            float flagAlignOffsetX = 0.0f;
            {   std::tr1::shared_ptr<SpriteAtlas> ba = reg.getAtlas("Buildings");
                uint32_t fi = ba->GetIndex("flag"); const SpriteRegion* fr = ba->GetRegion(fi);
                if (fr) { const std::vector<uint32_t>* rg = streetsAtlas->GetGroup("street_1");
                if (rg && !rg->empty()) { const SpriteRegion* rr = streetsAtlas->GetRegion((*rg)[0]);
                if (rr) { flagAlignOffsetX = (fr->width * 0.5f - fr->pivotX) - (rr->width * 0.5f - rr->pivotX); }}}}

            const std::vector<uint32_t>* group = streetsAtlas->GetGroup("street_1");
            if (group && !group->empty()) {
                uint32_t regionIdx = (*group)[0];
                const SpriteRegion* region = streetsAtlas->GetRegion(regionIdx);
                if (region) {
                    for (size_t i = 0; i < m_roadController->GetAutoPath().size(); ++i) {
                        int ax = m_roadController->GetAutoPath()[i].first;
                        int ay = m_roadController->GetAutoPath()[i].second;
                        float wx, wy;
                        coords.NodeTileToWorld(ax, ay, wx, wy);
                        Graphics::RenderCommandBuilder()
                            .WorldSprite(wx - region->pivotX + flagAlignOffsetX, wy - region->pivotY,
                                (float)region->width, (float)region->height,
                                region->u0, region->v0, region->u1, region->v1,
                                SLOT_STREETS, static_cast<WORD>(0.98f * 65535.0f))
                            .Color(D3DCOLOR_ARGB(160, 100, 200, 255))
                            .Layer(LAYER_FOREGROUND)
                            .Submit(renderQueue);
                    }
                }
            }

            const std::vector<uint32_t>* ewGroup = streetsAtlas->GetGroup("street_1");
            if (ewGroup && !ewGroup->empty()) {
                uint32_t ewIdx = (*ewGroup)[0];
                const SpriteRegion* ewRegion = streetsAtlas->GetRegion(ewIdx);
                if (ewRegion) {
                    for (size_t i = 0; i + 1 < m_roadController->GetAutoPath().size(); ++i) {
                        int x1 = m_roadController->GetAutoPath()[i].first;
                        int y1 = m_roadController->GetAutoPath()[i].second;
                        int x2 = m_roadController->GetAutoPath()[i + 1].first;
                        int y2 = m_roadController->GetAutoPath()[i + 1].second;
                        if (abs(x1 - x2) == 1 && y1 == y2) {
                            float wx1, wy1, wx2, wy2;
                            coords.NodeTileToWorld(x1, y1, wx1, wy1);
                            coords.NodeTileToWorld(x2, y2, wx2, wy2);
                            float cx = (wx1 + wx2) * 0.5f;
                            float cy = (wy1 + wy2) * 0.5f;
                            float dx = (float)fabs(wx2 - wx1);
                            Graphics::RenderCommandBuilder()
                                .WorldSprite(cx - dx * 0.5f + flagAlignOffsetX, cy - 3.0f,
                                    dx, 6.0f,
                                    ewRegion->u0, ewRegion->v0, ewRegion->u1, ewRegion->v1,
                                    SLOT_STREETS, static_cast<WORD>(0.98f * 65535.0f))
                                .Color(D3DCOLOR_ARGB(160, 100, 200, 255))
                                .Layer(LAYER_FOREGROUND)
                                .Submit(renderQueue);
                        }
                    }
                }
            }
        }
    }

    // ─── Render valid neighbor tiles ───────────────────────────────────
    if (m_placement->GetState() == PLACESTATE_PLACE_ROAD && !m_roadController->GetValidNeighbors().empty()) {
        std::tr1::shared_ptr<SpriteAtlas> streetsAtlas = reg.getAtlas("streets");
        if (streetsAtlas) {
            CoordinateSystem& coords = CoordinateSystem::GetInstance();

            float flagAlignOffsetX = 0.0f;
            {   std::tr1::shared_ptr<SpriteAtlas> ba = reg.getAtlas("Buildings");
                uint32_t fi = ba->GetIndex("flag"); const SpriteRegion* fr = ba->GetRegion(fi);
                if (fr) { const std::vector<uint32_t>* rg = streetsAtlas->GetGroup("street_1");
                if (rg && !rg->empty()) { const SpriteRegion* rr = streetsAtlas->GetRegion((*rg)[0]);
                if (rr) { flagAlignOffsetX = (fr->width * 0.5f - fr->pivotX) - (rr->width * 0.5f - rr->pivotX); }}}}

            const std::vector<uint32_t>* group = streetsAtlas->GetGroup("street_1");
            if (group && !group->empty()) {
                uint32_t regionIdx = (*group)[0];
                const SpriteRegion* region = streetsAtlas->GetRegion(regionIdx);
                if (region) {
                    for (size_t i = 0; i < m_roadController->GetValidNeighbors().size(); ++i) {
                        int nx = m_roadController->GetValidNeighbors()[i].first;
                        int ny = m_roadController->GetValidNeighbors()[i].second;
                        float wx, wy;
                        coords.NodeTileToWorld(nx, ny, wx, wy);
                        Graphics::RenderCommandBuilder()
                            .WorldSprite(wx - region->pivotX + flagAlignOffsetX, wy - region->pivotY,
                                (float)region->width, (float)region->height,
                                region->u0, region->v0, region->u1, region->v1,
                                SLOT_STREETS, static_cast<WORD>(0.99f * 65535.0f))
                            .Color(D3DCOLOR_ARGB(120, 255, 100, 100))
                            .Layer(LAYER_FOREGROUND)
                            .Submit(renderQueue);
                    }
                }
            }
        }
    }

    // ─── Render build menu ─────────────────────────────────────────────
    if (m_buildMenu && frame.input.menuActive) {
        m_buildMenu->Render();
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

    // ─── Geologist overlay ─────────────────────────────────────────────
    RenderGeologistOverlay(renderQueue, frame);

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

// ─── RenderCursor ──────────────────────────────────────────────────────
void GameRenderer::RenderCursor(Graphics::RenderQueue* renderQueue, const FrameContext& frame)
{
    TextureRegistry& reg = TextureRegistry::instance();
    std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
    if (!uiAtlas) return;

    uint32_t cursorIdx = uiAtlas->GetIndex("cursor");
    if (cursorIdx == 0xFFFFFFFF) return;

    const SpriteRegion* cursorRegion = uiAtlas->GetRegion(cursorIdx);
    if (!cursorRegion) return;

    SpriteRenderer* spriteRenderer = m_renderer ? m_renderer->GetSpriteRenderer() : NULL;
    if (spriteRenderer) {
        LPDIRECT3DTEXTURE9 uiTex = uiAtlas->GetTexture();
        if (uiTex) spriteRenderer->SetTextureSlot(SLOT_UI_CURSOR, uiTex);
    }

    float worldX, worldY;
    CoordinateSystem::GetInstance().NodeTileToWorld(frame.input.cursorTileX, frame.input.cursorTileY, worldX, worldY);

    Graphics::RenderCommandBuilder()
        .WorldSprite(worldX - cursorRegion->pivotX, worldY - cursorRegion->pivotY,
            (float)cursorRegion->width, (float)cursorRegion->height,
            cursorRegion->u0, cursorRegion->v0, cursorRegion->u1, cursorRegion->v1,
            SLOT_UI_CURSOR, static_cast<WORD>(0.99f * 65535.0f))
        .Layer(LAYER_FOREGROUND)
        .Submit(renderQueue);
}

// ─── RenderGeologistOverlay ────────────────────────────────────────────
void GameRenderer::RenderGeologistOverlay(Graphics::RenderQueue* renderQueue, const FrameContext& frame)
{
    if (!m_map || !renderQueue) return;
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    TextureRegistry& reg = TextureRegistry::instance();
    std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");

    // 1. Mountain highlight
    if (m_map) {
        int cursorX = frame.input.cursorTileX;
        int cursorY = frame.input.cursorTileY;
        const World::Tile& objTile = m_map->GetTile(World::Objects, cursorX, cursorY);
        if (objTile.type == World::Mountain || objTile.type == World::MountainOnWater || objTile.type == World::Rock) {
            SpriteRenderer* sr = m_renderer ? m_renderer->GetSpriteRenderer() : NULL;
            float wx, wy;
            coords.NodeTileToWorld(cursorX, cursorY, wx, wy);
            int tileW = 60, tileH = 30;
            {
                const World::Tile& objTile2 = m_map->GetTile(World::Objects, cursorX, cursorY);
                if (objTile2.regionIndex >= 0) {
                    std::tr1::shared_ptr<SpriteAtlas> maptilesAtlas = reg.getAtlas("maptiles");
                    if (maptilesAtlas) {
                        const SpriteRegion* mountainReg = maptilesAtlas->GetRegion((uint32_t)objTile2.regionIndex);
                        if (mountainReg) {
                            tileW = (int)mountainReg->width;
                            tileH = (int)mountainReg->height;
                        }
                    }
                }
            }
            std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
            LPDIRECT3DTEXTURE9 buildingsTex = buildingsAtlas ? buildingsAtlas->GetTexture() : NULL;
            if (sr && buildingsTex) sr->SetTextureSlot(SLOT_BUILDINGS_HIGHLIGHT, buildingsTex);
            Graphics::RenderCommandBuilder()
                .WorldSprite(wx, wy,
                    (float)tileW, (float)tileH,
                    0.5f, 0.5f, 0.5001f, 0.5001f,
                    SLOT_BUILDINGS_HIGHLIGHT, static_cast<WORD>(0.98f * 65535.0f))
                .Color(D3DCOLOR_ARGB(80, 255, 255, 0))
                .Layer(LAYER_EFFECTS)
                .Submit(renderQueue);
        }
    }

        // 2. Geologist confirmation overlay
        if (m_geologistMenu && m_geologistMenu->IsVisible()) {
            m_geologistMenu->Render();
        if (m_textManager && m_geologistMenu->IsVisible()) {
            std::tr1::shared_ptr<SpriteAtlas> uiAtl = reg.getAtlas("ui");
            std::tr1::shared_ptr<SpriteAtlas> iconAtl = reg.getAtlas("Icon");
            SpriteRenderer* sr2 = m_renderer ? m_renderer->GetSpriteRenderer() : NULL;
            float cx = 640.0f;
            float yOff = 200.0f;
            int iconSize = 40;

            WORD kSlotBg = SLOT_UI_MENU_BG;
            WORD kSlotIcon = SLOT_UI_MENU_ICON;
            auto renderIcon = [&](const char* name, float x, float y, float w, float h, D3DCOLOR fallback) {
                bool ok = false;
                if (sr2) {
                    LPDIRECT3DTEXTURE9 tex = NULL;
                    float u0=0.5f,v0=0.5f,u1=0.5001f,v1=0.5001f;
                    if (uiAtl) {
                        uint32_t idx = uiAtl->GetIndex(name);
                        if (idx != 0xFFFFFFFF) {
                            const SpriteRegion* r = uiAtl->GetRegion(idx);
                            if (r) { u0=r->u0;v0=r->v0;u1=r->u1;v1=r->v1; tex=uiAtl->GetTexture(); }
                        }
                    }
                    if (!tex && iconAtl) {
                        uint32_t idx = iconAtl->GetIndex(name);
                        if (idx != 0xFFFFFFFF) {
                            const SpriteRegion* r = iconAtl->GetRegion(idx);
                            if (r) { u0=r->u0;v0=r->v0;u1=r->u1;v1=r->v1; tex=iconAtl->GetTexture(); }
                        }
                    }
                    if (tex) {
                        WORD slot = tex == uiAtl->GetTexture() ? kSlotBg : kSlotIcon;
                        sr2->SetTextureSlot(slot, tex);
                        Graphics::RenderCommandBuilder()
                            .UIElement(x, y, w, h, u0, v0, u1, v1, slot, 100)
                            .Submit(renderQueue);
                        ok = true;
                    }
                }
                if (!ok) {
                    Graphics::RenderCommandBuilder()
                        .UIElement(x, y, w, h, 0.5f, 0.5f, 0.5001f, 0.5001f, kSlotBg, 100)
                        .Color(fallback)
                        .Submit(renderQueue);
                }
            };

            renderIcon("icon_mountain", cx - 24.0f, yOff, 48.0f, 48.0f, D3DCOLOR_ARGB(200, 140, 110, 80));
            m_textManager->DrawTextCenteredToScreen("Геолог", cx, yOff + 54.0f, D3DCOLOR_ARGB(255, 255, 255, 220), 0.095f, FONT_MENU, FONT_STYLE_NORMAL, LAYER_FOREGROUND);
            renderIcon("icon_geologist", cx - 18.0f, yOff + 90.0f, 36.0f, 36.0f, D3DCOLOR_ARGB(200, 255, 220, 100));
            renderIcon("ornament_1", cx - 50.0f, yOff + 132.0f, 100.0f, 14.0f, D3DCOLOR_ARGB(180, 180, 150, 80));
            m_textManager->DrawTextCenteredToScreen("Отправить геолога для поиска полезных ископаемых",
                cx, yOff + 162.0f, D3DCOLOR_ARGB(255, 200, 200, 200), 0.08f, FONT_MENU, FONT_STYLE_NORMAL, LAYER_FOREGROUND);
        }
    }

    // 3. Resource icons on SURVEYED mountains
    int w = m_map->GetWidth() * 2;
    int h = m_map->GetHeight() * 4;
    SpriteRenderer* sr = m_renderer ? m_renderer->GetSpriteRenderer() : NULL;
    std::tr1::shared_ptr<SpriteAtlas> iconAtlas = reg.getAtlas("Icon");

    for (int y = 0; y < h; y += 1) {
        for (int x = 0; x < w; x += 1) {
            const World::ResourceNode& node = m_map->GetResourceNode(x, y);
            if (!node.surveyed || node.type == World::ResourceType_None || node.amount <= 0) continue;

            float wx, wy;
            coords.NodeTileToWorld(x, y, wx, wy);

            bool iconRendered = false;
            if (iconAtlas && sr) {
                LPDIRECT3DTEXTURE9 iconTex = iconAtlas->GetTexture();
                if (iconTex) {
                    const char* depositName = World::ResourceTypeToDepositIconName(node.type);
                    if (depositName && depositName[0] != '\0') {
                        sr->SetTextureSlot(SLOT_UI_MENU_ICON, iconTex);
                        uint32_t depositIdx = iconAtlas->GetIndex(depositName);
                        if (depositIdx != 0xFFFFFFFF) {
                            const SpriteRegion* depositReg = iconAtlas->GetRegion(depositIdx);
                            if (depositReg) {
                                Graphics::RenderCommandBuilder()
                                    .WorldSprite(wx, wy - 40.0f,
                                        (float)depositReg->width * 0.8f, (float)depositReg->height * 0.8f,
                                        depositReg->u0, depositReg->v0, depositReg->u1, depositReg->v1,
                                        SLOT_UI_MENU_ICON, static_cast<WORD>(0.97f * 65535.0f))
                                    .Color(D3DCOLOR_ARGB(220, 255, 255, 255))
                                    .Layer(LAYER_EFFECTS)
                                    .Submit(renderQueue);
                                iconRendered = true;
                            }
                        }
                    }
                }
            }
            if (!iconRendered) {
                D3DCOLOR fallbackColor = D3DCOLOR_ARGB(200, 255, 255, 0);
                switch (node.type) {
                    case World::ResourceType_Coal:    fallbackColor = D3DCOLOR_ARGB(200, 80, 80, 80);    break;
                    case World::ResourceType_IronOre: fallbackColor = D3DCOLOR_ARGB(200, 180, 100, 50);  break;
                    case World::ResourceType_GoldOre: fallbackColor = D3DCOLOR_ARGB(200, 255, 215, 0);   break;
                    case World::ResourceType_Stone:   fallbackColor = D3DCOLOR_ARGB(200, 150, 150, 150); break;
                    case World::ResourceType_Marble:  fallbackColor = D3DCOLOR_ARGB(200, 200, 180, 220); break;
                    case World::ResourceType_Granite: fallbackColor = D3DCOLOR_ARGB(200, 130, 90, 70);   break;
                    default:                         fallbackColor = D3DCOLOR_ARGB(200, 255, 255, 0);   break;
                }
                Graphics::RenderCommandBuilder()
                    .WorldSprite(wx, wy - 40.0f,
                        24.0f, 24.0f,
                        0.5f, 0.5f, 0.5001f, 0.5001f,
                        SLOT_UI_MENU_ICON, static_cast<WORD>(0.97f * 65535.0f))
                    .Color(fallbackColor)
                    .Layer(LAYER_EFFECTS)
                    .Submit(renderQueue);
            }
        }
    }

    // 4. Geologist working indicator
    if (frame.overlay.geologistState == OverlayFrameState::GEOLOGIST_WORKING && frame.overlay.geologistTileX >= 0 && frame.overlay.geologistTileY >= 0) {
        float wx, wy;
        coords.NodeTileToWorld(frame.overlay.geologistTileX, frame.overlay.geologistTileY, wx, wy);
        bool iconRendered = false;
        if (iconAtlas && sr) {
            LPDIRECT3DTEXTURE9 iconTex = iconAtlas->GetTexture();
            if (iconTex) {
                sr->SetTextureSlot(SLOT_UI_MENU_ICON, iconTex);
                uint32_t workIdx = iconAtlas->GetIndex("icon_geologist_work");
                if (workIdx != 0xFFFFFFFF) {
                    const SpriteRegion* workReg = iconAtlas->GetRegion(workIdx);
                    if (workReg) {
                        Graphics::RenderCommandBuilder()
                            .WorldSprite(wx, wy - 50.0f,
                                (float)workReg->width, (float)workReg->height,
                                workReg->u0, workReg->v0, workReg->u1, workReg->v1,
                                SLOT_UI_MENU_ICON, static_cast<WORD>(0.97f * 65535.0f))
                                .Layer(LAYER_EFFECTS)
                                .Submit(renderQueue);
                        iconRendered = true;
                    }
                }
            }
        }
        if (!iconRendered) {
            Graphics::RenderCommandBuilder()
                .WorldSprite(wx - 16.0f, wy - 50.0f,
                    32.0f, 32.0f,
                    0.5f, 0.5f, 0.5001f, 0.5001f,
                    SLOT_UI_MENU_ICON, static_cast<WORD>(0.97f * 65535.0f))
                .Color(D3DCOLOR_ARGB(180, 255, 255, 0))
                .Layer(LAYER_EFFECTS)
                .Submit(renderQueue);
        }
    }
}

// ─── PushUiToQueue (gamepad cursor + notifications) ────────────────────
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

    // Ensure UI atlas texture is bound for notification backgrounds
    {
        std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
        if (uiAtlas) {
            SpriteRenderer* sr = m_renderer ? m_renderer->GetSpriteRenderer() : NULL;
            if (sr) {
                LPDIRECT3DTEXTURE9 uiTex = uiAtlas->GetTexture();
                if (uiTex) sr->SetTextureSlot(SLOT_UI_MENU_BG, uiTex);
            }
        }
    }

    // ── Notifications ─────────────────────────────────────────────────
    if (frame.ui.notificationCount > 0) {
        float startX = 1280.0f - 280.0f;
        float startY = 20.0f;
        float boxW = 260.0f;
        float boxH = 60.0f;
        float pad = 10.0f;

        for (int i = 0; i < frame.ui.notificationCount; ++i) {
            const UiFrameState::UiNotification& n = frame.ui.notifications[i];
            if (!n.isActive) continue;

            float yPos = startY + (float)i * (boxH + 6.0f);

            // Background box
            Graphics::RenderCommandBuilder()
                .UIElement(startX, yPos, boxW, boxH,
                    0.0f, 0.0f, 1.0f, 1.0f,
                    SLOT_UI_MENU_BG, static_cast<WORD>(0.95f * 65535.0f))
                .Color(D3DCOLOR_ARGB(200, 20, 20, 40))
                .Submit(renderQueue);

            // Title
            m_textManager->DrawString(n.title, startX + pad, yPos + 4.0f, D3DCOLOR_ARGB(255, 255, 200, 80), 0.07f);

            // Line1
            m_textManager->DrawString(n.line1, startX + pad, yPos + 22.0f, D3DCOLOR_ARGB(255, 220, 220, 220), 0.06f);

            // Line2
            if (n.line2[0] != '\0') {
                m_textManager->DrawString(n.line2, startX + pad, yPos + 38.0f, D3DCOLOR_ARGB(255, 180, 180, 180), 0.055f);
            }
        }
    }

}

} // namespace Scene
