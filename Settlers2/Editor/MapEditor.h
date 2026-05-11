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
#include <vector>

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
    void Render();

    void SetEditMode(EditMode mode) { m_currentMode = mode; }
    void SetBrushSize(BrushSize size) { m_brushSize = size; }
    void SetCurrentTileType(World::TileType type) { m_currentTileType = type; }
    void SetTileByIndex(int index);
    void SetLayer(World::LayerType layer) { m_currentLayer = layer; m_placingTile = false; }
    void SetSpriteRenderer(SpriteRenderer* sr) { m_spriteRenderer = sr; }
    void SetObjectAtlas(const char* name);
    World::LayerType GetLayer() const { return m_currentLayer; }

    World::TileType GetCurrentTileType() const { return m_currentTileType; }
    int GetCurrentTileIndex() const { return m_currentTileIndex; }
    EditMode GetCurrentMode() const { return m_currentMode; }

    World::Map* GetMap() { return m_map; }
    const World::Map* GetMap() const { return m_map; }

    void PaintArea(int centerX, int centerY);
    void PaintTile(int x, int y);
    void PaintCurrentTile();
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

    UI::TilePalette* m_tilePalette;
    UI::Panel* m_toolbarPanel;

    float m_cameraX, m_cameraY;
    float m_zoomLevel;

    public:
    static const int GRID_WIDTH = 20;
    static const int GRID_HEIGHT = 20;
    static const int NODES_W = 40;
    static const int NODES_H = 40;

private:
	const char* m_currentObjectAtlasName;

    int m_hoveredTileX, m_hoveredTileY;
    int m_selectedTileX, m_selectedTileY;
    bool m_isDragging;

    LPDIRECT3DTEXTURE9 m_groundTexture;
    std::tr1::shared_ptr<SpriteAtlas> m_groundAtlas;
    std::tr1::shared_ptr<SpriteAtlas> m_objectAtlas;
    LPDIRECT3DTEXTURE9 m_cursorTexture;
    LPDIRECT3DTEXTURE9 m_isoCursorTexture;
    LPDIRECT3DTEXTURE9 m_dotTexture;

    int m_cursorTileX;
    int m_cursorTileY;
    bool m_placingTile;

	bool m_showObjects;
    bool m_showOverlay;
    bool m_showNodes;

    NodePos m_nodesCache[NODES_W][NODES_H];
    NodePos m_groundCache[GRID_WIDTH][GRID_HEIGHT];

    void InitializeMap();
    void RenderActiveTile();
    void RenderGridLayer();
    void RenderCursor();
    void RenderWeightMap();
};

} // namespace Editor
