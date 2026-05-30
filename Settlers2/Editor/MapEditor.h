#pragma once
#include "../World/Map.h"
#include "../World/TileLayer.h"
#include "../Graphics/TileRenderer.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Logic/WeightMap.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/TextureRegistry.h"
#include "../Graphics/SpriteAtlas.h"
#include "../Input/InputManager.h"
#include "../UI/TilePalette.h"
#include "../UI/Panel.h"
#include "../Graphics/TextManager.h"
#include <vector>
#include "../Graphics/Camera.h"

using Graphics::SpriteRenderer;

class Renderer;

namespace Editor {

struct NodePos {
    float worldX, worldY;
};

enum EditMode {
    EditMode_Select,
    EditMode_PaintGround,
    EditMode_PaintObjects,
    EditMode_PlaceBuilding,
    EditMode_Erase
};

enum BrushSize {
    BrushSize_Single = 1,
    BrushSize_Small = 3,
    BrushSize_Medium = 5,
    BrushSize_Large = 7
};

class MapEditor {
public:
    MapEditor();
    ~MapEditor();

    void Initialize(World::Map* map, ::Renderer* renderer, Input::InputManager* inputManager, IDirect3DDevice9* device);
    void Update(float deltaTime);
    void RenderGeometry();
    void RenderUI();

    void SetEditMode(EditMode mode) { m_currentMode = mode; }
    void SetBrushSize(BrushSize size) { m_brushSize = size; }
    void SetCurrentTileType(World::TileType type) { m_currentTileType = type; }
    void SetTileByIndex(int index);
    void SetLayer(World::LayerType layer);
    void SetSpriteRenderer(SpriteRenderer* sr) { m_spriteRenderer = sr; }
    void SetCamera(Camera* pCamera) { m_pCamera = pCamera; }
    void SetRenderQueue(Graphics::RenderQueue* rq) { m_renderQueue = rq; if (m_textManager) m_textManager->SetRenderQueue(rq); }
    void SetTextManager(TextManager* tm) { m_textManager = tm; }
    void SetObjectGroup(const char* groupName);
    void SetPlacementOccupied(bool occupied) { m_placementOccupied = occupied; }
    bool IsPlacementOccupied() const { return m_placementOccupied; }
    World::LayerType GetLayer() const { return m_currentLayer; }
    void SetCursorWorldPosition(float x, float y);
    void SetCursorPreview(int index) { m_previewSpriteIndex = index; }

    void AutoAssignResourcesForTrees();
    void RebuildObjectInteractionZones();
    void SetShowResourceIcons(bool show) { m_showResourceIcons = show; }

    World::TileType GetCurrentTileType() const { return m_currentTileType; }
    int GetCurrentTileIndex() const { return m_currentTileIndex; }
    EditMode GetCurrentMode() const { return m_currentMode; }

    World::Map* GetMap() { return m_map; }
    const World::Map* GetMap() const { return m_map; }
    int GetCursorTileX() const { return m_cursorTileX; }
    int GetCursorTileY() const { return m_cursorTileY; }

    void PaintArea(int centerX, int centerY);
    void PaintTile(int x, int y);
    void PaintCurrentTile();
    void DeleteObjectAt(int x, int y);
    bool IsPlacementFootprintFree(int tx, int ty, const SpriteRegion* region) const;
    bool FindMountainObjectForResource(int x, int y, int& mountainX, int& mountainY) const;
	bool CanPlaceObject(int x, int y, World::TileType objectType);

	void MapEditor::SetShowObjects(bool show) { m_showObjects = show; }
	void MapEditor::SetShowOverlay(bool show) { m_showOverlay = show; }
	void MapEditor::SetShowNodes(bool show) { m_showNodes = show; }

    bool SaveMap(const std::string& filename);
    bool LoadMap(const std::string& filename);

    UI::TilePalette* GetTilePalette() { return m_tilePalette; }
	World::TileType GetObjectTypeByIndex(int index);
private:
    static void OnTileSelected(World::TileType type, void* userData);

    void CreateUI(IDirect3DDevice9* device);
    void HandleInput();
    void UpdateCamera(float deltaTime);
    void RenderGrid();
    void RenderTilePreview();
    void CacheNodePositions();	

World::Map* m_map;
    TileRenderer* m_tileRenderer;
    ::Renderer* m_renderer;
    Input::InputManager* m_inputManager;
    LPDIRECT3DDEVICE9 m_pDevice;

    EditMode m_currentMode;
    BrushSize m_brushSize;
    World::TileType m_currentTileType;
    int m_currentTileIndex;
    World::LayerType m_currentLayer;
    Logic::WeightMap* m_weightMap;
    SpriteRenderer* m_spriteRenderer;
    Camera* m_pCamera;
    Graphics::RenderQueue* m_renderQueue;

    UI::TilePalette* m_tilePalette;
    UI::Panel* m_toolbarPanel;

    float m_cameraX, m_cameraY;
    float m_zoomLevel;
    float m_worldCenterX, m_worldCenterY;

    public:
    static const int GRID_WIDTH = 20;
    static const int GRID_HEIGHT = 20;
    static const int NODES_W = 40;
    static const int NODES_H = 80;

private:
	const char* m_currentObjectAtlasName;
    const char* m_currentObjectGroupName;

    int m_hoveredTileX, m_hoveredTileY;
    int m_selectedTileX, m_selectedTileY;
    bool m_isDragging;

    LPDIRECT3DTEXTURE9 m_groundTexture;
    std::tr1::shared_ptr<SpriteAtlas> m_groundAtlas;
    std::tr1::shared_ptr<SpriteAtlas> m_objectAtlas;
    LPDIRECT3DTEXTURE9 m_dotTexture;

    int m_cursorTileX;
    int m_cursorTileY;
    bool m_placingTile;
    int m_previewSpriteIndex;
    int m_activeSpriteIndex; // New: Stores the persistent active sprite index

    TextManager* m_textManager;

	bool m_showObjects;
    bool m_showOverlay;
    bool m_showNodes;

    NodePos m_nodesCache[NODES_H][NODES_W];
    NodePos m_groundCache[GRID_HEIGHT][GRID_WIDTH];

    void InitializeMap();
    void RenderActiveTile();
    bool m_placementOccupied;
    bool m_showResourceIcons;
    int m_resourceIconIndices[World::ResourceType_Count]; // Cached sprite indices for resource icons (indexed by ResourceType)
    void RenderGridLayer();
    void RenderResources();
    void RenderCursor();
    void RenderWeightMap();
    void ClearPlacementFootprint(int tx, int ty, World::TileLayer* objectsLayer);
    void ClearObjectInteractionZone(int tx, int ty, World::TileLayer* objectsLayer);
    void MarkObjectInteractionZone(int tx, int ty, const World::Tile& objectTile, const SpriteRegion* region);
};

} // namespace Editor
