#include "stdafx.h"
#include "MapEditor.h"
#include "../Logic/WeightMap.h"
#include "../Logic/CoordinateSystem.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/RenderQueue.h"
#include "../Graphics/RenderLayers.h"
#include <d3dx9.h>
#include "../Graphics/Texture.h"
#include "../Input/Gamepad.h"
#include "../World/MapSerializer.h"
#include "../Graphics/TextureRegistry.h"
#include <algorithm>
#include <cmath>
#include <map>

namespace Editor {

// Forward declarations
static int CalcPatternAt(int x, int y, World::TileLayer* roadsLayer, const std::vector<std::pair<int,int>>& previewPath);

MapEditor::MapEditor()
    : m_map(0)
    , m_tileRenderer(0)
    , m_inputManager(0)
    , m_tilePalette(0)
    , m_toolbarPanel(0)
    , m_currentMode(EditMode_PaintGround)
    , m_brushSize(BrushSize_Single)
    , m_currentTileType(World::None)
    , m_currentTileIndex(0)
    , m_currentLayer(World::Ground)
    , m_weightMap(nullptr)
    , m_renderQueue(0)
    , m_cameraX(0.0f)
    , m_cameraY(0.0f)
    , m_zoomLevel(1.0f)
    , m_hoveredTileX(-1)
    , m_hoveredTileY(-1)
    , m_selectedTileX(-1)
    , m_selectedTileY(-1)
    , m_isDragging(false)
    , m_groundTexture(0)
    , m_groundAtlas(0)
    , m_dotTexture(0)
    , m_roadTexture(0)
    , m_roadAtlas(0)
    , m_roadBuildState(ROAD_IDLE)
    , m_roadStartX(-1)
    , m_roadStartY(-1)
    , m_cursorTileX(0)
    , m_cursorTileY(0)
    , m_placingTile(false)
    , m_previewSpriteIndex(-1)
	, m_showObjects(true)
    , m_showOverlay(true)
, m_showNodes(true)
 	, m_currentObjectAtlasName("maptiles")
    , m_currentObjectGroupName("Tree")
    , m_pCamera(nullptr)
    , m_placementOccupied(true)
    , m_showResourceIcons(false)
    , m_textManager(nullptr)
{
    for (int i = 0; i < World::ResourceType_Count; ++i) m_resourceIconIndices[i] = -1;
}

MapEditor::~MapEditor() {
    delete m_tilePalette;
    delete m_toolbarPanel;
    delete m_tileRenderer;
}

void MapEditor::Initialize(World::Map* map, Renderer* renderer,
                          Input::InputManager* inputManager,
                          IDirect3DDevice9* device)
{
    // Сохраняем переданные зависимости
    m_map = map;
    m_renderer = renderer;
    m_inputManager = inputManager;
    m_pDevice = device;
	SetSpriteRenderer(renderer->GetSpriteRenderer());

    // === ИСПРАВЛЕНИЕ: инициализируем систему координат реальным размером Ground-слоя ===
    int groundWidth  = 20;   // значения по умолчанию (на случай, если слой ещё не создан)
    int groundHeight = 20;
    if (m_map)
    {
        World::TileLayer* groundLayer = m_map->GetLayer(World::Ground);
        if (groundLayer)
        {
            groundWidth  = groundLayer->GetWidth();
            groundHeight = groundLayer->GetHeight();
        }
    }
    CoordinateSystem::GetInstance().Initialize(groundWidth, groundHeight);
    // ============================================================================

    // Кешируем позиции узлов (для быстрого доступа при рендеринге/выборе)
    CacheNodePositions();

    // Загрузка текстур и атласов
    TextureRegistry& registry = TextureRegistry::instance();
    m_groundTexture = registry.getTextureOrLoad("ground");
    m_groundAtlas   = registry.getAtlas("ground");
    m_objectAtlas   = registry.getAtlas("maptiles");
    if (m_spriteRenderer && m_objectAtlas) {
        m_spriteRenderer->SetTextureSlot(9, m_objectAtlas->GetTexture());
    }

    // UI atlas for cursor rendering — slot 4 (slot 12 is used by font)
    std::tr1::shared_ptr<SpriteAtlas> uiAtlas = registry.getAtlas("ui");
    if (uiAtlas && m_spriteRenderer) {
        m_spriteRenderer->SetTextureSlot(4, uiAtlas->GetTexture());
    }

    // Load roads atlas
    m_roadTexture = registry.getTextureOrLoad("streets");
    m_roadAtlas = registry.getAtlas("streets");
    if (m_roadAtlas && m_spriteRenderer) {
        m_spriteRenderer->SetTextureSlot(16, m_roadAtlas->GetTexture());
        char buf[128];
        sprintf_s(buf, "[MapEditor] Roads atlas loaded: %d sprites\n", m_roadAtlas->GetRegionCount());
        OutputDebugStringA(buf);
    }

    // Создаём простую белую точку для весовой карты (используется в RenderWeightMap)
    if (device)
    {
        D3DXCreateTexture(device, 1, 1, 1, 0,
                          D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
                          &m_dotTexture);
        if (m_dotTexture)
        {
            D3DLOCKED_RECT lockRect;
            m_dotTexture->LockRect(0, &lockRect, nullptr, 0);
            *(DWORD*)lockRect.pBits = D3DCOLOR_ARGB(255, 255, 255, 255);
            m_dotTexture->UnlockRect(0);
        }
        if (m_spriteRenderer) {
            m_spriteRenderer->SetTextureSlot(2, m_dotTexture);
        }
    }

    // Инициализируем весовую карту (по умолчанию – глубокая вода)
    m_weightMap = new Logic::WeightMap(GRID_WIDTH, GRID_HEIGHT);
    for (int y = 0; y < GRID_HEIGHT; ++y)
    {
        for (int x = 0; x < GRID_WIDTH; ++x)
        {
            m_weightMap->SetWeight(x, y, Logic::WEIGHT_DEEP_WATER);
        }
    }

    // Создаём рендерер тайлов, если есть рендерер и карта
    if (renderer && map)
    {
        m_tileRenderer = new TileRenderer(renderer,
                                          map->GetWidth(),
                                          map->GetHeight());
        m_tileRenderer->SetMap(map);
    }

    // Инициализация UI (палитра тайлов, панель инструментов)
    CreateUI(device);

    // Заполняем начальное состояние карты (все тайлы получают регион 0 из атласа "ground")
    InitializeMap();

    // Центрируем камеру в центре изометрического алмаза карты
    if (m_map)
    {
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        float cx, cy;
        coords.GetDiamondCenter(cx, cy);
        m_cameraX = cx;
        m_cameraY = cy;
    }

    // === ОТЛАДОЧНЫЙ ВЫВОД: проверяем, что преобразование координат даёт ожидаемый диапазон ===
    // (можно закомментировать/удалить после подтверждения, что карта видна)
    CoordinateSystem& coords = CoordinateSystem::GetInstance();
    float wx0, wy0, wx1, wy1;
    coords.GroundTileToWorld(0, 0, wx0, wy0);              // левый верхний угол карты
    coords.GroundTileToWorld(groundWidth - 1,
                            groundHeight - 1,
                            wx1, wy1);                     // правый нижний угол
    char dbgBuf[256];
    _snprintf(dbgBuf, sizeof(dbgBuf),
              "[DEBUG] Ground tile (0,0) -> World: (%f, %f)\n",
              wx0, wy0);
    OutputDebugStringA(dbgBuf);
    _snprintf(dbgBuf, sizeof(dbgBuf),
              "[DEBUG] Ground tile (%d,%d) -> World: (%f, %f)\n",
              groundWidth-1, groundHeight-1, wx1, wy1);
    OutputDebugStringA(dbgBuf);
    // ========================================================================
}

void MapEditor::Update(float deltaTime) {
    HandleInput();

    if (m_tilePalette) {
        m_tilePalette->Update(deltaTime);
    }
}

void MapEditor::SetCursorWorldPosition(float x, float y) {
    CoordinateSystem& coords = CoordinateSystem::GetInstance();
    if (m_currentLayer == World::Ground) {
        coords.WorldToGroundTile(x, y, m_cursorTileX, m_cursorTileY);
    } else if (m_map) {
        // Snap to nearest cell center for consistent placement
        int bestX, bestY;
        if (m_map->GetTileAt(x, y, m_currentLayer, bestX, bestY)) {
            m_cursorTileX = bestX;
            m_cursorTileY = bestY;
        } else {
            coords.WorldToNodeTile(x, y, m_cursorTileX, m_cursorTileY);
        }
    } else {
        coords.WorldToNodeTile(x, y, m_cursorTileX, m_cursorTileY);
    }

    // Auto-update A* road preview when cursor moves during road building
    if (m_roadBuildState == ROAD_PLACING) {
        UpdateRoadPreview(m_cursorTileX, m_cursorTileY);
    }
}

void MapEditor::RenderGeometry() {
    // Camera update handled exclusively in EditorScene::Update() to prevent double update
    // if (m_pCamera) {
    //     m_pCamera->Update();
    // }

    if (m_pDevice) {
        m_pDevice->SetVertexShader(NULL);
        m_pDevice->SetPixelShader(NULL);
        m_pDevice->SetTexture(0, NULL);
    }

    // Bind font texture to slot 15 for world-space text rendering
    if (m_textManager && m_spriteRenderer) {
        LPDIRECT3DTEXTURE9 fontTex = m_textManager->GetFontTexture(FONT_MENU);
        if (fontTex) {
            m_spriteRenderer->SetTextureSlot(15, fontTex);
        }
    }

    RenderGridLayer();
    if (m_showNodes) {
        RenderWeightMap();
    }

    if (m_showObjects && m_currentLayer == World::Objects) {
    }

    if (m_showOverlay && m_currentLayer == World::Overlay) {
    }

    if (m_pDevice) {
        m_pDevice->SetVertexShader(NULL);
        m_pDevice->SetPixelShader(NULL);
        m_pDevice->SetTexture(0, NULL);
    }
}

void MapEditor::RenderUI() {
    RenderCursor();
    // Amount text is now rendered in RenderGridLayer alongside icons
//    if (m_showResourceIcons) {
//        RenderResources();
//    }
//    RenderTilePreview();
//    RenderActiveTile();
}
void MapEditor::HandleInput() {
    if (!m_inputManager) return;

    Input::Gamepad* gamepad = m_inputManager->GetGamepad();
    if (!gamepad) return;

    if (gamepad->IsButtonPressed(Input::GP_LB)) {
        int currentMode = static_cast<int>(m_currentMode);
        currentMode = (currentMode + 1) % 5;
        m_currentMode = static_cast<EditMode>(currentMode);
    }

    if (gamepad->IsButtonPressed(Input::GP_RB)) {
        int currentSize = static_cast<int>(m_brushSize);
        currentSize = (currentSize + 1) % 4;
        m_brushSize = static_cast<BrushSize>(currentSize);
    }
}

void MapEditor::PaintArea(int centerX, int centerY) {
    if (!m_map) return;

    int brushRadius = static_cast<int>(m_brushSize) / 2;
    World::LayerType layer = World::Ground;
    int gridW = GRID_WIDTH;
    int gridH = GRID_HEIGHT;

switch (m_currentMode) {
        case EditMode_PaintGround:
            layer = World::Ground;
            gridW = GRID_WIDTH;
            gridH = GRID_HEIGHT;
            break;
        case EditMode_PaintObjects: {
            layer = World::Objects;
            World::TileLayer* objectsLayer = m_map->GetLayer(World::Objects);
            gridW = objectsLayer ? objectsLayer->GetWidth() : NODES_W;
            gridH = objectsLayer ? objectsLayer->GetHeight() : NODES_H;
            break;
        }
        case EditMode_Erase:
            layer = World::Ground;
            gridW = GRID_WIDTH;
            gridH = GRID_HEIGHT;
            break;
        default:
            return;
    }

for (int y = centerY - brushRadius; y <= centerY + brushRadius; ++y) {
        for (int x = centerX - brushRadius; x <= centerX + brushRadius; ++x) {
            if (x >= 0 && x < gridW && y >= 0 && y < gridH) {
                if (m_currentMode == EditMode_Erase) {
                    m_map->SetTileType(layer, x, y, World::None);
                } else if (m_currentLayer == World::Ground || m_currentLayer == World::Objects) {
                    int oldCursorX = m_cursorTileX;
                    int oldCursorY = m_cursorTileY;
                    m_cursorTileX = x;
                    m_cursorTileY = y;
                    PaintCurrentTile();
                    m_cursorTileX = oldCursorX;
                    m_cursorTileY = oldCursorY;
                }
            }
        }
    }
}

void MapEditor::PaintTile(int x, int y) {
    PaintArea(x, y);
}

void MapEditor::CreateUI(IDirect3DDevice9* device) {
    m_tilePalette = new UI::TilePalette();
    m_tilePalette->SetPosition(10.0f, 10.0f);
    m_tilePalette->SetSize(200.0f, 300.0f);
    m_tilePalette->SetTileSelectedCallback(OnTileSelected, this);
    m_tilePalette->CreateTileButtons(device);
    m_tilePalette->LayoutButtons();
}

void MapEditor::RenderGrid() {
    if (!m_map) return;
}

void MapEditor::RenderTilePreview() {
}

bool MapEditor::SaveMap(const std::string& filename) {
    if (!m_map) return false;
    return MapSerializer::Save(*m_map, filename, &m_roadFlags);
}

bool MapEditor::LoadMap(const std::string& filename) {
    if (!m_map) return false;
    m_roadFlags.clear();
    return MapSerializer::Load(*m_map, filename, &m_roadFlags);
}

void MapEditor::UpdateCamera(float deltaTime) {
    (void)deltaTime;
    // No limits - free camera movement
}

void MapEditor::OnTileSelected(World::TileType type, void* userData) {
    MapEditor* editor = static_cast<MapEditor*>(userData);
    if (editor) {
        editor->m_currentTileType = type;
    }
}

void MapEditor::InitializeMap() {
    if (!m_map || !m_groundAtlas) return;

    const SpriteRegion* firstRegion = m_groundAtlas->GetRegion(0);
    if (!firstRegion) return;

    for (int layerType = 0; layerType < static_cast<int>(World::LayerCount); ++layerType) {
        World::TileLayer* layer = m_map->GetLayer(static_cast<World::LayerType>(layerType));
        if (!layer) continue;

        for (int y = 0; y < layer->GetHeight(); ++y) {
            for (int x = 0; x < layer->GetWidth(); ++x) {
                World::Tile& tile = layer->GetTile(x, y);
                tile.type = World::None;
                tile.regionIndex = -1;
                tile.u0 = 0.0f;
                tile.v0 = 0.0f;
                tile.u1 = 0.0f;
                tile.v1 = 0.0f;
                tile.atlasName.clear();
            }
        }
    }

    World::TileLayer* groundLayer = m_map->GetLayer(World::Ground);
    if (groundLayer) {
        for (int y = 0; y < groundLayer->GetHeight(); ++y) {
            for (int x = 0; x < groundLayer->GetWidth(); ++x) {
                World::Tile& tile = groundLayer->GetTile(x, y);
                tile.type = World::None;
                tile.regionIndex = 0;
                tile.u0 = firstRegion->u0;
                tile.v0 = firstRegion->v0;
                tile.u1 = firstRegion->u1;
                tile.v1 = firstRegion->v1;
                tile.atlasName = "ground";
            }
        }
    }
}

void MapEditor::RenderGridLayer() {
    if (!m_renderQueue || !m_map || !m_groundAtlas) return;

    World::TileLayer* groundLayer = m_map->GetLayer(World::Ground);
    if (!groundLayer) return;

    WORD groundTexID = 0;

    for (int y = 0; y < groundLayer->GetHeight(); ++y) {
        for (int x = 0; x < groundLayer->GetWidth(); ++x) {
            float wx, wy;
            CoordinateSystem::GetInstance().GroundTileToWorld(x, y, wx, wy);

            const World::Tile& tile = groundLayer->GetTile(x, y);
            float spriteW = 238.0f, spriteH = 148.0f;
            float pivotX = spriteW * 0.5f, pivotY = spriteH * 0.5f;
            if (tile.regionIndex >= 0) {
                const SpriteRegion* region = m_groundAtlas->GetRegion(tile.regionIndex);
                if (region) { spriteW = (float)region->width; spriteH = (float)region->height; pivotX = region->pivotX; pivotY = region->pivotY; }
            }

            Graphics::RenderCommand cmd = {};
            cmd.x = wx + 119.0f - pivotX;
            cmd.y = wy + 74.0f - pivotY;
            cmd.width = spriteW;
            cmd.height = spriteH;
            cmd.u0 = tile.u0;
            cmd.v0 = tile.v0;
            cmd.u1 = tile.u1;
            cmd.v1 = tile.v1;
            cmd.color = 0xFFFFFFFF;
            cmd.textureID = groundTexID;
            cmd.shaderID = SHADER_TERRAIN;
            cmd.blendMode = 1;
            cmd.layer = LAYER_TERRAIN;
            cmd.depth = static_cast<WORD>(0.95f * 65535.0f);
            m_renderQueue->Submit(cmd);
        }
    }


    if (m_currentLayer == World::Overlay) {
        World::TileLayer* overlayLayer = m_map->GetLayer(World::Overlay);
        if (overlayLayer) {
            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            float dotW = coords.GetNodeWidth() * 0.25f;
            float dotH = coords.GetNodeHeight() * 0.25f;

            for (int y = 0; y < overlayLayer->GetHeight(); ++y) {
                for (int x = 0; x < overlayLayer->GetWidth(); ++x) {
                    const World::Tile& tile = overlayLayer->GetTile(x, y);

                    float wx, wy;
                    coords.NodeTileToWorld(x, y, wx, wy);

                    bool hasOverlay = (tile.u1 > tile.u0 && tile.v1 > tile.v0);
                    DWORD dotColor = hasOverlay
                        ? D3DCOLOR_ARGB(180, 50, 200, 50)
                        : D3DCOLOR_ARGB(120, 100, 100, 100);

                    Graphics::RenderCommand cmd = {};
                    cmd.x = wx - dotW * 0.5f;
                    cmd.y = wy - dotH * 0.5f;
                    cmd.width = dotW;
                    cmd.height = dotH;
                    cmd.u0 = 0.0f;
                    cmd.v0 = 0.0f;
                    cmd.u1 = 1.0f;
                    cmd.v1 = 1.0f;
                    cmd.color = dotColor;
                    cmd.textureID = 2;
                    cmd.shaderID = SHADER_TERRAIN;
                    cmd.blendMode = 1;
                    cmd.layer = LAYER_EFFECTS;
                    cmd.depth = static_cast<WORD>(0.97f * 65535.0f);
                    m_renderQueue->Submit(cmd);
                }
            }
        }
    }

    if (m_currentLayer == World::Placement) {
        World::TileLayer* placementLayer = m_map->GetLayer(World::Placement);
        World::TileLayer* objectsLayer = m_map->GetLayer(World::Objects);
        if (placementLayer) {
            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            float dotW = coords.GetNodeWidth() * 0.25f;
            float dotH = coords.GetNodeHeight() * 0.25f;

            for (int y = 0; y < NODES_H; ++y) {
                for (int x = 0; x < NODES_W; ++x) {
                    float wx, wy;
                    coords.NodeTileToWorld(x, y, wx, wy);

                    bool hasObject = false;
                    if (objectsLayer) {
                        const World::Tile& objTile = objectsLayer->GetTile(x, y);
                        hasObject = (objTile.u1 > objTile.u0 && objTile.v1 > objTile.v0);
                    }
                    if (!hasObject) {
                        const World::Tile& pt = placementLayer->GetTile(x, y);
                        hasObject = (pt.regionIndex >= 0);
                    }
                    DWORD dotColor = hasObject
                        ? D3DCOLOR_ARGB(180, 255, 50, 50)
                        : D3DCOLOR_ARGB(120, 100, 100, 100);

                    Graphics::RenderCommand cmd = {};
                    cmd.x = wx - dotW * 0.5f;
                    cmd.y = wy - dotH * 0.5f;
                    cmd.width = dotW;
                    cmd.height = dotH;
                    cmd.u0 = 0.0f;
                    cmd.v0 = 0.0f;
                    cmd.u1 = 1.0f;
                    cmd.v1 = 1.0f;
                    cmd.color = dotColor;
                    cmd.textureID = 2;
                    cmd.shaderID = SHADER_TERRAIN;
                    cmd.blendMode = 1;
                    cmd.layer = LAYER_EFFECTS;
                    cmd.depth = static_cast<WORD>(0.97f * 65535.0f);
                    m_renderQueue->Submit(cmd);
                }
            }
        }
    }

    if (m_currentLayer == World::Resources) {
        TextureRegistry& registry = TextureRegistry::instance();
        std::tr1::shared_ptr<SpriteAtlas> uiAtl = registry.getAtlas("ui");
        if (uiAtl) {
            // Cache resource icon indices on first access
            if (m_resourceIconIndices[World::ResourceType_Wood] < 0) {
                // ResourceIcons group order is atlas-defined; map enum values by sprite name instead.
                for (int i = 1; i < World::ResourceType_Count; ++i) {
                    World::ResourceType rt = static_cast<World::ResourceType>(i);
                    const char* iconName = World::ResourceTypeToIconName(rt);
                    if (iconName && iconName[0]) {
                        uint32_t idx = uiAtl->GetIndex(iconName);
                        m_resourceIconIndices[i] = (idx != 0xFFFFFFFF) ? (int)idx : -1;
                    }
                }
            }

            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            float nodeW = coords.GetNodeWidth();

            // Render resource icons centered above each tile that has a resource node
            for (int y = 0; y < NODES_H; ++y) {
                for (int x = 0; x < NODES_W; ++x) {
                    const World::ResourceNode& rn = m_map->GetResourceNode(x, y);
                    if (rn.type == World::ResourceType_None || !rn.isVisible) continue;

                    float wx, wy;
                    coords.NodeTileToWorld(x, y, wx, wy);

                    if (rn.type <= World::ResourceType_None || rn.type >= World::ResourceType_Count) continue;
                    int iconIdx = m_resourceIconIndices[rn.type];
                    if (iconIdx < 0) continue;

                    const SpriteRegion* iconRegion = uiAtl->GetRegion(iconIdx);
                    if (!iconRegion) continue;

                    float iconW = (float)iconRegion->width;
                    float iconH = (float)iconRegion->height;
                    float pivotX = iconRegion->pivotX;
                    float pivotY = iconRegion->pivotY;
                    // Position icon at sprite pivot (like regular objects)
                    float iconX = wx - pivotX;
                    float iconY = wy - pivotY;

                    Graphics::RenderCommand cmd = {};
                    cmd.x = iconX;
                    cmd.y = iconY;
                    cmd.width = iconW;
                    cmd.height = iconH;
                    cmd.u0 = iconRegion->u0;
                    cmd.v0 = iconRegion->v0;
                    cmd.u1 = iconRegion->u1;
                    cmd.v1 = iconRegion->v1;
                    cmd.color = 0xFFFFFFFF;
                    cmd.textureID = 4;
                    cmd.shaderID = SHADER_TERRAIN;
                    cmd.blendMode = 1;
                    cmd.layer = LAYER_UI;
                    cmd.depth = static_cast<WORD>(0.99f * 65535.0f);
                    m_renderQueue->Submit(cmd);

                    // Отображение количества ресурса в жиле
                    if (m_textManager) {
                        char amountStr[16];
                        sprintf_s(amountStr, "%d", rn.amount);
                        float textX = iconX + iconW * 0.5f;
                        float textY = iconY + iconH + 2.0f;
                        DWORD textColor = (rn.amount > 10) ? 0xFFFFCC00 : 0xFFFF4444;
                        m_textManager->DrawTextToWorld(amountStr, textX, textY, textColor, 0.15f);
                    }
                }
            }
        }
    }

    // Always render placed road sprites (visible on all layers)
    {
        World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
        if (roadsLayer && m_roadAtlas) {
            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            for (int y = 0; y < NODES_H; ++y) {
                for (int x = 0; x < NODES_W; ++x) {
                    const World::Tile& tile = roadsLayer->GetTile(x, y);
                    if (tile.u1 <= tile.u0 || tile.v1 <= tile.v0) continue;
                    if (tile.regionIndex < 0 || tile.atlasName != "streets") continue;

                    float wx, wy;
                    coords.NodeTileToWorld(x, y, wx, wy);

                    const SpriteRegion* region = m_roadAtlas->GetRegion(tile.regionIndex);
                    if (!region) continue;

                    Graphics::RenderCommand cmd = {};
                    cmd.x = wx - region->pivotX;
                    cmd.y = wy - region->pivotY;
                    cmd.width = (float)region->width;
                    cmd.height = (float)region->height;
                    cmd.u0 = region->u0;
                    cmd.v0 = region->v0;
                    cmd.u1 = region->u1;
                    cmd.v1 = region->v1;
                    cmd.color = 0xFFFFFFFF;
                    cmd.textureID = 16;
                    cmd.shaderID = SHADER_TERRAIN;
                    cmd.blendMode = 0;
                    cmd.layer = LAYER_WORLD;
                    cmd.depth = static_cast<WORD>(0.96f * 65535.0f);
                    m_renderQueue->Submit(cmd);
                }
            }
        }
    }

    // Render road flags (FlagStreets) on top of roads
    if (m_roadAtlas && !m_roadFlags.empty()) {
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        const std::vector<uint32_t>* flagGroup = m_roadAtlas->GetGroup("FlagStreets");
        if (flagGroup && !flagGroup->empty()) {
            uint32_t flagIdx = (*flagGroup)[0];
            const SpriteRegion* flagRegion = m_roadAtlas->GetRegion(flagIdx);
            if (flagRegion) {
                for (size_t fi = 0; fi < m_roadFlags.size(); ++fi) {
                    int fx = m_roadFlags[fi].first;
                    int fy = m_roadFlags[fi].second;
                    float wx, wy;
                    coords.NodeTileToWorld(fx, fy, wx, wy);
                    Graphics::RenderCommand cmd = {};
                    cmd.x = wx - flagRegion->pivotX;
                    cmd.y = wy - flagRegion->pivotY;
                    cmd.width = (float)flagRegion->width;
                    cmd.height = (float)flagRegion->height;
                    cmd.u0 = flagRegion->u0;
                    cmd.v0 = flagRegion->v0;
                    cmd.u1 = flagRegion->u1;
                    cmd.v1 = flagRegion->v1;
                    cmd.color = 0xFFFFFFFF;
                    cmd.textureID = 16;
                    cmd.shaderID = SHADER_TERRAIN;
                    cmd.blendMode = 1;
                    cmd.layer = LAYER_WORLD;
                    cmd.depth = static_cast<WORD>(0.97f * 65535.0f);
                    m_renderQueue->Submit(cmd);
                }
            }
        }
    }

    // Render road preview path (A* pathfinding preview)
    // Shows correct sprites using same street_X formula as committed roads
    if (m_roadBuildState == ROAD_PLACING && !m_roadPreviewPath.empty() && m_roadAtlas) {
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);

        for (size_t i = 1; i < m_roadPreviewPath.size(); ++i) {
            int px = m_roadPreviewPath[i].first;
            int py = m_roadPreviewPath[i].second;

            int pattern = CalcPatternAt(px, py, roadsLayer, m_roadPreviewPath);

            char groupBuf[16];
            const char* groupName = groupBuf;
            switch (pattern) {
                case 0:  groupName = "street_1"; break;
                case 1:  groupName = "street_1"; break;
                case 2:  groupName = "street_2"; break;
                case 3:  sprintf_s(groupBuf, "street_%d", 3); break;
                case 4:  groupName = "street_1"; break;
                case 5:  groupName = "street_1"; break;
                case 6:  sprintf_s(groupBuf, "street_%d", 6); break;
                case 7:  sprintf_s(groupBuf, "street_%d", 7); break;
                case 8:  groupName = "street_2"; break;
                case 9:  sprintf_s(groupBuf, "street_%d", 9); break;
                case 10: groupName = "street_2"; break;
                case 11: sprintf_s(groupBuf, "street_%d", 11); break;
                case 12: sprintf_s(groupBuf, "street_%d", 12); break;
                case 13: sprintf_s(groupBuf, "street_%d", 13); break;
                case 14: sprintf_s(groupBuf, "street_%d", 14); break;
                case 15: sprintf_s(groupBuf, "street_%d", 15); break;
            }

            const std::vector<uint32_t>* group = m_roadAtlas->GetGroup(groupName);
            if (!group || group->empty()) {
                group = m_roadAtlas->GetGroup("street_1");
                if (!group || group->empty()) continue;
            }

            uint32_t regionIdx = (*group)[0];
            const SpriteRegion* region = m_roadAtlas->GetRegion(regionIdx);
            if (!region) continue;

            float wx, wy;
            coords.NodeTileToWorld(px, py, wx, wy);

            Graphics::RenderCommand cmd = {};
            cmd.x = wx - region->pivotX;
            cmd.y = wy - region->pivotY;
            cmd.width = (float)region->width;
            cmd.height = (float)region->height;
            cmd.u0 = region->u0;
            cmd.v0 = region->v0;
            cmd.u1 = region->u1;
            cmd.v1 = region->v1;
            cmd.color = D3DCOLOR_ARGB(160, 255, 255, 255);
            cmd.textureID = 16;
            cmd.shaderID = SHADER_TERRAIN;
            cmd.blendMode = 1;
            cmd.layer = LAYER_WORLD;
            cmd.depth = static_cast<WORD>(0.955f * 65535.0f);
            m_renderQueue->Submit(cmd);
        }
    }

    if (m_currentLayer == World::Roads) {
        World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
        if (roadsLayer) {
            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            float dotW = coords.GetNodeWidth() * 0.25f;
            float dotH = coords.GetNodeHeight() * 0.25f;

            for (int y = 0; y < NODES_H; ++y) {
                for (int x = 0; x < NODES_W; ++x) {
                    const World::Tile& tile = roadsLayer->GetTile(x, y);
                    bool occupied = (tile.u1 > tile.u0 && tile.v1 > tile.v0);
                    if (occupied) continue;

                    float wx, wy;
                    coords.NodeTileToWorld(x, y, wx, wy);

                    DWORD dotColor = D3DCOLOR_ARGB(120, 100, 100, 100);
                    Graphics::RenderCommand cmd = {};
                    cmd.x = wx - dotW * 0.5f;
                    cmd.y = wy - dotH * 0.5f;
                    cmd.width = dotW;
                    cmd.height = dotH;
                    cmd.u0 = 0.0f;
                    cmd.v0 = 0.0f;
                    cmd.u1 = 1.0f;
                    cmd.v1 = 1.0f;
                    cmd.color = dotColor;
                    cmd.textureID = 2;
                    cmd.shaderID = SHADER_TERRAIN;
                    cmd.blendMode = 1;
                    cmd.layer = LAYER_EFFECTS;
                    cmd.depth = static_cast<WORD>(0.97f * 65535.0f);
                    m_renderQueue->Submit(cmd);
                }
            }
        }
    }

    {
        World::TileLayer* objectsLayer = m_map->GetLayer(World::Objects);
        if (objectsLayer && m_objectAtlas) {
            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            TextureRegistry& reg = TextureRegistry::instance();

            // Build per-atlas texture slot mapping for all unique atlases on the map
            std::map<std::string, WORD> atlasSlots;
            WORD nextSlot = 20;

            for (int y = 0; y < objectsLayer->GetHeight(); ++y) {
                for (int x = 0; x < objectsLayer->GetWidth(); ++x) {
                    const World::Tile& tile = objectsLayer->GetTile(x, y);
                    if (tile.u1 <= tile.u0 || tile.v1 <= tile.v0) continue;
                    if (tile.atlasName.empty() || atlasSlots.count(tile.atlasName)) continue;

                    WORD slot = (tile.atlasName == m_currentObjectAtlasName) ? 9 : nextSlot++;
                    std::tr1::shared_ptr<SpriteAtlas> atlas = reg.getAtlas(tile.atlasName);
                    if (atlas.get() && atlas->GetTexture()) {
                        if (m_spriteRenderer) m_spriteRenderer->SetTextureSlot(slot, atlas->GetTexture());
                        atlasSlots[tile.atlasName] = slot;
                    }
                }
            }

            for (int y = 0; y < objectsLayer->GetHeight(); ++y) {
                for (int x = 0; x < objectsLayer->GetWidth(); ++x) {
                    const World::Tile& tile = objectsLayer->GetTile(x, y);
                    if (tile.u1 <= tile.u0 || tile.v1 <= tile.v0) continue;

                    float wx, wy;
                    coords.NodeTileToWorld(x, y, wx, wy);

                    // Use the correct texture slot for this tile's original atlas
                    WORD texSlot = 9;
                    if (!tile.atlasName.empty() && atlasSlots.count(tile.atlasName))
                        texSlot = atlasSlots[tile.atlasName];

                    // Get sprite dimensions and pivot from the tile's original atlas
                    float spriteW = 119.0f, spriteH = 72.0f;
                    float pivotX = 0.0f, pivotY = 0.0f;
                    if (tile.regionIndex >= 0) {
                        std::tr1::shared_ptr<SpriteAtlas> tileAtlas = reg.getAtlas(tile.atlasName);
                        if (!tileAtlas.get()) tileAtlas = m_objectAtlas;
                        const SpriteRegion* region = tileAtlas->GetRegion(tile.regionIndex);
                        if (region) { spriteW = (float)region->width; spriteH = (float)region->height; pivotX = region->pivotX; pivotY = region->pivotY; }
                    }

                    Graphics::RenderCommand cmd = {};
                    cmd.x = wx - pivotX;
                    cmd.y = wy - pivotY;
                    cmd.width = spriteW;
                    cmd.height = spriteH;
                    cmd.u0 = tile.u0;
                    cmd.v0 = tile.v0;
                    cmd.u1 = tile.u1;
                    cmd.v1 = tile.v1;
                    cmd.color = 0xFFFFFFFF;
                    cmd.textureID = texSlot;
                    cmd.shaderID = SHADER_TERRAIN;
                    cmd.blendMode = 0;
                    cmd.layer = LAYER_WORLD;
                    cmd.depth = static_cast<WORD>(0.96f * 65535.0f);
                    m_renderQueue->Submit(cmd);
                }
            }
        }
    }
}

void MapEditor::RenderCursor() {
    if (!m_renderQueue || !m_groundAtlas) return;

    // Roads layer: show streetCursor sprite instead of the brush preview
    if (m_currentLayer == World::Roads && m_roadAtlas) {
        const std::vector<uint32_t>* cursorGroup = m_roadAtlas->GetGroup("streetCursor");
        if (cursorGroup && !cursorGroup->empty()) {
            uint32_t regionIdx = (*cursorGroup)[0];
            const SpriteRegion* previewRegion = m_roadAtlas->GetRegion(regionIdx);
            if (previewRegion) {
                float cursorWorldX, cursorWorldY;
                CoordinateSystem::GetInstance().NodeTileToWorld(m_cursorTileX, m_cursorTileY, cursorWorldX, cursorWorldY);
                float cursorW = static_cast<float>(previewRegion->width);
                float cursorH = static_cast<float>(previewRegion->height);
                cursorWorldX -= previewRegion->pivotX;
                cursorWorldY -= previewRegion->pivotY;

                Graphics::RenderCommand cmd = {};
                cmd.x = cursorWorldX;
                cmd.y = cursorWorldY;
                cmd.width = cursorW;
                cmd.height = cursorH;
                cmd.u0 = previewRegion->u0;
                cmd.v0 = previewRegion->v0;
                cmd.u1 = previewRegion->u1;
                cmd.v1 = previewRegion->v1;
                cmd.color = 0xFFFFFFFF;
                cmd.textureID = 16;
                cmd.shaderID = SHADER_TERRAIN;
                cmd.blendMode = 1;
                cmd.layer = LAYER_EFFECTS;
                cmd.depth = static_cast<WORD>(0.99f * 65535.0f);
                m_renderQueue->Submit(cmd);
                return;
            }
        }
    }

    int spriteToRender = m_previewSpriteIndex;
    if (spriteToRender < 0) {
        spriteToRender = m_activeSpriteIndex;
    }

    // If a sprite is selected (preview or active), render it
    if (spriteToRender >= 0) {
        SpriteAtlas* atlas = nullptr;
        WORD texID = 0;
        if (m_currentLayer == World::Objects) {
            atlas = m_objectAtlas.get();
            texID = 9;
        } else {
            atlas = m_groundAtlas.get();
            texID = 0;
        }
        if (atlas && spriteToRender < (int)atlas->GetRegionCount()) {
            const SpriteRegion* previewRegion = atlas->GetRegion(spriteToRender);
            if (previewRegion) {
                float cursorWorldX, cursorWorldY;
                float cursorW, cursorH;

                if (m_currentLayer == World::Ground) {
                    CoordinateSystem::GetInstance().GroundTileToWorld(m_cursorTileX, m_cursorTileY, cursorWorldX, cursorWorldY);
                    cursorW = static_cast<float>(previewRegion->width);
                    cursorH = static_cast<float>(previewRegion->height);
                    cursorWorldX += 119.0f - previewRegion->pivotX;
                    cursorWorldY += 74.0f - previewRegion->pivotY;
                } else {
                    CoordinateSystem::GetInstance().NodeTileToWorld(m_cursorTileX, m_cursorTileY, cursorWorldX, cursorWorldY);
                    cursorW = static_cast<float>(previewRegion->width);
                    cursorH = static_cast<float>(previewRegion->height);
                    cursorWorldX -= previewRegion->pivotX;
                    cursorWorldY -= previewRegion->pivotY;
                }

                Graphics::RenderCommand cmd = {};
                cmd.x = cursorWorldX;
                cmd.y = cursorWorldY;
                cmd.width = cursorW;
                cmd.height = cursorH;
                cmd.u0 = previewRegion->u0;
                cmd.v0 = previewRegion->v0;
                cmd.u1 = previewRegion->u1;
                cmd.v1 = previewRegion->v1;
                cmd.color = 0xFFFFFFFF;
                cmd.textureID = texID;
                cmd.shaderID = SHADER_TERRAIN;
                cmd.blendMode = 1;
                cmd.layer = LAYER_EFFECTS;
                cmd.depth = static_cast<WORD>(0.99f * 65535.0f);
                m_renderQueue->Submit(cmd);
                return; // Return so we don't draw the fallback outline
            }
        }
    }

    // Fallback: render cursor outline
    TextureRegistry& reg = TextureRegistry::instance();
    std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
    LPDIRECT3DTEXTURE9 uiTex = uiAtlas ? uiAtlas->GetTexture() : NULL;
    if (!uiTex) return;

    const char* cursorName = (m_currentLayer == World::Ground) ? "background_cursor_red" : "cursor";
    const SpriteRegion* cursorRegion = NULL;
    float u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;
    if (uiAtlas) {
        uint32_t cursorIdx = uiAtlas->GetIndex(cursorName);
        if (cursorIdx != 0xFFFFFFFF) {
            cursorRegion = uiAtlas->GetRegion(cursorIdx);
        }
    }
    if (cursorRegion) {
        u0 = cursorRegion->u0; v0 = cursorRegion->v0;
        u1 = cursorRegion->u1; v1 = cursorRegion->v1;
    }

    const SpriteRegion* firstRegion = m_groundAtlas->GetRegion(0);
    if (!firstRegion) return;

    float cursorWorldX, cursorWorldY;
    float cursorW, cursorH;

    if (m_currentLayer == World::Ground) {
        CoordinateSystem::GetInstance().GroundTileToWorld(m_cursorTileX, m_cursorTileY, cursorWorldX, cursorWorldY);
        cursorW = static_cast<float>(firstRegion->width);
        cursorH = static_cast<float>(firstRegion->height);
    } else {
        CoordinateSystem::GetInstance().NodeTileToWorld(m_cursorTileX, m_cursorTileY, cursorWorldX, cursorWorldY);
        cursorW = CoordinateSystem::GetInstance().GetNodeWidth();
        cursorH = CoordinateSystem::GetInstance().GetNodeHeight();
        if (cursorRegion) {
            cursorWorldX -= cursorRegion->pivotX;
            cursorWorldY -= cursorRegion->pivotY;
        }
    }

    Graphics::RenderCommand cmd = {};
    cmd.x = cursorWorldX;
    cmd.y = cursorWorldY;
    cmd.width = cursorW;
    cmd.height = cursorH;
    cmd.u0 = u0;
    cmd.v0 = v0;
    cmd.u1 = u1;
    cmd.v1 = v1;
    cmd.color = 0xFFFFFFFF;
    cmd.textureID = 4;
    cmd.shaderID = SHADER_TERRAIN;
    cmd.blendMode = 1;
    cmd.layer = LAYER_FOREGROUND;
    cmd.depth = static_cast<WORD>(0.99f * 65535.0f);
    m_renderQueue->Submit(cmd);
}

void MapEditor::SetLayer(World::LayerType layer) {
    if (layer != World::Roads) CancelRoad();
    m_currentLayer = layer;
    m_placingTile = false;
    m_currentTileIndex = -1;
    m_activeSpriteIndex = -1;
    if (layer == World::Roads && m_roadAtlas && m_roadAtlas->GetRegionCount() > 0) {
        m_currentTileIndex = 0;
        m_activeSpriteIndex = 0;
    }
}

void MapEditor::SetTileByIndex(int index) {
    m_currentTileIndex = index;
    m_activeSpriteIndex = index; // Update active index
    m_placingTile = true;
}

void MapEditor::SetObjectGroup(const char* groupName) {
    if (groupName) {
        m_currentObjectGroupName = groupName;
        m_currentObjectAtlasName = "maptiles";
        TextureRegistry& registry = TextureRegistry::instance();
        m_objectAtlas = registry.getAtlas("maptiles");
        if (m_spriteRenderer && m_objectAtlas) {
            m_spriteRenderer->SetTextureSlot(9, m_objectAtlas->GetTexture());
        }
        char buf[128];
        sprintf_s(buf, "[MapEditor] SetObjectGroup: '%s'\n", groupName);
        OutputDebugStringA(buf);
    }
}

void MapEditor::ClearPlacementFootprint(int tx, int ty, World::TileLayer* objectsLayer) {
    World::TileLayer* placementLayer = m_map->GetLayer(World::Placement);
    if (!placementLayer || !objectsLayer) return;

    const World::Tile& oldTile = objectsLayer->GetTile(tx, ty);
    if (oldTile.regionIndex < 0) return;

    uint32_t oldCollW = 1, oldCollH = 1;
    int offX = 0, offY = 0;
    std::vector<std::pair<int,int> > collMask;
    if (!oldTile.atlasName.empty()) {
        std::tr1::shared_ptr<SpriteAtlas> atlas = TextureRegistry::instance().getAtlas(oldTile.atlasName);
        if (atlas.get()) {
            const SpriteRegion* oldRegion = atlas->GetRegion(oldTile.regionIndex);
            if (oldRegion) {
                oldCollW = oldRegion->collWidth;
                oldCollH = oldRegion->collHeight;
                CoordinateSystem& coords = CoordinateSystem::GetInstance();
                if (oldRegion->collOffX != 0 || oldRegion->collOffY != 0) {
                    offX = oldRegion->collOffX;
                    offY = oldRegion->collOffY;
                } else {
                    float pivotDX = oldRegion->pivotX - oldRegion->width * 0.5f;
                    float pivotDY = oldRegion->pivotY - oldRegion->height * 0.5f;
                    offX = -(int)(pivotDX / coords.GetNodeWidth());
                    offY = -(int)(pivotDY / coords.GetNodeHeight());
                }
                collMask = oldRegion->collMask;
            }
        }
    }

    int pw = placementLayer->GetWidth();
    int ph = placementLayer->GetHeight();
    if (!collMask.empty()) {
        for (size_t i = 0; i < collMask.size(); ++i) {
            int fx = tx + offX + collMask[i].first;
            int fy = ty + offY + collMask[i].second;
            if (fx >= 0 && fx < pw && fy >= 0 && fy < ph) {
                World::Tile& pt = placementLayer->GetTile(fx, fy);
                pt.regionIndex = -1;
                pt.type = World::None;
                pt.atlasName.clear();
                pt.walkable = true;
                pt.buildable = true;
            }
        }
    } else {
        for (uint32_t dy = 0; dy < oldCollH; ++dy) {
            for (uint32_t dx = 0; dx < oldCollW; ++dx) {
                int fx = tx + offX + (int)dx;
                int fy = ty + offY + (int)dy;
                if (fx >= 0 && fx < pw && fy >= 0 && fy < ph) {
                    World::Tile& pt = placementLayer->GetTile(fx, fy);
                    pt.regionIndex = -1;
                    pt.type = World::None;
                    pt.atlasName.clear();
                    pt.walkable = true;
                    pt.buildable = true;
                }
            }
        }
    }
}

static bool IsMountainTileType(World::TileType type) {
    return type == World::Mountain || type == World::MountainOnWater;
}

static void SetObjectInteractionTile(World::TileLayer* layer, int x, int y, int ownerX, int ownerY, const World::Tile& objectTile) {
    if (!layer) return;
    if (x < 0 || y < 0 || x >= layer->GetWidth() || y >= layer->GetHeight()) return;

    World::Tile& marker = layer->GetTile(x, y);
    marker.type = objectTile.type;
    marker.x = ownerX;
    marker.y = ownerY;
    marker.regionIndex = objectTile.regionIndex;
    marker.u0 = 0.0f;
    marker.v0 = 0.0f;
    marker.u1 = 1.0f;
    marker.v1 = 1.0f;
    marker.atlasName = objectTile.atlasName;
    marker.walkable = objectTile.walkable;
    marker.buildable = false;
}

static int MarkObjectInteractionTile(World::TileLayer* layer, int x, int y, int ownerX, int ownerY, const World::Tile& objectTile) {
    if (!layer) return 0;
    if (x < 0 || y < 0 || x >= layer->GetWidth() || y >= layer->GetHeight()) return 0;

    const World::Tile& oldMarker = layer->GetTile(x, y);
    bool wasSame = oldMarker.x == ownerX && oldMarker.y == ownerY
        && oldMarker.type == objectTile.type && oldMarker.regionIndex == objectTile.regionIndex;
    SetObjectInteractionTile(layer, x, y, ownerX, ownerY, objectTile);
    return wasSame ? 0 : 1;
}

void MapEditor::ClearObjectInteractionZone(int tx, int ty, World::TileLayer* objectsLayer) {
    if (!m_map || !objectsLayer) return;

    World::TileLayer* zoneLayer = m_map->GetLayer(World::Resources);
    if (!zoneLayer) return;

    const World::Tile& oldTile = objectsLayer->GetTile(tx, ty);
    if (oldTile.regionIndex < 0) return;

    for (int y = 0; y < zoneLayer->GetHeight(); ++y) {
        for (int x = 0; x < zoneLayer->GetWidth(); ++x) {
            World::Tile& marker = zoneLayer->GetTile(x, y);
            if (marker.x == tx && marker.y == ty && marker.type == oldTile.type
                && marker.regionIndex == oldTile.regionIndex && marker.atlasName == oldTile.atlasName) {
                marker = World::Tile();
            }
        }
    }
}

void MapEditor::MarkObjectInteractionZone(int tx, int ty, const World::Tile& objectTile, const SpriteRegion* region) {
    if (!m_map || !region) return;

    World::TileLayer* zoneLayer = m_map->GetLayer(World::Resources);
    if (!zoneLayer) return;

    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    int marked = 0;

    // Always mark the anchor node.
    marked += MarkObjectInteractionTile(zoneLayer, tx, ty, tx, ty, objectTile);

    // Mark the collision footprint when authored.
    int offX, offY;
    if (region->collOffX != 0 || region->collOffY != 0) {
        offX = region->collOffX;
        offY = region->collOffY;
    } else {
        float pivotDX = region->pivotX - region->width * 0.5f;
        float pivotDY = region->pivotY - region->height * 0.5f;
        offX = -(int)(pivotDX / coords.GetNodeWidth());
        offY = -(int)(pivotDY / coords.GetNodeHeight());
    }

    if (!region->collMask.empty()) {
        for (size_t i = 0; i < region->collMask.size(); ++i) {
            marked += MarkObjectInteractionTile(zoneLayer, tx + offX + region->collMask[i].first,
                ty + offY + region->collMask[i].second, tx, ty, objectTile);
        }
    } else {
        for (uint32_t dy = 0; dy < region->collHeight; ++dy) {
            for (uint32_t dx = 0; dx < region->collWidth; ++dx) {
                marked += MarkObjectInteractionTile(zoneLayer, tx + offX + (int)dx, ty + offY + (int)dy, tx, ty, objectTile);
            }
        }
    }

    // Mountains are large visual objects. Mark a generous node rectangle around the
    // sprite bounds, so clicking any visible part of the mountain is understood as mountain area.
    if (IsMountainTileType(objectTile.type)) {
        int radiusX = max(2, (int)((region->width + coords.GetNodeWidth() - 1.0f) / coords.GetNodeWidth()) + 2);
        int radiusY = max(2, (int)((region->height + coords.GetNodeHeight() - 1.0f) / (coords.GetNodeHeight() * 0.5f)) + 2);
        for (int y = max(0, ty - radiusY); y <= min(zoneLayer->GetHeight() - 1, ty + radiusY); ++y) {
            for (int x = max(0, tx - radiusX); x <= min(zoneLayer->GetWidth() - 1, tx + radiusX); ++x) {
                marked += MarkObjectInteractionTile(zoneLayer, x, y, tx, ty, objectTile);
            }
        }
    }

    char buf[192];
    sprintf_s(buf, "[MapEditor] Marked interaction zone: %s owner=(%d,%d) sprite=%s cells=%d\n",
        World::TileTypeToString(objectTile.type), tx, ty, region->name.c_str(), marked);
    OutputDebugStringA(buf);
}

void MapEditor::PaintCurrentTile() {
    if (!m_map) return;
    if (m_currentTileIndex < 0 && m_currentLayer != World::Placement) return;

    World::TileLayer* layer = m_map->GetLayer(m_currentLayer);
    if (!layer) return;

    int tileX = m_cursorTileX;
    int tileY = m_cursorTileY;

    if (tileX < 0 || tileX >= layer->GetWidth() || tileY < 0 || tileY >= layer->GetHeight()) return;

    // Erase mode: clear object and its Placement footprint
    if (m_currentMode == EditMode_Erase && m_currentLayer == World::Objects) {
        World::TileLayer* objectsLayer = m_map->GetLayer(World::Objects);
        if (objectsLayer) {
            const World::Tile& oldTile = objectsLayer->GetTile(tileX, tileY);
            if (oldTile.regionIndex >= 0) {
                ClearPlacementFootprint(tileX, tileY, objectsLayer);
                ClearObjectInteractionZone(tileX, tileY, objectsLayer);
            }
            World::Tile& eraseTile = objectsLayer->GetTile(tileX, tileY);
            eraseTile = World::Tile();
        }
        return;
    }

    World::Tile& tile = layer->GetTile(tileX, tileY);

    if (m_currentLayer == World::Objects) {
        // Clear old placement footprint if tile already has an object
        World::TileLayer* objectsLayer = m_map->GetLayer(World::Objects);
        if (objectsLayer) {
            const World::Tile& oldTile = objectsLayer->GetTile(tileX, tileY);
            if (oldTile.regionIndex >= 0) {
                ClearPlacementFootprint(tileX, tileY, objectsLayer);
                ClearObjectInteractionZone(tileX, tileY, objectsLayer);
            }
        }

        if (!m_objectAtlas) return;
        if (m_currentTileIndex >= (int)m_objectAtlas->GetRegionCount()) return;
        const SpriteRegion* region = m_objectAtlas->GetRegion(m_currentTileIndex);
        if (!region) return;

        // Check terrain suitability (Nodes layer weight) for the cursor tile
        World::TileType objType = GetObjectTypeByIndex(m_currentTileIndex);
        if (!CanPlaceObject(tileX, tileY, objType)) {
            OutputDebugStringA("[Editor] Запрещено: данный объект нельзя разместить на этом типе местности!\n");
            return;
        }

        // Check occupancy: if ANY tile in the collision footprint is occupied, deny placement
        if (!IsPlacementFootprintFree(tileX, tileY, region)) {
            return;
        }

        tile.u0 = region->u0;
        tile.v0 = region->v0;
        tile.u1 = region->u1;
        tile.v1 = region->v1;
        tile.regionIndex = m_currentTileIndex;
        tile.type = GetObjectTypeByIndex(m_currentTileIndex);
        tile.atlasName = "maptiles";
        tile.walkable = !region->blocksMovement;

        // Collision tile offset: use stored collOffX/collOffY if set, else compute from pivot
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        int offX, offY;
        if (region->collOffX != 0 || region->collOffY != 0) {
            offX = region->collOffX;
            offY = region->collOffY;
        } else {
            float pivotDX = region->pivotX - region->width * 0.5f;
            float pivotDY = region->pivotY - region->height * 0.5f;
            offX = -(int)(pivotDX / coords.GetNodeWidth());
            offY = -(int)(pivotDY / coords.GetNodeHeight());
        }

        // Fill Placement layer for collision footprint
        World::TileLayer* placementLayer = m_map->GetLayer(World::Placement);
        if (placementLayer) {
            int pw = placementLayer->GetWidth();
            int ph = placementLayer->GetHeight();
            if (!region->collMask.empty()) {
                for (size_t i = 0; i < region->collMask.size(); ++i) {
                    int fx = tileX + offX + region->collMask[i].first;
                    int fy = tileY + offY + region->collMask[i].second;
                    if (fx >= 0 && fx < pw && fy >= 0 && fy < ph) {
                        World::Tile& pt = placementLayer->GetTile(fx, fy);
                        pt.regionIndex = m_currentTileIndex;
                        pt.type = tile.type;
                        pt.atlasName = "maptiles";
                        pt.walkable = !region->blocksMovement;
                        pt.buildable = false;
                    }
                }
            } else {
                for (uint32_t dy = 0; dy < region->collHeight; ++dy) {
                    for (uint32_t dx = 0; dx < region->collWidth; ++dx) {
                        int fx = tileX + offX + (int)dx;
                        int fy = tileY + offY + (int)dy;
                        if (fx >= 0 && fx < pw && fy >= 0 && fy < ph) {
                            World::Tile& pt = placementLayer->GetTile(fx, fy);
                            pt.regionIndex = m_currentTileIndex;
                            pt.type = tile.type;
                            pt.atlasName = "maptiles";
                            pt.walkable = !region->blocksMovement;
                            pt.buildable = false;
                        }
                    }
                }
            }

            // Auto-assign resource for trees/mountains/water objects
            World::ResourceType autoRt = World::TileTypeToResourceType(tile.type);
            if (autoRt != World::ResourceType_None) {
                World::ResourceNode& rn = m_map->GetResourceNode(tileX, tileY);
                if (rn.type == World::ResourceType_None) {
                    rn.type = autoRt;
                    rn.amount = World::GetDefaultResourceAmount(autoRt);
                    rn.isVisible = true;
                }
            }
        }
        MarkObjectInteractionZone(tileX, tileY, tile, region);
    } else if (m_currentLayer == World::Ground) {
        if (!m_groundAtlas) return;
        if (m_currentTileIndex >= (int)m_groundAtlas->GetRegionCount()) return;
        const SpriteRegion* region = m_groundAtlas->GetRegion(m_currentTileIndex);
        if (!region) return;
        tile.u0 = region->u0;
        tile.v0 = region->v0;
        tile.u1 = region->u1;
        tile.v1 = region->v1;
        tile.regionIndex = m_currentTileIndex;
        tile.atlasName = "ground";

        // First, initialize ALL nodes in this tile's area to Land (default)
        for (int dy = 0; dy < 4; dy++) {
            int maxDx = (dy % 2 == 1) ? 2 : 1; // odd rows have 3 columns, even have 2
            for (int dx = 0; dx <= maxDx; dx++) {
                int nx = tileX * 2 + dx;
                int ny = tileY * 4 + dy;
                if (nx >= 0 && nx < NODES_W && ny >= 0 && ny < NODES_H) {
                    m_map->SetNodeWeight(nx, ny, World::Weight_Land);
                }
            }
        }

        // Then apply per-node weight entries (overrides Land where needed)
        for (size_t wi = 0; wi < region->nodeWeightEntries.size(); ++wi) {
            int nx = tileX * 2 + region->nodeWeightEntries[wi].nx;
            int ny = tileY * 4 + region->nodeWeightEntries[wi].ny;
            if (nx >= 0 && nx < NODES_W && ny >= 0 && ny < NODES_H) {
                m_map->SetNodeWeight(nx, ny, region->nodeWeightEntries[wi].weight);
            }
        }
    } else if (m_currentLayer == World::Placement) {
        tile.regionIndex = m_placementOccupied ? 1 : -1;
        tile.buildable = !m_placementOccupied;
        tile.walkable = !m_placementOccupied;
        tile.type = World::None;
        tile.atlasName.clear();
    }
}

void MapEditor::StartRoad(int x, int y) {
    if (!m_map || !m_roadAtlas) return;
    if (x < 0 || x >= NODES_W || y < 0 || y >= NODES_H) return;

    // Check start node is passable (allow starting from existing roads)
    BYTE weight = m_map->GetNodeWeight(x, y);
    if (weight == World::Weight_Deep || weight == World::Weight_Block) return;
    World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
    if (!roadsLayer) return;

    m_roadBuildState = ROAD_PLACING;
    m_roadStartX = x;
    m_roadStartY = y;
    m_roadPreviewPath.clear();
    UpdateRoadPreview(x, y);
}

void MapEditor::UpdateRoadPreview(int cursorX, int cursorY) {
    if (m_roadBuildState != ROAD_PLACING) return;
    if (!m_map || !m_roadAtlas) return;
    if (cursorX < 0 || cursorX >= NODES_W || cursorY < 0 || cursorY >= NODES_H) return;

    // If cursor is over an impassable node, find nearest passable neighbour
    int endX = cursorX, endY = cursorY;
    {
        BYTE w = m_map->GetNodeWeight(endX, endY);
        if (w == World::Weight_Deep || w == World::Weight_Block) {
            const int dx[8] = {0,1,1,1,0,-1,-1,-1};
            const int dy[8] = {-1,-1,0,1,1,1,0,-1};
            bool found = false;
            for (int d = 0; d < 8; ++d) {
                int nx = cursorX + dx[d];
                int ny = cursorY + dy[d];
                if (nx < 0 || nx >= NODES_W || ny < 0 || ny >= NODES_H) continue;
                BYTE nw = m_map->GetNodeWeight(nx, ny);
                if (nw != World::Weight_Deep && nw != World::Weight_Block) {
                    endX = nx; endY = ny; found = true; break;
                }
            }
            if (!found) return;
        }
    }

    // Also verify start is still valid
    BYTE startW = m_map->GetNodeWeight(m_roadStartX, m_roadStartY);
    if (startW == World::Weight_Deep || startW == World::Weight_Block) {
        CancelRoad();
        return;
    }

    if (m_roadStartX == endX && m_roadStartY == endY) {
        m_roadPreviewPath.clear();
        m_roadPreviewPath.push_back(std::make_pair(m_roadStartX, m_roadStartY));
        return;
    }

    struct RoadPassable {
        World::Map* map;
        RoadPassable(World::Map* m) : map(m) {}
        bool operator()(int x, int y) {
            BYTE w = map->GetNodeWeight(x, y);
            if (w == World::Weight_Deep || w == World::Weight_Block) return false;
            // Also check Placement layer: can't build road on occupied cell
            World::TileLayer* placementLayer = map->GetLayer(World::Placement);
            if (placementLayer) {
                const World::Tile& pt = placementLayer->GetTile(x, y);
                if (pt.regionIndex >= 0) return false;
            }
            return true;
        }
    };
    struct RoadCost {
        World::Map* map;
        RoadCost(World::Map* m) : map(m) {}
        float operator()(int x, int y) {
            World::TileLayer* roadsLayer = map->GetLayer(World::Roads);
            if (roadsLayer) {
                const World::Tile& t = roadsLayer->GetTile(x, y);
                if (t.regionIndex >= 0) return 0.3f;
            }
            return 1.0f;
        }
    };

    Logic::IsoNeighbors isoNeighbors;
    Logic::AStar::FindPath(
        m_roadStartX, m_roadStartY, endX, endY,
        NODES_W, NODES_H,
        RoadPassable(m_map),
        RoadCost(m_map),
        isoNeighbors,
        m_roadPreviewPath
    );

    // Ensure start is first (AStar already does this, but be safe)
    if (!m_roadPreviewPath.empty() &&
        (m_roadPreviewPath[0].first != m_roadStartX || m_roadPreviewPath[0].second != m_roadStartY)) {
        m_roadPreviewPath.insert(m_roadPreviewPath.begin(), std::make_pair(m_roadStartX, m_roadStartY));
    }
}


void MapEditor::CommitRoad() {
    if (m_roadBuildState != ROAD_PLACING) return;
    if (!m_map || !m_roadAtlas) return;

    World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
    if (!roadsLayer) return;

    World::TileLayer* placementLayer = m_map->GetLayer(World::Placement);

    // Mark all preview nodes as roads
    for (size_t i = 0; i < m_roadPreviewPath.size(); ++i) {
        int px = m_roadPreviewPath[i].first;
        int py = m_roadPreviewPath[i].second;
        if (px < 0 || px >= NODES_W || py < 0 || py >= NODES_H) continue;

        // 1. Always remove flag if it exists at this position
        for (size_t f = 0; f < m_roadFlags.size(); ++f) {
            if (m_roadFlags[f].first == px && m_roadFlags[f].second == py) {
                m_roadFlags.erase(m_roadFlags.begin() + f);
                break;
            }
        }

        World::Tile& tile = roadsLayer->GetTile(px, py);
        if (tile.regionIndex < 0) {
            // 2. If it's not a road, make it one
            tile.regionIndex = 0;
            tile.atlasName = "streets";
            tile.walkable = true;
            tile.buildable = false;

            m_map->SetNodeWeight(px, py, World::Weight_Land);

            // 3. Mark road cell in Placement layer as occupied
            if (placementLayer) {
                World::Tile& pt = placementLayer->GetTile(px, py);
                pt.regionIndex = 0;
                pt.type = World::None;
                pt.atlasName = "streets";
                pt.walkable = true;
                pt.buildable = false;
            }
        } else {
            // 4. If it's already a road, still ensure placement is updated (in case it was somehow cleared)
            if (placementLayer) {
                World::Tile& pt = placementLayer->GetTile(px, py);
                if (pt.regionIndex < 0) {
                    pt.regionIndex = 0;
                    pt.type = World::None;
                    pt.atlasName = "streets";
                    pt.walkable = true;
                    pt.buildable = false;
                }
            }
        }
    }

    // Rebuild sprites for all placed nodes and their neighbors
    for (size_t i = 0; i < m_roadPreviewPath.size(); ++i) {
        int px = m_roadPreviewPath[i].first;
        int py = m_roadPreviewPath[i].second;
        RebuildRoadSprite(px, py);
        UpdateRoadNeighbors(px, py);
    }

    char buf[128];
    sprintf_s(buf, "[MapEditor] Road committed: %d nodes\n", (int)m_roadPreviewPath.size());
    OutputDebugStringA(buf);

    CancelRoad();
}

void MapEditor::CancelRoad() {
    m_roadBuildState = ROAD_IDLE;
    m_roadStartX = -1;
    m_roadStartY = -1;
    m_roadPreviewPath.clear();
}

void MapEditor::ToggleFlag(int x, int y) {
    if (!m_map || !m_roadAtlas) return;
    if (x < 0 || x >= NODES_W || y < 0 || y >= NODES_H) return;

    World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
    if (!roadsLayer) return;

    bool isRoad = (roadsLayer->GetTile(x, y).regionIndex >= 0);
    if (!isRoad) {
        for (size_t k = 0; k < m_roadPreviewPath.size(); ++k) {
            if (m_roadPreviewPath[k].first == x && m_roadPreviewPath[k].second == y) {
                isRoad = true;
                break;
            }
        }
    }
    if (!isRoad) return;

    // If flag already exists at this position, remove it
    for (size_t i = 0; i < m_roadFlags.size(); ++i) {
        if (m_roadFlags[i].first == x && m_roadFlags[i].second == y) {
            m_roadFlags.erase(m_roadFlags.begin() + i);
            char buf[128];
            sprintf_s(buf, "[MapEditor] Flag removed at (%d,%d)\n", x, y);
            OutputDebugStringA(buf);
            return;
        }
    }

    // Place new flag
    m_roadFlags.push_back(std::make_pair(x, y));
    char buf[128];
    sprintf_s(buf, "[MapEditor] Flag placed at (%d,%d)\n", x, y);
    OutputDebugStringA(buf);
}

void MapEditor::SetRoadFlagMode(bool on) {
    if (on) {
        m_roadBuildState = ROAD_FLAG;
        m_roadStartX = -1;
        m_roadStartY = -1;
        m_roadPreviewPath.clear();
        OutputDebugStringA("[MapEditor] Flag placement mode ON\n");
    } else {
        if (m_roadBuildState == ROAD_FLAG) {
            m_roadBuildState = ROAD_IDLE;
            OutputDebugStringA("[MapEditor] Flag placement mode OFF\n");
        }
    }
}

// Helper: check if a node at (nx,ny) is a road (committed OR in previewPath)
static bool IsNodeRoad(int nx, int ny, World::TileLayer* roadsLayer, const std::vector<std::pair<int,int>>& previewPath) {
    if (nx < 0 || ny < 0) return false;
    if (roadsLayer && roadsLayer->GetTile(nx, ny).regionIndex >= 0) return true;
    for (size_t k = 0; k < previewPath.size(); ++k) {
        if (previewPath[k].first == nx && previewPath[k].second == ny) return true;
    }
    return false;
}

// Calculate pattern considering both committed roads and preview path
static int CalcPatternAt(int x, int y, World::TileLayer* roadsLayer, const std::vector<std::pair<int,int>>& previewPath) {
    int pattern = 0;
    bool evenRow = (y % 2 == 0);
    if (evenRow) {
        if (IsNodeRoad(x+1, y-1, roadsLayer, previewPath)) pattern |= 1; // NE
        if (IsNodeRoad(x+1, y+1, roadsLayer, previewPath)) pattern |= 2; // SE
        if (IsNodeRoad(x, y+1, roadsLayer, previewPath))   pattern |= 4; // SW
        if (IsNodeRoad(x, y-1, roadsLayer, previewPath))   pattern |= 8; // NW
    } else {
        if (IsNodeRoad(x, y-1, roadsLayer, previewPath))   pattern |= 1; // NE
        if (IsNodeRoad(x, y+1, roadsLayer, previewPath))   pattern |= 2; // SE
        if (IsNodeRoad(x-1, y+1, roadsLayer, previewPath)) pattern |= 4; // SW
        if (IsNodeRoad(x-1, y-1, roadsLayer, previewPath)) pattern |= 8; // NW
    }
    return pattern;
}

int MapEditor::CalcRoadPattern(int x, int y, World::TileLayer* roadsLayer) {
    std::vector<std::pair<int,int>> empty;
    return CalcPatternAt(x, y, roadsLayer, empty);
}

void MapEditor::RebuildRoadSprite(int x, int y) {
    if (!m_roadAtlas) return;
    World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
    if (!roadsLayer) return;
    if (x < 0 || x >= NODES_W || y < 0 || y >= NODES_H) return;

    World::Tile& tile = roadsLayer->GetTile(x, y);
    bool hasRoad = (tile.regionIndex >= 0);
    if (!hasRoad) return;

    int pattern = CalcRoadPattern(x, y, roadsLayer);

    // street_X = pattern value, with fallbacks for missing groups.
    char groupBuf[16];
    const char* groupName = groupBuf;
    uint32_t spriteIdx = 0;

    switch (pattern) {
        case 0:  groupName = "street_1"; break;
        case 1:  groupName = "street_1"; break;
        case 2:  groupName = "street_2"; break;
        case 3:  sprintf_s(groupBuf, "street_%d", 3); break;
        case 4:  groupName = "street_1"; break;
        case 5:  groupName = "street_1"; break;
        case 6:  sprintf_s(groupBuf, "street_%d", 6); break;
        case 7:  sprintf_s(groupBuf, "street_%d", 7); break;
        case 8:  groupName = "street_2"; break;
        case 9:  sprintf_s(groupBuf, "street_%d", 9); break;
        case 10: groupName = "street_2"; break;
        case 11: sprintf_s(groupBuf, "street_%d", 11); break;
        case 12: sprintf_s(groupBuf, "street_%d", 12); break;
        case 13: sprintf_s(groupBuf, "street_%d", 13); break;
        case 14: sprintf_s(groupBuf, "street_%d", 14); break;
        case 15: sprintf_s(groupBuf, "street_%d", 15); break;
    }

    const std::vector<uint32_t>* group = m_roadAtlas->GetGroup(groupName);
    if (!group || group->empty()) {
        group = m_roadAtlas->GetGroup("street_1");
        if (!group || group->empty()) return;
        spriteIdx = 0;
    }

    // Clamp spriteIdx to valid range
    if (spriteIdx >= group->size()) spriteIdx = 0;
    uint32_t regionIdx = (*group)[spriteIdx];

    const SpriteRegion* region = m_roadAtlas->GetRegion(regionIdx);
    if (!region) return;

    tile.u0 = region->u0;
    tile.v0 = region->v0;
    tile.u1 = region->u1;
    tile.v1 = region->v1;
    tile.regionIndex = (int)regionIdx;
    tile.atlasName = "streets";
    tile.walkable = true;
    tile.buildable = false;
}

void MapEditor::UpdateRoadNeighbors(int x, int y) {
    // Rebuild sprite for all 4 isometric staggered neighbors
    bool evenRow = (y % 2 == 0);

    // NW + NE on row above
    if (y - 1 >= 0) {
        RebuildRoadSprite(evenRow ? x : (x - 1), y - 1); // NW
        RebuildRoadSprite(evenRow ? (x + 1) : x, y - 1); // NE
    }

    // SW + SE on row below
    if (y + 1 < NODES_H) {
        RebuildRoadSprite(evenRow ? x : (x - 1), y + 1); // SW
        RebuildRoadSprite(evenRow ? (x + 1) : x, y + 1); // SE
    }
}

void MapEditor::DeleteObjectAt(int x, int y) {
    if (!m_map) return;

    // Roads layer — clear road tile and update neighbors
    if (m_currentLayer == World::Roads) {
        World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
        if (!roadsLayer) return;
        if (x < 0 || x >= roadsLayer->GetWidth() || y < 0 || y >= roadsLayer->GetHeight()) return;
        World::Tile& tile = roadsLayer->GetTile(x, y);
        if (tile.regionIndex < 0) return;
        tile = World::Tile();
        UpdateRoadNeighbors(x, y);

        // Clear Placement layer cell
        World::TileLayer* placementLayer = m_map->GetLayer(World::Placement);
        if (placementLayer) {
            World::Tile& pt = placementLayer->GetTile(x, y);
            pt = World::Tile();
        }

        // Also remove any flag at this position
        for (size_t i = 0; i < m_roadFlags.size(); ++i) {
            if (m_roadFlags[i].first == x && m_roadFlags[i].second == y) {
                m_roadFlags.erase(m_roadFlags.begin() + i);
                break;
            }
        }
        return;
    }

    World::TileLayer* objectsLayer = m_map->GetLayer(World::Objects);
    if (!objectsLayer) return;
    if (x < 0 || x >= objectsLayer->GetWidth() || y < 0 || y >= objectsLayer->GetHeight()) return;

    const World::Tile& oldTile = objectsLayer->GetTile(x, y);
    if (oldTile.regionIndex < 0) return;

    ClearPlacementFootprint(x, y, objectsLayer);
    ClearObjectInteractionZone(x, y, objectsLayer);
    World::Tile& eraseTile = objectsLayer->GetTile(x, y);
    eraseTile = World::Tile();
}

static bool TileHasSpriteData(const World::Tile& tile) {
    return tile.regionIndex >= 0 && tile.u1 > tile.u0 && tile.v1 > tile.v0;
}

static bool IsTileInRegionFootprint(int tileX, int tileY, int objectX, int objectY, const SpriteRegion* region) {
    if (!region) return false;

    CoordinateSystem& coords = CoordinateSystem::GetInstance();
    int offX, offY;
    if (region->collOffX != 0 || region->collOffY != 0) {
        offX = region->collOffX;
        offY = region->collOffY;
    } else {
        float pivotDX = region->pivotX - region->width * 0.5f;
        float pivotDY = region->pivotY - region->height * 0.5f;
        offX = -(int)(pivotDX / coords.GetNodeWidth());
        offY = -(int)(pivotDY / coords.GetNodeHeight());
    }

    if (!region->collMask.empty()) {
        for (size_t i = 0; i < region->collMask.size(); ++i) {
            int fx = objectX + offX + region->collMask[i].first;
            int fy = objectY + offY + region->collMask[i].second;
            if (tileX == fx && tileY == fy) return true;
        }
        return false;
    }

    for (uint32_t dy = 0; dy < region->collHeight; ++dy) {
        for (uint32_t dx = 0; dx < region->collWidth; ++dx) {
            int fx = objectX + offX + (int)dx;
            int fy = objectY + offY + (int)dy;
            if (tileX == fx && tileY == fy) return true;
        }
    }
    return false;
}

bool MapEditor::FindMountainObjectForResource(int x, int y, int& mountainX, int& mountainY) const {
    mountainX = x;
    mountainY = y;
    if (!m_map) return false;

    World::TileLayer* objectsLayer = m_map->GetLayer(World::Objects);
    World::TileLayer* placementLayer = m_map->GetLayer(World::Placement);
    World::TileLayer* zoneLayer = m_map->GetLayer(World::Resources);
    if (!objectsLayer) return false;

    int width = objectsLayer->GetWidth();
    int height = objectsLayer->GetHeight();
    if (x < 0 || y < 0 || x >= width || y >= height) return false;

    if (zoneLayer && x < zoneLayer->GetWidth() && y < zoneLayer->GetHeight()) {
        const World::Tile& zoneTile = zoneLayer->GetTile(x, y);
        if (zoneTile.regionIndex >= 0 && IsMountainTileType(zoneTile.type)) {
            return true;
        }
    }

    const World::Tile& objectTile = objectsLayer->GetTile(x, y);
    if (TileHasSpriteData(objectTile) && IsMountainTileType(objectTile.type)) {
        return true;
    }

    if (placementLayer && x < placementLayer->GetWidth() && y < placementLayer->GetHeight()) {
        const World::Tile& placementTile = placementLayer->GetTile(x, y);
        if (placementTile.regionIndex >= 0 && IsMountainTileType(placementTile.type)) {
            return true;
        }
    }

    TextureRegistry& reg = TextureRegistry::instance();
    for (int sy = 0; sy < height; ++sy) {
        for (int sx = 0; sx < width; ++sx) {
            const World::Tile& candidate = objectsLayer->GetTile(sx, sy);
            if (!TileHasSpriteData(candidate) || !IsMountainTileType(candidate.type)) continue;

            std::tr1::shared_ptr<SpriteAtlas> atlas = reg.getAtlas(candidate.atlasName);
            if (!atlas.get()) atlas = m_objectAtlas;
            if (!atlas.get()) continue;

            const SpriteRegion* region = atlas->GetRegion(candidate.regionIndex);
            if (IsTileInRegionFootprint(x, y, sx, sy, region)) return true;
        }
    }

    const World::Tile* zoneTile = NULL;
    const World::Tile* placementTile = NULL;
    if (zoneLayer && x < zoneLayer->GetWidth() && y < zoneLayer->GetHeight()) {
        zoneTile = &zoneLayer->GetTile(x, y);
    }
    if (placementLayer && x < placementLayer->GetWidth() && y < placementLayer->GetHeight()) {
        placementTile = &placementLayer->GetTile(x, y);
    }
    char buf[256];
    sprintf_s(buf,
        "[MapEditor] No mountain zone at (%d,%d): object=%s/%d zone=%s/%d placement=%s/%d\n",
        x, y,
        World::TileTypeToString(objectTile.type), objectTile.regionIndex,
        zoneTile ? World::TileTypeToString(zoneTile->type) : "None", zoneTile ? zoneTile->regionIndex : -1,
        placementTile ? World::TileTypeToString(placementTile->type) : "None", placementTile ? placementTile->regionIndex : -1);
    OutputDebugStringA(buf);
    return false;
}

void MapEditor::RenderActiveTile() {
    if (!m_renderer || m_currentTileIndex < 0 || !m_renderQueue) return;

    SpriteAtlas* atlas = (m_currentLayer == World::Objects) ? m_objectAtlas.get() : m_groundAtlas.get();
    LPDIRECT3DTEXTURE9 tex = (m_currentLayer == World::Objects) ? m_objectAtlas->GetTexture() : m_groundTexture;

    if (!atlas || m_currentTileIndex >= (int)atlas->GetRegionCount()) return;

    const SpriteRegion* region = atlas->GetRegion(m_currentTileIndex);
    if (!region) return;

    float screenW = static_cast<float>(m_renderer->GetScreenWidth());
    float tileSize = 64.0f;

    Graphics::RenderCommand cmd = {};
    cmd.x = screenW - tileSize - 10.0f;
    cmd.y = 10.0f;
    cmd.width = tileSize;
    cmd.height = tileSize;
    cmd.u0 = region->u0;
    cmd.v0 = region->v0;
    cmd.u1 = region->u1;
    cmd.v1 = region->v1;
    cmd.color = 0xFFFFFFFF;
    cmd.shaderID = SHADER_SPRITE;
    cmd.textureID = 0;
    cmd.blendMode = 1;
    cmd.layer = LAYER_UI;
    cmd.depth = 100;
    m_renderQueue->Submit(cmd);
}

void MapEditor::CacheNodePositions() {
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    // Cache Nodes (Layer 1)
    for (int ny = 0; ny < NODES_H; ++ny) {
        for (int nx = 0; nx < NODES_W; ++nx) {
            coords.NodeTileToWorld(nx, ny, m_nodesCache[ny][nx].worldX, m_nodesCache[ny][nx].worldY);
        }
    }

    // Cache Ground tiles (Layer 0)
    for (int gy = 0; gy < GRID_HEIGHT; ++gy) {
        for (int gx = 0; gx < GRID_WIDTH; ++gx) {
            coords.GroundTileToWorld(gx, gy, m_groundCache[gy][gx].worldX, m_groundCache[gy][gx].worldY);
        }
    }
}

static D3DCOLOR WeightToColor(BYTE weight) {
    switch (weight) {
        case World::Weight_Deep:    return D3DCOLOR_ARGB(255, 50, 100, 255);
        case World::Weight_Shallow: return D3DCOLOR_ARGB(255, 50, 255, 255);
        case World::Weight_Land:    return D3DCOLOR_ARGB(255, 100, 220, 100);
        case World::Weight_Block:   return D3DCOLOR_ARGB(255, 255, 80, 80);
        default:                    return D3DCOLOR_ARGB(255, 100, 100, 100);
    }
}

void MapEditor::RenderWeightMap() {
    if (!m_renderQueue) return;
    if (m_currentLayer != World::Nodes) return;
    if (!m_dotTexture) return;
    if (!m_map) return;

    CoordinateSystem& wCoords = CoordinateSystem::GetInstance();
    float dotW = wCoords.GetNodeWidth() * 0.25f;
    float dotH = wCoords.GetNodeHeight() * 0.25f;

    for (int ny = 0; ny < NODES_H; ++ny) {
        for (int nx = 0; nx < NODES_W; ++nx) {
            BYTE w = m_map->GetNodeWeight(nx, ny);
            const NodePos& pos = m_nodesCache[ny][nx];

            Graphics::RenderCommand cmd = {};
            cmd.x = pos.worldX - dotW * 0.5f;
            cmd.y = pos.worldY - dotH * 0.5f;
            cmd.width = dotW;
            cmd.height = dotH;
            cmd.u0 = 0.0f;
            cmd.v0 = 0.0f;
            cmd.u1 = 1.0f;
            cmd.v1 = 1.0f;
            cmd.color = WeightToColor(w);
            cmd.textureID = 2;
            cmd.shaderID = SHADER_TERRAIN;
            cmd.blendMode = 1;
            cmd.layer = LAYER_EFFECTS;
            cmd.depth = static_cast<WORD>(0.98f * 65535.0f);
            m_renderQueue->Submit(cmd);
        }
    }
}

bool MapEditor::IsPlacementFootprintFree(int tx, int ty, const SpriteRegion* region) const {
    if (!m_map || !region) return false;
    if (region->collWidth == 0 || region->collHeight == 0) return true;

    CoordinateSystem& coords = CoordinateSystem::GetInstance();
    int offX, offY;
    if (region->collOffX != 0 || region->collOffY != 0) {
        offX = region->collOffX;
        offY = region->collOffY;
    } else {
        float pivotDX = region->pivotX - region->width * 0.5f;
        float pivotDY = region->pivotY - region->height * 0.5f;
        offX = -(int)(pivotDX / coords.GetNodeWidth());
        offY = -(int)(pivotDY / coords.GetNodeHeight());
    }

    World::TileLayer* placementLayer = m_map->GetLayer(World::Placement);
    if (!placementLayer) return true;

    int pw = placementLayer->GetWidth();
    int ph = placementLayer->GetHeight();
    if (!region->collMask.empty()) {
        for (size_t i = 0; i < region->collMask.size(); ++i) {
            int fx = tx + offX + region->collMask[i].first;
            int fy = ty + offY + region->collMask[i].second;
            if (fx >= 0 && fx < pw && fy >= 0 && fy < ph) {
                const World::Tile& pt = placementLayer->GetTile(fx, fy);
                if (pt.regionIndex >= 0) return false;
            }
        }
    } else {
        for (uint32_t dy = 0; dy < region->collHeight; ++dy) {
            for (uint32_t dx = 0; dx < region->collWidth; ++dx) {
                int fx = tx + offX + (int)dx;
                int fy = ty + offY + (int)dy;
                if (fx >= 0 && fx < pw && fy >= 0 && fy < ph) {
                    const World::Tile& pt = placementLayer->GetTile(fx, fy);
                    if (pt.regionIndex >= 0) return false;
                }
            }
        }
    }
    return true;
}

bool MapEditor::CanPlaceObject(int x, int y, World::TileType objectType) {
    if (!m_map) return false;
    BYTE weight = m_map->GetNodeWeight(x, y);

    switch (objectType) {
        case World::MountainOnWater:
            // Fishing: only on shallow water / coast
            return weight == World::Weight_Shallow;
        case World::Mountain:
        case World::Tree:
        case World::Rock:
        case World::Decoration:
            // Trees, mountains, rocks only on land
            return weight == World::Weight_Land;
        case World::Building:
            return weight == World::Weight_Land;
        default:
            return true;
    }
}

void MapEditor::RenderResources() {
    if (!m_renderQueue || !m_map || !m_textManager || m_currentLayer != World::Resources) return;

    CoordinateSystem& coords = CoordinateSystem::GetInstance();
    float nodeW = coords.GetNodeWidth();

    for (int y = 0; y < NODES_H; ++y) {
        for (int x = 0; x < NODES_W; ++x) {
            const World::ResourceNode& rn = m_map->GetResourceNode(x, y);
            if (rn.type == World::ResourceType_None || !rn.isVisible) continue;

            float wx, wy;
            coords.NodeTileToWorld(x, y, wx, wy);

            char amountBuf[16];
            sprintf_s(amountBuf, "%d", rn.amount);

            float textX = wx + nodeW * 0.5f;
            float textY = wy - 24.0f;

            m_textManager->DrawTextToWorld(amountBuf, textX, textY, 0xFFFFFF00, 0.12f, FONT_MENU, FONT_STYLE_SHADOW);
        }
    }
}

void MapEditor::RebuildObjectInteractionZones() {
    if (!m_map) return;

    World::TileLayer* objectsLayer = m_map->GetLayer(World::Objects);
    World::TileLayer* zoneLayer = m_map->GetLayer(World::Resources);
    if (!objectsLayer || !zoneLayer) return;

    for (int y = 0; y < zoneLayer->GetHeight(); ++y) {
        for (int x = 0; x < zoneLayer->GetWidth(); ++x) {
            zoneLayer->GetTile(x, y) = World::Tile();
        }
    }

    int objectCount = 0;
    int mountainCount = 0;
    for (int y = 0; y < objectsLayer->GetHeight(); ++y) {
        for (int x = 0; x < objectsLayer->GetWidth(); ++x) {
            const World::Tile& tile = objectsLayer->GetTile(x, y);
            if (!TileHasSpriteData(tile)) continue;

            std::tr1::shared_ptr<SpriteAtlas> atlas = TextureRegistry::instance().getAtlas(tile.atlasName);
            if (!atlas.get()) atlas = m_objectAtlas;
            if (!atlas.get()) continue;

            const SpriteRegion* region = atlas->GetRegion(tile.regionIndex);
            MarkObjectInteractionZone(x, y, tile, region);
            objectCount++;
            if (IsMountainTileType(tile.type)) mountainCount++;
        }
    }

    char buf[160];
    sprintf_s(buf, "[MapEditor] Rebuilt object interaction zones: objects=%d mountains=%d\n",
        objectCount, mountainCount);
    OutputDebugStringA(buf);
}

void MapEditor::AutoAssignResourcesForTrees() {
    if (!m_map) return;

    World::TileLayer* objectsLayer = m_map->GetLayer(World::Objects);
    if (!objectsLayer) return;

    RebuildObjectInteractionZones();

    int assigned = 0;
    for (int y = 0; y < objectsLayer->GetHeight(); ++y) {
        for (int x = 0; x < objectsLayer->GetWidth(); ++x) {
            const World::Tile& tile = objectsLayer->GetTile(x, y);

            // Only use valid objects (has sprite data)
            if (!TileHasSpriteData(tile)) continue;

            World::ResourceNode& rn = m_map->GetResourceNode(x, y);

            // Skip resource auto-assignment if already has a resource
            if (rn.type != World::ResourceType_None) continue;

            World::ResourceType rt = World::TileTypeToResourceType(tile.type);
            if (rt != World::ResourceType_None) {
                rn.type = rt;
                rn.amount = World::GetDefaultResourceAmount(rt);
                rn.isVisible = true;
                assigned++;
            }
        }
    }

    if (assigned > 0) {
        char buf[128];
        sprintf_s(buf, "[MapEditor] Auto-assigned %d resource nodes for trees/objects\n", assigned);
        OutputDebugStringA(buf);
    }
}

World::TileType MapEditor::GetObjectTypeByIndex(int index) {
    if (m_currentObjectGroupName) {
        if (strcmp(m_currentObjectGroupName, "tree") == 0) {
            return World::Tree;
        } else if (strcmp(m_currentObjectGroupName, "mountain_water") == 0) {
            return World::Mountain;
        } else if (strcmp(m_currentObjectGroupName, "mountain") == 0) {
            return World::Mountain;
        } else if (strcmp(m_currentObjectGroupName, "rock") == 0) {
            return World::Decoration;
        } else if (strcmp(m_currentObjectGroupName, "decoration") == 0) {
            return World::Decoration;
        }
    }
    return World::Tree;
}

} // namespace Editor