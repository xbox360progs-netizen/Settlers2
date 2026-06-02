#pragma once
#include "../World/Map.h"
#include "../World/TileLayer.h"
#include "../Graphics/TileRenderer.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Logic/WeightMap.h"
#include "../Logic/AStar.h"
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

enum RoadBuildState {
    ROAD_IDLE,
    ROAD_PLACING,
    ROAD_FLAG
};

class MapEditor {
public:
    int m_gridWidth;
    int m_gridHeight;
    int m_nodesW;
    int m_nodesH;

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
    Graphics::RenderQueue* GetRenderQueue() const { return m_renderQueue; }
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
    void SetShowObjects(bool show) { m_showObjects = show; }
    void SetShowOverlay(bool show) { m_showOverlay = show; }
    void SetShowBuildings(bool show) { m_showBuildings = show; }
    void SetShowNodes(bool show) { m_showNodes = show; }
    void SetShowCursor(bool show) { m_showCursor = show; }

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

    bool SaveMap(const std::string& filename);
    bool LoadMap(const std::string& filename);

    UI::TilePalette* GetTilePalette() { return m_tilePalette; }
    World::TileType GetObjectTypeByIndex(int index);

    // Road building (A* pathfinding)
    void StartRoad(int x, int y);
    void UpdateRoadPreview(int cursorX, int cursorY);
    void CommitRoad();
    void CancelRoad();
    RoadBuildState GetRoadBuildState() const { return m_roadBuildState; }
    const std::vector<std::pair<int,int>>& GetRoadPreviewPath() const { return m_roadPreviewPath; }

    // Road flag nodes
    void ToggleFlag(int x, int y);
    bool IsRoadFlagMode() const { return m_roadBuildState == ROAD_FLAG; }
    void SetRoadFlagMode(bool on);
    const std::vector<std::pair<int,int>>& GetRoadFlags() const { return m_roadFlags; }
    void SetRoadFlags(const std::vector<std::pair<int,int>>& flags) { m_roadFlags = flags; }

private:
    static void OnTileSelected(World::TileType type, void* userData);

    void CreateUI(IDirect3DDevice9* device);
    void HandleInput();
    void UpdateCamera(float deltaTime);
    void RenderGrid();
    void RenderTilePreview();
    void CacheNodePositions();

    // Member variables
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

    const char* m_currentObjectAtlasName;
    const char* m_currentObjectGroupName;

    int m_hoveredTileX, m_hoveredTileY;
    int m_selectedTileX, m_selectedTileY;
    bool m_isDragging;

    LPDIRECT3DTEXTURE9 m_groundTexture;
    std::tr1::shared_ptr<SpriteAtlas> m_groundAtlas;
    std::tr1::shared_ptr<SpriteAtlas> m_objectAtlas;
    LPDIRECT3DTEXTURE9 m_dotTexture;
    LPDIRECT3DTEXTURE9 m_roadTexture;
    std::tr1::shared_ptr<SpriteAtlas> m_roadAtlas;
    LPDIRECT3DTEXTURE9 m_buildingsTexture;
    std::tr1::shared_ptr<SpriteAtlas> m_buildingsAtlas;

    int m_cursorTileX;
    int m_cursorTileY;
    bool m_placingTile;
    int m_previewSpriteIndex;
    int m_activeSpriteIndex;

    TextManager* m_textManager;

    bool m_showObjects;
    bool m_showOverlay;
    bool m_showBuildings;
    bool m_showNodes;
    bool m_placementOccupied;
    bool m_showResourceIcons;
    bool m_showCursor;

    int m_resourceIconIndices[World::ResourceType_Count];

    std::vector<std::vector<NodePos>> m_nodesCache;
    std::vector<std::vector<NodePos>> m_groundCache;

    RoadBuildState m_roadBuildState;
    int m_roadStartX;
    int m_roadStartY;
    std::vector<std::pair<int,int>> m_roadPreviewPath;
    std::vector<std::pair<int,int>> m_roadFlags;

    // Private methods
    void InitializeMap();
    void RenderActiveTile();
    void RenderGridLayer();
    void RenderResources();
    void RenderCursor();
    void RenderWeightMap();
    void ClearPlacementFootprint(int tx, int ty, World::TileLayer* objectsLayer);
    void ClearObjectInteractionZone(int tx, int ty, World::TileLayer* objectsLayer);
    void MarkObjectInteractionZone(int tx, int ty, const World::Tile& objectTile, const SpriteRegion* region);

    // Road sprite helpers (Rule 15)
    static int CalcRoadPattern(int x, int y, World::TileLayer* roadsLayer);
    void RebuildRoadSprite(int x, int y);
    void UpdateRoadNeighbors(int x, int y);
};

} // namespace Editor