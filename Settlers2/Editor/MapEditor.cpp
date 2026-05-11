#include "stdafx.h"
#include "MapEditor.h"
#include "../Logic/WeightMap.h"
#include "../Logic/CoordinateSystem.h"
#include "../Graphics/Renderer.h"
#include <d3dx9.h>
#include "../Graphics/Texture.h"
#include "../Input/Gamepad.h"
#include "../World/MapSerializer.h"
#include "../Graphics/TextureRegistry.h"
#include <algorithm>
#include <cmath>
#include <map>

namespace Editor {

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
    , m_cursorTexture(0)
    , m_isoCursorTexture(0)  
    , m_dotTexture(0)
    , m_cursorTileX(0)
    , m_cursorTileY(0)
    , m_placingTile(false)
	, m_showObjects(true)
    , m_showOverlay(true)
    , m_showNodes(true)
	, m_currentObjectAtlasName("icon_tree")
{
}

MapEditor::~MapEditor() {
    delete m_tilePalette;
    delete m_toolbarPanel;
    delete m_tileRenderer;
}

void MapEditor::Initialize(World::Map* map, Renderer* renderer, Input::InputManager* inputManager, IDirect3DDevice9* device) {
    m_map = map;
    m_renderer = renderer;
    m_inputManager = inputManager;
    m_pDevice = device;

    CoordinateSystem::GetInstance().Initialize(MAP_WIDTH, MAP_HEIGHT);
    CacheNodePositions();

    TextureRegistry& registry = TextureRegistry::instance();
    m_groundTexture = registry.getTextureOrLoad("ground");
    m_groundAtlas = registry.getAtlas("ground");
    m_objectAtlas = registry.getAtlas("icon_tree");
    m_cursorTexture = registry.getTextureOrLoad("background_cursor_red");
    m_isoCursorTexture = registry.getTextureOrLoad("cursor");

    // Create a simple white dot texture for weight map
    if (device) {
        D3DXCreateTexture(device, 1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_dotTexture);
        if (m_dotTexture) {
            D3DLOCKED_RECT lockRect;
            m_dotTexture->LockRect(0, &lockRect, NULL, 0);
            *(DWORD*)lockRect.pBits = D3DCOLOR_ARGB(255, 255, 255, 255);
            m_dotTexture->UnlockRect(0);
        }
    }

    // Initialize weight map as deep water (default for water tile)
    m_weightMap = new Logic::WeightMap(GRID_WIDTH, GRID_HEIGHT);
    for (int y = 0; y < GRID_HEIGHT; ++y) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            m_weightMap->SetWeight(x, y, Logic::WEIGHT_DEEP_WATER);
        }
    }

    if (renderer && map) {
        m_tileRenderer = new TileRenderer(renderer, map->GetWidth(), map->GetHeight());
        m_tileRenderer->SetMap(map);
    }

    CreateUI(device);

    InitializeMap();

    // Center camera on the iso diamond center
    if (m_map) {
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        float cx, cy;
        coords.GetDiamondCenter(cx, cy);
        m_cameraX = cx;
        m_cameraY = cy;
    }
}

void MapEditor::Update(float deltaTime) {
    HandleInput();
    UpdateCamera(deltaTime);

    if (m_tilePalette) {
        m_tilePalette->Update(deltaTime);
    }
}

void MapEditor::Render() {

    m_spriteRenderer->Flush();
    if (m_pDevice) {
        m_pDevice->SetVertexShader(NULL);
        m_pDevice->SetPixelShader(NULL);
        m_pDevice->SetTexture(0, NULL);
    }

    RenderGridLayer();

    if (m_showNodes) {
        RenderWeightMap();
    }

    if (m_showObjects && m_currentLayer == World::Objects) {
    }

    if (m_showOverlay && m_currentLayer == World::Overlay) {
    }

    m_spriteRenderer->Flush();
    if (m_pDevice) {
        m_pDevice->SetVertexShader(NULL);
        m_pDevice->SetPixelShader(NULL);
        m_pDevice->SetTexture(0, NULL);
    }

    RenderCursor();
    RenderTilePreview();
    RenderActiveTile();
}
void MapEditor::HandleInput() {
    if (!m_inputManager) return;

    Input::Gamepad* gamepad = m_inputManager->GetGamepad();
    if (!gamepad) return;

    float moveSpeed = 1500.0f;

    float leftX, leftY;
    gamepad->GetLeftStick(leftX, leftY);
    if (fabsf(leftX) > 0.1f || fabsf(leftY) > 0.1f) {
        m_cameraX += leftX * moveSpeed * 0.016f;
        m_cameraY += leftY * moveSpeed * 0.016f;
    }

    float rightX, rightY;
    gamepad->GetRightStick(rightX, rightY);
    if (fabsf(rightY) > 0.1f) {
        float zoomDelta = rightY * 2.0f * 0.016f;
        m_zoomLevel += zoomDelta;
        if (m_zoomLevel < 0.25f) m_zoomLevel = 0.25f;
        if (m_zoomLevel > 4.0f) m_zoomLevel = 4.0f;
    }

// Update cursor tile from screen center (camera position)
    const SpriteRegion* firstRegion = m_groundAtlas ? m_groundAtlas->GetRegion(0) : nullptr;
    if (firstRegion) {
        CoordinateSystem& coords = CoordinateSystem::GetInstance();

        if (m_currentLayer == World::Ground) {
            // Ground layer - use simple rectangular grid
            int newTileX, newTileY;
            coords.WorldToGroundTile(m_cameraX, m_cameraY, newTileX, newTileY);

            // Clamp to grid bounds
            if (newTileX >= 0 && newTileX < GRID_WIDTH && newTileY >= 0 && newTileY < GRID_HEIGHT) {
                m_cursorTileX = newTileX;
                m_cursorTileY = newTileY;
            }
        } else {
            // Nodes layer - use dense staggered grid
            int newTileX, newTileY;
            coords.WorldToNodeTile(m_cameraX, m_cameraY, newTileX, newTileY);

            // Clamp to nodes grid bounds
            int nodesW = coords.GetNodesWidth();
            int nodesH = coords.GetNodesHeight();
            if (newTileX >= 0 && newTileX < nodesW && newTileY >= 0 && newTileY < nodesH) {
                m_cursorTileX = newTileX;
                m_cursorTileY = newTileY;
            }
        }
    }

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
    return MapSerializer::Save(*m_map, filename);
}

bool MapEditor::LoadMap(const std::string& filename) {
    if (!m_map) return false;
    return MapSerializer::Load(*m_map, filename);
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
    if (!m_spriteRenderer || !m_map || !m_groundAtlas) return;

    World::TileLayer* groundLayer = m_map->GetLayer(World::Ground);
    if (!groundLayer) return;

    float camX = m_cameraX;
    float camY = m_cameraY;
    float zoom = m_zoomLevel;
    float scX = m_renderer->GetScreenWidth() * 0.5f;
    float scY = m_renderer->GetScreenHeight() * 0.5f;

    // === FRUSTUM CULLING: Calculate visible tile range based on camera ===
    // Screen bounds in world coordinates
    float screenW = m_renderer->GetScreenWidth();
    float screenH = m_renderer->GetScreenHeight();
    float halfW = screenW * 0.5f;
    float halfH = screenH * 0.5f;
    
    // Convert screen bounds to world bounds
    float minWorldX = camX - halfW / zoom;
    float maxWorldX = camX + halfW / zoom;
    float minWorldY = camY - halfH / zoom;
    float maxWorldY = camY + halfH / zoom;
    
    // Convert world bounds to tile bounds with padding for safety
    // For Ground tiles (20x20 grid)
    int minGroundX = 0, maxGroundX = groundLayer->GetWidth();
    int minGroundY = 0, maxGroundY = groundLayer->GetHeight();
    
    // Simple culling: iterate all for now, but can be optimized further
    // For Xbox 360, we'll limit to visible tiles when map gets larger

    // Ground layer - Y-sorting depth: 0.9-1.0 (farthest)
    {
        float tw = 238.0f * zoom;
        float th = 148.0f * zoom;
        int mapHeight = groundLayer->GetHeight();

        // Use SHADER_SPRITE_CONSTANT_INSTANCED with Y-depth sorting (ID 1 is valid, ID 0 is missing)
        m_spriteRenderer->BeginWorldObject(SHADER_SPRITE_CONSTANT_INSTANCED, m_groundAtlas->GetTexture(), 0.0f, 0.95f, 0.0001f, 1);
        
        for (int y = minGroundY; y < maxGroundY; ++y) {
            for (int x = minGroundX; x < maxGroundX; ++x) {
                float wx, wy;
                CoordinateSystem::GetInstance().GroundTileToWorld(x, y, wx, wy);
                
                // Simple frustum check: skip if outside screen bounds
                float sx = scX + (wx - camX) * zoom;
                float sy = scY + (wy - camY) * zoom;
                
                // Skip if tile is completely off-screen
                if (sx + tw < 0 || sx - tw > screenW || sy + th < 0 || sy - th > screenH) {
                    continue;
                }
                
                const World::Tile& tile = groundLayer->GetTile(x, y);
                m_spriteRenderer->Draw(sx - tw * 0.5f, sy - th * 0.5f, tw, th, tile.u0, tile.v0, tile.u1, tile.v1, 0xFFFFFFFF);
            }
        }
        m_spriteRenderer->End();
    }

    // Overlay layer - Y-sorting depth: 0.5-0.8 (mid-range)
    {
        World::TileLayer* overlayLayer = m_map->GetLayer(World::Overlay);
        if (overlayLayer && m_groundAtlas) {
            float ntw = 119.0f * zoom;
            float nth = 72.0f * zoom;
            int mapHeight = overlayLayer->GetHeight();

            // Use SHADER_SPRITE_CONSTANT_INSTANCED with Y-depth sorting
            m_spriteRenderer->BeginWorldObject(SHADER_SPRITE_CONSTANT_INSTANCED, m_groundAtlas->GetTexture(), 0.0f, 0.65f, 0.0001f, 1);
            
            for (int y = 0; y < overlayLayer->GetHeight(); ++y) {
                for (int x = 0; x < overlayLayer->GetWidth(); ++x) {
                    const World::Tile& tile = overlayLayer->GetTile(x, y);
                    if (tile.u1 <= tile.u0 || tile.v1 <= tile.v0) continue;

                    float wx, wy;
                    CoordinateSystem::GetInstance().NodeTileToWorld(x, y, wx, wy);
                    float sx = scX + (wx - camX) * zoom;
                    float sy = scY + (wy - camY) * zoom;
                    
                    // Frustum culling: skip if off-screen
                    if (sx + ntw < 0 || sx - ntw > screenW || sy + nth < 0 || sy - nth > screenH) {
                        continue;
                    }
                    
                    m_spriteRenderer->Draw(sx - ntw * 0.5f, sy - nth * 0.5f, ntw, nth, tile.u0, tile.v0, tile.u1, tile.v1, 0x80FFFFFF);
                }
            }
            m_spriteRenderer->End();
        }
    }

    // Placement layer - Y-sorting depth: 0.5-0.8
    if (m_currentLayer == World::Placement) {
        World::TileLayer* placementLayer = m_map->GetLayer(World::Placement);
        if (placementLayer) {
            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            int mapHeight = NODES_H;

            // Use SHADER_SPRITE_CONSTANT_INSTANCED with Y-depth sorting
            m_spriteRenderer->BeginWorldObject(SHADER_SPRITE_CONSTANT_INSTANCED, m_dotTexture, 0.0f, 0.65f, 0.0001f, 1);
            
            for (int y = 0; y < NODES_H; ++y) {
                for (int x = 0; x < NODES_W; ++x) {
                    float wx, wy;
                    coords.NodeTileToWorld(x, y, wx, wy);
                    float sx = scX + (wx - camX) * zoom;
                    float sy = scY + (wy - camY) * zoom;
                    
                    // Frustum culling: skip if off-screen
                    if (sx + 8.0f < 0 || sx - 8.0f > screenW || sy + 8.0f < 0 || sy - 8.0f > screenH) {
                        continue;
                    }
                    
                    const World::Tile& tile = placementLayer->GetTile(x, y);

                    if (tile.regionIndex >= 0) {
                        m_spriteRenderer->Draw(sx - 4.0f, sy - 4.0f, 8.0f, 8.0f, 0.0f, 0.0f, 1.0f, 1.0f, D3DCOLOR_ARGB(255, 0, 255, 0));
                    } else {
                        m_spriteRenderer->Draw(sx - 4.0f, sy - 4.0f, 8.0f, 8.0f, 0.0f, 0.0f, 1.0f, 1.0f, D3DCOLOR_ARGB(255, 100, 100, 100));
                    }
                }
            }
            m_spriteRenderer->End();
        }
    }

    // Resources layer - Y-sorting depth: 0.5-0.8
    if (m_currentLayer == World::Resources) {
        World::TileLayer* resourcesLayer = m_map->GetLayer(World::Resources);
        if (resourcesLayer && m_objectAtlas) {
            float ntw = 119.0f * zoom;
            float nth = 72.0f * zoom;
            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            int mapHeight = GRID_HEIGHT;

            // Use SHADER_SPRITE_CONSTANT_INSTANCED with Y-depth sorting
            m_spriteRenderer->BeginWorldObject(SHADER_SPRITE_CONSTANT_INSTANCED, m_objectAtlas->GetTexture(), 0.0f, 0.65f, 0.0001f, 1);
            
            for (int y = 0; y < GRID_HEIGHT; ++y) {
                for (int x = 0; x < GRID_WIDTH; ++x) {
                    const World::Tile& tile = resourcesLayer->GetTile(x, y);
                    if (tile.u1 <= tile.u0 || tile.v1 <= tile.v0) continue;

                    float wx, wy;
                    coords.NodeTileToWorld(x, y, wx, wy);
                    float sx = scX + (wx - camX) * zoom;
                    float sy = scY + (wy - camY) * zoom;
                    
                    // Frustum culling: skip if off-screen
                    if (sx + ntw < 0 || sx - ntw > screenW || sy + nth < 0 || sy - nth > screenH) {
                        continue;
                    }
                    
                    m_spriteRenderer->Draw(sx - ntw * 0.5f, sy - nth * 0.5f, ntw, nth, tile.u0, tile.v0, tile.u1, tile.v1, 0xFFFFFFFF);
                }
            }
            m_spriteRenderer->End();
        }
    }

    // Roads layer - Y-sorting depth: 0.5-0.8
    if (m_currentLayer == World::Roads) {
        World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
        if (roadsLayer && m_objectAtlas) {
            float ntw = 119.0f * zoom;
            float nth = 72.0f * zoom;
            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            int mapHeight = GRID_HEIGHT;

            // Use SHADER_SPRITE_CONSTANT_INSTANCED with Y-depth sorting
            m_spriteRenderer->BeginWorldObject(SHADER_SPRITE_CONSTANT_INSTANCED, m_objectAtlas->GetTexture(), 0.0f, 0.65f, 0.0001f, 1);
            
            for (int y = 0; y < GRID_HEIGHT; ++y) {
                for (int x = 0; x < GRID_WIDTH; ++x) {
                    const World::Tile& tile = roadsLayer->GetTile(x, y);
                    if (tile.u1 <= tile.u0 || tile.v1 <= tile.v0) continue;

                    float wx, wy;
                    coords.NodeTileToWorld(x, y, wx, wy);
                    float sx = scX + (wx - camX) * zoom;
                    float sy = scY + (wy - camY) * zoom;
                    
                    // Frustum culling: skip if off-screen
                    if (sx + ntw < 0 || sx - ntw > screenW || sy + nth < 0 || sy - nth > screenH) {
                        continue;
                    }
                    
                    m_spriteRenderer->Draw(sx - ntw * 0.5f, sy - nth * 0.5f, ntw, nth, tile.u0, tile.v0, tile.u1, tile.v1, 0xFFFFFFFF);
                }
            }
            m_spriteRenderer->End();
        }
    }
}

void MapEditor::RenderCursor() {
    if (!m_renderer || !m_groundAtlas) return;

    LPDIRECT3DTEXTURE9 cursorTex = (m_currentLayer == World::Ground) ? m_cursorTexture : m_isoCursorTexture;
    if (!cursorTex) return;

    const SpriteRegion* firstRegion = m_groundAtlas->GetRegion(0);
    if (!firstRegion) return;

    float tileW = static_cast<float>(firstRegion->width);
    float tileH = static_cast<float>(firstRegion->height);

    float screenCenterX = m_renderer->GetScreenWidth() * 0.5f;
    float screenCenterY = m_renderer->GetScreenHeight() * 0.5f;

    float cursorWidth = tileW * m_zoomLevel;
    float cursorHeight = tileH * m_zoomLevel;

    float cursorScreenX, cursorScreenY;

    if (m_currentLayer == World::Ground) {
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        float wx, wy;
        coords.GroundTileToWorld(m_cursorTileX, m_cursorTileY, wx, wy);
        cursorScreenX = screenCenterX + (wx - m_cameraX) * m_zoomLevel;
        cursorScreenY = screenCenterY + (wy - m_cameraY) * m_zoomLevel;
    } else {
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        float wx, wy;
        coords.NodeTileToWorld(m_cursorTileX, m_cursorTileY, wx, wy);
        cursorScreenX = screenCenterX + (wx - m_cameraX) * m_zoomLevel;
        cursorScreenY = screenCenterY + (wy - m_cameraY) * m_zoomLevel;
    }

    Texture tex;
    tex.SetTexture(cursorTex);
    m_renderer->DrawSingleSprite(&tex, cursorScreenX - cursorWidth * 0.5f, cursorScreenY - cursorHeight * 0.5f, cursorWidth, cursorHeight);
}

void MapEditor::SetTileByIndex(int index) {
    m_currentTileIndex = index;
    m_placingTile = true;
}

void MapEditor::SetObjectAtlas(const char* name) {
    if (name) {
        m_currentObjectAtlasName = name; 
        TextureRegistry& registry = TextureRegistry::instance();
        m_objectAtlas = registry.getAtlas(name);
    }
}

void MapEditor::PaintCurrentTile() {
    if (!m_map || m_currentTileIndex < 0) return;

    World::TileLayer* layer = m_map->GetLayer(m_currentLayer);
    if (!layer) return;

    int tileX = m_cursorTileX;
    int tileY = m_cursorTileY;

    if (tileX < 0 || tileX >= layer->GetWidth() || tileY < 0 || tileY >= layer->GetHeight()) return;

    World::Tile& tile = layer->GetTile(tileX, tileY);

    if (m_currentLayer == World::Objects) {
        if (!m_objectAtlas) return;
        if (m_currentTileIndex >= (int)m_objectAtlas->GetRegionCount()) return;
        const SpriteRegion* region = m_objectAtlas->GetRegion(m_currentTileIndex);
        if (!region) return;
        tile.u0 = region->u0;
        tile.v0 = region->v0;
        tile.u1 = region->u1;
        tile.v1 = region->v1;
        tile.regionIndex = m_currentTileIndex;
        tile.type = GetObjectTypeByIndex(m_currentTileIndex);
        tile.atlasName = m_currentObjectAtlasName;
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
    }
}

void MapEditor::RenderActiveTile() {
    if (!m_renderer || m_currentTileIndex < 0) return;

    SpriteAtlas* atlas = (m_currentLayer == World::Objects) ? m_objectAtlas.get() : m_groundAtlas.get();
    LPDIRECT3DTEXTURE9 tex = (m_currentLayer == World::Objects) ? m_objectAtlas->GetTexture() : m_groundTexture;

    if (!atlas || m_currentTileIndex >= (int)atlas->GetRegionCount()) return;

    const SpriteRegion* region = atlas->GetRegion(m_currentTileIndex);
    if (!region) return;

    float screenW = static_cast<float>(m_renderer->GetScreenWidth());
    float tileSize = 64.0f;

    Texture texObj;
    texObj.SetTexture(tex);
    m_renderer->DrawSingleSprite(&texObj, screenW - tileSize - 10.0f, 10.0f, tileSize, tileSize,
                                 region->u0, region->v0, region->u1, region->v1);
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

void MapEditor::RenderWeightMap() {
    if (!m_spriteRenderer || !m_weightMap) return;
    if (m_currentLayer != World::Nodes) return;
    if (!m_dotTexture) return;

    CoordinateSystem& coords = CoordinateSystem::GetInstance();
    float scX = static_cast<float>(m_renderer->GetScreenWidth()) * 0.5f;
    float scY = static_cast<float>(m_renderer->GetScreenHeight()) * 0.5f;
    float ds = 16.0f * m_zoomLevel;
    float hds = ds * 0.5f;
    float z = m_zoomLevel;

    // UNIFIED SHADER: Use sprite_constant_instanced
    m_spriteRenderer->Begin("sprite_constant_instanced", m_dotTexture);

    for (int ny = 0; ny < NODES_H; ++ny) {
        for (int nx = 0; nx < NODES_W; ++nx) {
            float wx, wy;
            coords.NodeTileToWorld(nx, ny, wx, wy);
            float sx = scX + (wx - m_cameraX) * z;
            float sy = scY + (wy - m_cameraY) * z;
            m_spriteRenderer->Draw(sx - hds, sy - hds, ds, ds, 0.0f, 0.0f, 1.0f, 1.0f, D3DCOLOR_ARGB(255, 100, 100, 255));
        }
    }

    m_spriteRenderer->End();
}

bool MapEditor::CanPlaceObject(int x, int y, World::TileType objectType) {
    Logic::TileWeight weight = m_weightMap->GetWeight(x, y);

    switch (objectType) {
        case World::MountainOnWater:
            return (weight == Logic::WEIGHT_DEEP_WATER || weight == Logic::WEIGHT_SHALLOW_WATER);
        case World::Mountain:
        case World::Tree:
        case World::Rock:
        case World::Building:
            return (weight == Logic::WEIGHT_LAND || weight == Logic::WEIGHT_COAST);
        default:
            return true;
    }
}

World::TileType MapEditor::GetObjectTypeByIndex(int index) {
    // ���������� ��� �� ����� ������
    if (strcmp(m_currentObjectAtlasName, "icon_tree") == 0) {
        return World::Tree;
    } else if (strcmp(m_currentObjectAtlasName, "icon_mountain") == 0) {
        return World::Mountain;
    } else if (strcmp(m_currentObjectAtlasName, "icon_building") == 0) {
        return World::Building;
    } else if (strcmp(m_currentObjectAtlasName, "icon_rock") == 0) {
        return World::Rock;
    } else if (strcmp(m_currentObjectAtlasName, "icon_decoration") == 0) {
        return World::Decoration;
    } else if (strcmp(m_currentObjectAtlasName, "icon_water_mountain") == 0) {
        return World::MountainOnWater;
    }
    return World::Tree; // �� ���������
}


} // namespace Editor
