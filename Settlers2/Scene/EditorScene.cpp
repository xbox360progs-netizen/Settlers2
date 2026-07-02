#include "stdafx.h"
#include "EditorScene.h"
#include <string>
#include "../Input/InputController.h"
#include "../Logic/CoordinateSystem.h"
#include "../Logic/MapConstants.h"
#include "../World/TileLayer.h"
#include "../UI/RadialMenu.h"
#include "../Graphics/TextManager.h"
#include "../UI/GridMenu.h"
#include "../Graphics/Texture.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/BinFileManager.h"
#include "../Input/InputManager.h"
#include "../Editor/MapEditor.h"
#include "../Graphics/SpriteAtlas.h"
#include "../Graphics/TextureRegistry.h"
#include "../Graphics/RenderLayers.h"
#include "../Graphics/RenderCommandBuilder.h"
#include "../Core/LanguageManager.h"
#include <iostream>
#include <cstdio>

namespace Scene {
 
    static World::BuildingType GetBuildingType(const std::string& name) {
        std::string lowerName = name;
        for (size_t ci = 0; ci < lowerName.size(); ++ci)
            if (lowerName[ci] >= 'A' && lowerName[ci] <= 'Z')
                lowerName[ci] = lowerName[ci] - 'A' + 'a';

        if (lowerName == "hut") return World::Hut;
        if (lowerName == "tower") return World::Tower;
        if (lowerName == "fortress") return World::Fortress;
        if (lowerName == "castle") return World::Castle;
        if (lowerName == "forester") return World::Forester;
        if (lowerName == "woodcutter") return World::Woodcutter;
        if (lowerName == "sawmill") return World::Sawmill;
        if (lowerName == "stonemason") return World::Stonemason;
        if (lowerName == "coalmine") return World::CoalMine;
        if (lowerName == "ironmine") return World::IronMine;
        if (lowerName == "goldmine") return World::GoldMine;
        if (lowerName == "ironsmelter") return World::IronSmelter;
        if (lowerName == "goldsmelter") return World::GoldSmelter;
        if (lowerName == "farm") return World::Farm;
        if (lowerName == "mill") return World::Mill;
        if (lowerName == "bakery") return World::Bakery;
        if (lowerName == "fisher") return World::Fisher;
        if (lowerName == "hunter") return World::Hunter;
        if (lowerName == "baker") return World::Baker;
        if (lowerName == "brewer") return World::Brewer;
        if (lowerName == "toolworkshop") return World::ToolWorkshop;
        if (lowerName == "storehouse" || lowerName == "warehouse") return World::Storehouse;
        if (lowerName == "residence") return World::Residence;
        if (lowerName == "stronghold") return World::Stronghold;
        if (lowerName == "well") return World::Well;
        if (lowerName == "bronzemine") return World::BronzeMine;
        if (lowerName == "bronzesmelter") return World::BronzeSmelter;
        if (lowerName == "toolmaker") return World::ToolWorkshop;
        if (lowerName == "barracks") return World::Barracks;

        return World::Building_None;
    }
 
    const char* EditorScene::kObjectGroupNames[] = {

    "tree",
    "mountain_water",
    "mountain",
    "rock",
    "decoration"
};
const int EditorScene::kObjectGroupCount = 5;

const char* EditorScene::kResourceGroupName = "ResourceIcons";
const int EditorScene::kResourceTypeCount = World::ResourceType_Count - 1;
const int EditorScene::kResourceMenuGroupCount = 4;

struct ResourceMenuGroupDef {
    const char* iconName;
    World::ResourceType resources[8];
    const char* depositIconNames[8];
    int count;
};

int EditorScene::s_mapGridWidth = 20;
int EditorScene::s_mapGridHeight = 20;

static const ResourceMenuGroupDef kResourceMenuGroups[] = {
    { "icon_resource_wood",
      { World::ResourceType_Wood, World::ResourceType_RealWood, World::ResourceType_ExoticWood },
      { "deposit_wood", "deposit_real_wood", "deposit_exotic_wood" }, 3 },
    { "icon_resource_stone",
      { World::ResourceType_Stone, World::ResourceType_Marble, World::ResourceType_Granite },
      { "deposit_stone", "deposit_marble", "deposit_granite" }, 3 },
    { "icon_resource_mine",
      { World::ResourceType_BronzeOre, World::ResourceType_IronOre, World::ResourceType_Coal, World::ResourceType_GoldOre, World::ResourceType_Salpeter, World::ResourceType_Titanium },
      { "deposit_bronze_ore", "deposit_iron", "deposit_coal", "deposit_gold", "deposit_salpeter", "deposit_titanium" }, 6 },
    { "icon_resource_food",
      { World::ResourceType_Fish, World::ResourceType_Water, World::ResourceType_Meat, World::ResourceType_Wheat },
      { "deposit_fish", "deposit_water", "deposit_meat", "deposit_corn" }, 4 }
};

EditorScene::EditorScene()
    : SceneBase("Editor")
    , m_renderer(nullptr)
    , m_spriteRenderer(nullptr)
    , m_inputManager(nullptr)
    , m_binFileManager(nullptr)
    , m_textManager(nullptr)
    , m_shaderManager(nullptr)
    , m_camera(nullptr)
    , m_radialMenu(nullptr)
    , m_gridMenu(nullptr)
    , m_mapEditor(nullptr)
    , m_unitManager(nullptr)
    , m_entityManager(nullptr)
    , m_animalSystem(nullptr)
    , m_animalManager(nullptr)
    , m_wildlife(nullptr)
    , m_currentLayer(World::Ground)
    , m_objectGroupIndex(0)
    , m_yButtonWasPressed(false)
    , m_blockCameraUntilStickNeutral(false)
    , m_fps(0)
    , m_frameCount(0)
    , m_lastFpsTime(0)
    , m_selectedTileX(0)
    , m_selectedTileY(0)
    , m_hasSelection(false)
    , m_inputController(nullptr)
    , m_currentState(STATE_IDLE)
    , m_activeResourceType(World::ResourceType_None)
    , m_phantomTileX(0)
    , m_phantomTileY(0)
    , m_editorMode(MODE_TERRAIN)
    , m_weightMenuVisible(false)
    , m_activeWeight(World::Weight_Land)
    , m_weightMenu(nullptr)
    , m_depositBuildingSpriteIdx(-1)
    , m_depositConfirmPending(false)
    , m_resourceAmount(10)
    , m_resourceMenuGroupIndex(-1)
    , m_resourceMenuShowingGroups(true)
    , m_resourcesInitialized(false)
    , m_saveLoadMenuActive(false)
    , m_saveLoadMenuSection(0)
    , m_saveLoadMenuSelection(0)
    , m_saveLoadMenuPendingSlot(0)
	, m_saveLoadMenuInputTimer(0.0f)
{
}

EditorScene::~EditorScene() {
    // Clear map's back-pointer to WildlifeSystem before deleting it
    if (m_wildlife && m_mapEditor && m_mapEditor->GetMap()) {
        m_mapEditor->GetMap()->SetWildlifeSystem(NULL);
    }
    delete m_wildlife;
    m_wildlife = nullptr;
    delete m_animalManager;
    m_animalManager = nullptr;
    delete m_animalSystem;
    m_animalSystem = nullptr;
    delete m_entityManager;
    m_entityManager = nullptr;
    delete m_unitManager;
    m_unitManager = nullptr;
    if (m_mapEditor) {
        delete m_mapEditor;
        m_mapEditor = nullptr;
    }
    if (m_radialMenu) {
        delete m_radialMenu;
        m_radialMenu = nullptr;
    }
    if (m_gridMenu) {
        delete m_gridMenu;
        m_gridMenu = nullptr;
    }
    if (m_weightMenu) {
        delete m_weightMenu;
        m_weightMenu = nullptr;
    }
    if (m_inputController) {
        delete m_inputController;
        m_inputController = nullptr;
    }
    if (m_camera) {
        delete m_camera;
        m_camera = nullptr;
    }
}

void EditorScene::Load() {
	if (m_loaded) return;
	OutputDebugStringA("[EditorScene] Load() called\n");

	if (!m_renderer) {
		OutputDebugStringA("[EditorScene] ERROR: m_renderer is NULL\n");
		return;
	}
	if (!m_inputManager) {
		OutputDebugStringA("[EditorScene] ERROR: m_inputManager is NULL\n");
		return;
	}
	if (!m_binFileManager) {
		OutputDebugStringA("[EditorScene] ERROR: m_binFileManager is NULL\n");
		return;
	}

	// Get ShaderManager from Renderer BEFORE initialization
	if (m_renderer) {
		m_shaderManager = m_renderer->GetShaderManager();
		m_spriteRenderer = m_renderer->GetSpriteRenderer();
		OutputDebugStringA("[EditorScene] Got ShaderManager from Renderer\n");
	}

	// Initialize ShaderManager with centralized shader loading
	if (m_shaderManager) {
		OutputDebugStringA("[EditorScene] Initializing ShaderManager...\n");
		if (!m_shaderManager->Init()) {
			OutputDebugStringA("[EditorScene] WARNING: Some shaders failed to load\n");
		}
	} else {
		OutputDebugStringA("[EditorScene] ERROR: m_shaderManager is NULL after GetShaderManager()\n");
	}

	OutputDebugStringA("[EditorScene] All dependencies OK, loading RadialMenu...\n");

	// Initialize Camera for world-space rendering
	m_camera = new Camera();
	if (m_camera) {
		m_camera->Initialize(1280.0f, 720.0f, m_shaderManager);
		m_camera->SetPosition(300.0f, 300.0f);
		m_camera->Zoom(0.0f); // stays at default 1.0f
		OutputDebugStringA("[EditorScene] Camera initialized and bound to ShaderManager\n");
	}

	// Initialize InputController for world coordinate translation
	m_inputController = new Logic::InputController();
	if (m_inputController && m_inputManager) {
		m_inputController->Initialize(m_camera, m_inputManager->GetGamepad());
		OutputDebugStringA("[EditorScene] InputController initialized\n");
	}

	m_weightMenu = new UI::WeightMenu();
	if (m_weightMenu) {
		m_weightMenu->Initialize(m_spriteRenderer, m_textManager);
		OutputDebugStringA("[EditorScene] WeightMenu stub created (textures set later)\n");
	}

	// Create and initialize RadialMenu
	m_radialMenu = new RadialMenu(m_renderer->GetDevice(), m_renderer->GetShaderManager(), m_binFileManager);
	if (m_radialMenu) {
		m_radialMenu->Initialize();

		// Add menu items for layers (name, typeId, spriteName from maptiles UI group)
		std::vector<RadialMenu::MenuItem> items;
		items.push_back(RadialMenu::MenuItem(UI::MSG_NONE, UI::UiAction(UI::UI_CMD_SET_LAYER, World::Roads), "build_way"));
		items.push_back(RadialMenu::MenuItem(UI::MSG_NONE, UI::UiAction(UI::UI_CMD_SET_LAYER, World::Nodes), "set_nodes"));
		items.push_back(RadialMenu::MenuItem(UI::MSG_NONE, UI::UiAction(UI::UI_CMD_SET_LAYER, World::Placement), "set_placement"));
		items.push_back(RadialMenu::MenuItem(UI::MSG_NONE, UI::UiAction(UI::UI_CMD_SET_LAYER, World::Resources), "set_resources"));
		items.push_back(RadialMenu::MenuItem(UI::MSG_NONE, UI::UiAction(UI::UI_CMD_SET_LAYER, World::Ground), "set_bg"));
		items.push_back(RadialMenu::MenuItem(UI::MSG_NONE, UI::UiAction(UI::UI_CMD_SET_LAYER, World::Objects), "set_landscape"));
		items.push_back(RadialMenu::MenuItem(UI::MSG_NONE, UI::UiAction(UI::UI_CMD_SET_LAYER, World::Buildings), "select_building"));
		m_radialMenu->SetItems(items);
	}

	// === TEXTURE SETUP ===

	TextureRegistry& registry = TextureRegistry::instance();
	registry.initializeFromManifest("game:\\Media\\Config\\textures.ini", "AtlasTextures");
	m_bgEditorTexture.SetTexture(registry.getTextureOrLoad("background_editor"));
	if (m_bgEditorTexture.GetTexture() && m_spriteRenderer) {
		m_spriteRenderer->SetTextureSlot(10, m_bgEditorTexture.GetTexture());
	}

	LPDIRECT3DTEXTURE9 groundTexture = registry.getTextureOrLoad("ground");

	std::tr1::shared_ptr<SpriteAtlas> groundAtlas = registry.getAtlas("ground");

	// Ground texture in slot 0
	if (groundTexture && m_spriteRenderer) {
		m_spriteRenderer->SetTextureSlot(0, groundTexture);
	}

	// Maptiles atlas (loaded by LoadingScene) — slot 5 for RadialMenu icons
	std::tr1::shared_ptr<SpriteAtlas> maptilesAtlas = registry.getAtlas("maptiles");
	LPDIRECT3DTEXTURE9 maptilesTex = (maptilesAtlas ? maptilesAtlas->GetTexture() : NULL);
	if (maptilesTex && m_spriteRenderer) {
		m_spriteRenderer->SetTextureSlot(5, maptilesTex);
		if (m_radialMenu) {
			m_radialMenu->SetIconTextureSlot(14);
		}
	}

	// Streets atlas for Roads layer — slot 3
	LPDIRECT3DTEXTURE9 streetsTex = registry.getTextureOrLoad("streets");
	if (streetsTex && m_spriteRenderer) {
		m_spriteRenderer->SetTextureSlot(3, streetsTex);
		OutputDebugStringA("[EditorScene] Streets atlas loaded and bound to slot 3\n");
	}

	// Buildings atlas — slot 1
	LPDIRECT3DTEXTURE9 buildingsTex = registry.getTextureOrLoad("Buildings");
	if (buildingsTex && m_spriteRenderer) {
		m_spriteRenderer->SetTextureSlot(1, buildingsTex);
		OutputDebugStringA("[EditorScene] Buildings atlas loaded and bound to slot 1\n");
	}

	// UI atlas (loaded by LoadingScene) — slot 14 for cursor, button hints, menu sprites
	std::tr1::shared_ptr<SpriteAtlas> uiAtlas = registry.getAtlas("ui");
	LPDIRECT3DTEXTURE9 uiTex = (uiAtlas ? uiAtlas->GetTexture() : NULL);
	if (uiTex && m_spriteRenderer) {
		m_spriteRenderer->SetTextureSlot(14, uiTex);
	}

	// Extract bg/cell UVs from UI atlas (menu_Grid, menu_cell1)
	GridMenu::TileUV bgUV = {0,0,1,1}, cellUV = {0,0,1,1};
	GridMenu::TileUV wmDpadUV = {0,0,1,1};
	{
		std::tr1::shared_ptr<SpriteAtlas> uiAtl = registry.getAtlas("ui");
		if (uiAtl) {
			uint32_t bgIdx = uiAtl->GetIndex("menu_Grid");
			if (bgIdx != 0xFFFFFFFF) {
				const SpriteRegion* reg = uiAtl->GetRegion(bgIdx);
				if (reg) { bgUV.u0 = reg->u0; bgUV.v0 = reg->v0; bgUV.u1 = reg->u1; bgUV.v1 = reg->v1; }
			} else {
				OutputDebugStringA("[EditorScene] WARNING: 'menu_Grid' NOT FOUND in UI atlas!\n");
			}
			uint32_t cellIdx = uiAtl->GetIndex("menu_cell1");
			if (cellIdx != 0xFFFFFFFF) {
				const SpriteRegion* reg = uiAtl->GetRegion(cellIdx);
				if (reg) { cellUV.u0 = reg->u0; cellUV.v0 = reg->v0; cellUV.u1 = reg->u1; cellUV.v1 = reg->v1; }
			} else {
				OutputDebugStringA("[EditorScene] WARNING: 'menu_cell1' NOT FOUND in UI atlas!\n");
			}
			uint32_t dpadIdx = uiAtl->GetIndex("d_pad");
			if (dpadIdx != 0xFFFFFFFF) {
				const SpriteRegion* reg = uiAtl->GetRegion(dpadIdx);
				if (reg) { wmDpadUV.u0 = reg->u0; wmDpadUV.v0 = reg->v0; wmDpadUV.u1 = reg->u1; wmDpadUV.v1 = reg->v1; }
			} else {
				OutputDebugStringA("[EditorScene] WARNING: 'd_pad' NOT FOUND in UI atlas!\n");
			}
		}
	}

	char logMsg[256];
	_snprintf(logMsg, sizeof(logMsg), "[EditorScene] Textures: Ground=%p, Maptiles=%p, Background=%p\n", 
		(void*)groundTexture, (void*)maptilesTex, (void*)m_bgEditorTexture.GetTexture());
	OutputDebugStringA(logMsg);

	// Register GridMenu texture slots (bg/cell from UI atlas slot 14)
	if (m_spriteRenderer) {
		m_spriteRenderer->SetTextureSlot(6, uiTex);
		m_spriteRenderer->SetTextureSlot(7, uiTex);
		m_spriteRenderer->SetTextureSlot(8, groundTexture);
		m_spriteRenderer->SetTextureSlot(9, groundTexture);
	}

	// Initialize WeightMenu textures (bg=menu_Grid, dpad=d_pad from UI atlas)
	if (m_weightMenu) {
		TextureRegistry& registry = TextureRegistry::instance();
		LPDIRECT3DTEXTURE9 fallbackTex = registry.getNotFoundTexture();

		m_weightMenu->SetTextureSlots(14, 14); // background/dpadCross slots
		m_weightMenu->SetTextures(uiTex ? uiTex : fallbackTex, uiTex ? uiTex : fallbackTex);
		m_weightMenu->SetDpadUV(wmDpadUV.u0, wmDpadUV.v0, wmDpadUV.u1, wmDpadUV.v1);
		m_weightMenu->SetBackgroundUV(bgUV.u0, bgUV.v0, bgUV.u1, bgUV.v1);
		if (m_spriteRenderer) {
			// UI atlas slot 14 is used for WeightMenu background and Dpad
			m_spriteRenderer->SetTextureSlot(14, uiTex ? uiTex : fallbackTex);
		}
		OutputDebugStringA("[EditorScene] WeightMenu textures set (menu_Grid/d_pad from UI atlas in slot 14)\n");
	}

	// Look up button hint textures from UI atlas
	m_buttonAUV.u0 = 0.0f; m_buttonAUV.v0 = 0.0f; m_buttonAUV.u1 = 1.0f; m_buttonAUV.v1 = 1.0f;
	m_buttonBUV.u0 = 0.0f; m_buttonBUV.v0 = 0.0f; m_buttonBUV.u1 = 1.0f; m_buttonBUV.v1 = 1.0f;
	{
		std::tr1::shared_ptr<SpriteAtlas> uiAtl = registry.getAtlas("ui");
		if (uiAtl) {
			uint32_t btnAIdx = uiAtl->GetIndex("button_A");
			if (btnAIdx != 0xFFFFFFFF) {
				const SpriteRegion* reg = uiAtl->GetRegion(btnAIdx);
				if (reg) { m_buttonAUV.u0 = reg->u0; m_buttonAUV.v0 = reg->v0; m_buttonAUV.u1 = reg->u1; m_buttonAUV.v1 = reg->v1; }
			}
			uint32_t btnBIdx = uiAtl->GetIndex("button_B");
			if (btnBIdx != 0xFFFFFFFF) {
				const SpriteRegion* reg = uiAtl->GetRegion(btnBIdx);
				if (reg) { m_buttonBUV.u0 = reg->u0; m_buttonBUV.v0 = reg->v0; m_buttonBUV.u1 = reg->u1; m_buttonBUV.v1 = reg->v1; }
			}
		}
	}
	if (m_spriteRenderer) {
		std::tr1::shared_ptr<SpriteAtlas> uiAtl = registry.getAtlas("ui");
		LPDIRECT3DTEXTURE9 uiTexSlot = uiAtl ? uiAtl->GetTexture() : NULL;
		m_spriteRenderer->SetTextureSlot(13, uiTexSlot ? uiTexSlot : maptilesTex);
	}

	// Load language strings
	LanguageManager::instance().LoadFromFile("game:\\Media\\Config\\language.ini");

	// Initialize GridMenu with textures (bg/cell from UI atlas)
	if (!m_gridMenu && m_renderer) {
		m_gridMenu = new GridMenu();
		if (m_gridMenu->Initialize()) {
			TextureRegistry& registry = TextureRegistry::instance();
			LPDIRECT3DTEXTURE9 fallbackTex = registry.getNotFoundTexture();

			m_gridMenu->SetTextures(uiTex ? uiTex : fallbackTex, uiTex ? uiTex : fallbackTex, groundTexture ? groundTexture : fallbackTex);
			m_gridMenu->SetTextureSlots(14, 14, 8);
			m_gridMenu->SetBackgroundUV(bgUV);
			m_gridMenu->SetCellUV(cellUV);

			// Build UVs from ground atlas if available
			if (groundAtlas) {
				std::vector<GridMenu::TileUV> allUVs;
				allUVs.reserve(groundAtlas->GetRegionCount());
				for (uint32_t i = 0; i < groundAtlas->GetRegionCount(); ++i) {
					const SpriteRegion* reg = groundAtlas->GetRegion(i);
					GridMenu::TileUV tu; 
					if (reg) { 
						tu.u0 = reg->u0; tu.v0 = reg->v0; tu.u1 = reg->u1; tu.v1 = reg->v1; 
					} else { 
						tu.u0 = 0.0f; tu.v0 = 0.0f; tu.u1 = 1.0f; tu.v1 = 1.0f; 
					}
					allUVs.push_back(tu);
				}
				m_gridMenu->SetAllTileUVs(allUVs);
			}
			m_gridMenu->SetTextManager(m_textManager);
			m_gridMenu->SetWindowStart(0);
			m_gridMenu->SetSpriteRenderer(m_spriteRenderer);
			OutputDebugStringA("[EditorScene] GridMenu initialized with textures (named or atlas-based).\n");
		} else {
			delete m_gridMenu;
			m_gridMenu = nullptr;
		}
	}

	// Initialize MapEditor
	if (!m_mapEditor && m_renderer && m_inputManager) {
		m_mapEditor = new Editor::MapEditor();
		World::Map* map = new World::Map(s_mapGridWidth, s_mapGridHeight, s_mapGridWidth * 2, s_mapGridHeight * 4);
		m_mapEditor->Initialize(map, m_renderer, m_inputManager, m_renderer->GetDevice());
		m_mapEditor->SetSpriteRenderer(m_spriteRenderer);
		m_mapEditor->SetCamera(m_camera);
		if (m_textManager) {
			m_mapEditor->SetTextManager(m_textManager);
		}
		OutputDebugStringA("[EditorScene] MapEditor initialized\n");
	}

	// ShaderManager already obtained from Renderer earlier in Load()
	// No need to get it again here

	OutputDebugStringA("[EditorScene] Texture slots configured\n");

	char debugMsg[256];
	_snprintf(debugMsg, sizeof(debugMsg), "[EditorScene] Main texture slots: ground=%p, maptiles=%p, ui=%p\n", 
		groundTexture, maptilesTex, uiTex);
	OutputDebugStringA(debugMsg);

	// Units atlas — slot 2, initialize UnitManager
	{
		LPDIRECT3DTEXTURE9 unitsTex = registry.getTextureOrLoad("Units");
		if (unitsTex && m_spriteRenderer) {
			m_spriteRenderer->SetTextureSlot(11, unitsTex);
			std::tr1::shared_ptr<SpriteAtlas> unitsAtlas = registry.getAtlas("Units");
			if (unitsAtlas && m_mapEditor) {
				m_unitManager = new Game::UnitManager();
				m_unitManager->SetAtlas(unitsAtlas);
				m_unitManager->SetTextureSlot(11);
				m_unitManager->Initialize(m_mapEditor->GetMap());
				OutputDebugStringA("[EditorScene] Units atlas loaded to slot 11, UnitManager created\n");
			}
		}
	}

	// Initialize ECS + WildlifeSystem for animal rendering in editor
	if (m_mapEditor && m_mapEditor->GetMap()) {
		World::Map* map = m_mapEditor->GetMap();
		m_entityManager = new World::EntityManager();
		m_animalSystem = new World::AnimalSystem(m_entityManager, map);
		m_animalManager = new World::AnimalManager(m_entityManager, m_animalSystem);
		m_animalManager->Init(&map->GetHabitatRegistry());
		m_wildlife = new World::WildlifeSystem(map, m_animalManager, m_animalSystem);
		map->SetWildlifeSystem(m_wildlife);
		OutputDebugStringA("[EditorScene] ECS WildlifeSystem initialized\n");
	}

	OutputDebugStringA("[EditorScene] Load() complete\n");
	m_loaded = true;
}

void EditorScene::Unload() {
    if (m_wildlife) {
        if (m_mapEditor && m_mapEditor->GetMap()) {
            m_mapEditor->GetMap()->SetWildlifeSystem(NULL);
        }
        delete m_wildlife;
        m_wildlife = NULL;
    }
    if (m_animalSystem) {
        delete m_animalSystem;
        m_animalSystem = NULL;
    }
    if (m_animalManager) {
        delete m_animalManager;
        m_animalManager = NULL;
    }
    if (m_entityManager) {
        delete m_entityManager;
        m_entityManager = NULL;
    }
    if (m_radialMenu) {
        m_radialMenu->Shutdown();
    }
}

void EditorScene::Update(float deltaTime) {
    UpdateFPS();

    if (!m_inputManager) return;

    Input::Gamepad* gamepad = m_inputManager->GetGamepad();
    if (!gamepad) return;

    // Save/Load menu toggle (Start button)
    if (!m_saveLoadMenuActive && m_mapEditor && gamepad->IsButtonPressed(Input::GP_Start)) {
        m_saveLoadMenuActive = true;
        m_saveLoadMenuSection = 0;
        m_saveLoadMenuSelection = 0;
    }

    // If save/load menu is active, handle it and skip everything else
    if (m_saveLoadMenuActive) {
        UpdateSaveLoadMenu(gamepad, deltaTime);
        return;
    }

    bool menuActive = (m_gridMenu && m_gridMenu->IsVisible()) || (m_radialMenu && m_radialMenu->IsVisible());

    UpdateMenus(gamepad, deltaTime);

    if (m_inputController) {
        UpdateInputController(deltaTime);
        UpdateResourcePlacementFSM();
    }

    // GridMenu resource selection must happen after FSM to prevent
    // processing the same A press as a placement
    if (HandleGridMenuResourceSelection(gamepad))
        return;

    if (!menuActive && m_camera) {
        UpdateCamera(gamepad, deltaTime);
    }

    if (m_inputController && m_camera && m_mapEditor) {
        UpdateCursorAndTiles();
    }

	if (m_mapEditor && !menuActive) {
		UpdateMapEditor(deltaTime, gamepad);
	}

	// X button deletion works even when menu is active (UpdateMapEditor is skipped)
	if (m_mapEditor && menuActive && gamepad->IsButtonPressed(Input::GP_X)) {
		if (m_currentLayer == World::Roads && m_mapEditor->GetRoadBuildState() == Editor::ROAD_FLAG) {
			m_mapEditor->ToggleFlag(m_mapEditor->GetCursorTileX(), m_mapEditor->GetCursorTileY());
		} else if (m_currentLayer != World::Resources) {
			m_mapEditor->DeleteObjectAt(m_mapEditor->GetCursorTileX(), m_mapEditor->GetCursorTileY());
			// Rebuild menu so newly available icons (e.g. town hall) appear
			if (m_gridMenu && m_gridMenu->IsVisible() && m_currentLayer == World::Buildings) {
				LoadGridMenuAtlas("Buildings");
			}
		}
	}

    // Update UnitManager (rebuild network when flags change)
    if (m_unitManager && m_mapEditor) {
        m_unitManager->SetRenderQueue(m_mapEditor->GetRenderQueue());
        static int s_lastFlagCount = -1;
        int flagCount = (int)m_mapEditor->GetRoadFlags().size();
        if (flagCount != s_lastFlagCount) {
            m_unitManager->RebuildRoadNetwork(m_mapEditor->GetRoadFlags());
            s_lastFlagCount = flagCount;
        }
        m_unitManager->Update(deltaTime);
    }

    // Update wildlife (spawning, movement)
    if (m_wildlife && m_mapEditor && m_mapEditor->GetMap()) {
        m_wildlife->Update(deltaTime, m_mapEditor->GetMap()->GetHabitatRegistry());
    }
}

void EditorScene::UpdateFPS() {
    DWORD now = GetTickCount();
    m_frameCount++;
    if (now - m_lastFpsTime >= 1000) {
        m_fps = m_frameCount;
        m_frameCount = 0;
        m_lastFpsTime = now;
    }
}

void EditorScene::UpdateMenus(Input::Gamepad* gamepad, float deltaTime) {
    bool anyMenuActive = (m_gridMenu && m_gridMenu->IsVisible()) || (m_radialMenu && m_radialMenu->IsVisible());

    UpdateWeightMenu(gamepad, deltaTime);
    UpdateLBButton(gamepad);
    UpdateRBButton(gamepad, anyMenuActive);
    UpdateGridMenu(gamepad, deltaTime);
    UpdateRadialMenu(gamepad);
}

void EditorScene::UpdateWeightMenu(Input::Gamepad* gamepad, float deltaTime) {
    if (m_weightMenuVisible && m_weightMenu) {
        m_weightMenu->Update(gamepad, deltaTime);
        m_weightMenuVisible = m_weightMenu->IsVisible();
        if (m_weightMenuVisible) {
            bool selected = false;
            if (IsPlacementMode()) {
                if (gamepad->IsButtonPressed(Input::GP_DPadUp)) {
                    m_activeWeight = World::Weight_Block;
                    selected = true;
                } else if (gamepad->IsButtonPressed(Input::GP_DPadDown)) {
                    m_activeWeight = World::Weight_Land;
                    selected = true;
                }
            } else {
                if (gamepad->IsButtonPressed(Input::GP_DPadUp)) {
                    m_activeWeight = World::Weight_Block;
                    selected = true;
                } else if (gamepad->IsButtonPressed(Input::GP_DPadDown)) {
                    m_activeWeight = World::Weight_Deep;
                    selected = true;
                } else if (gamepad->IsButtonPressed(Input::GP_DPadLeft)) {
                    m_activeWeight = World::Weight_Shallow;
                    selected = true;
                } else if (gamepad->IsButtonPressed(Input::GP_DPadRight)) {
                    m_activeWeight = World::Weight_Land;
                    selected = true;
                }
            }

            if (selected) {
                m_editorMode = IsPlacementMode() ? MODE_PLACEMENT : MODE_WEIGHTS;
                m_weightMenu->Close();
                m_weightMenuVisible = false;
            }
        }
    }
}

void EditorScene::UpdateLBButton(Input::Gamepad* gamepad) {
    if (gamepad->IsButtonPressed(Input::GP_LB)) {
        if (m_gridMenu && !m_gridMenu->IsVisible() && m_mapEditor) {
            m_mapEditor->SetCursorPreview(-1);
        }

        if (m_gridMenu && m_gridMenu->IsVisible()) {
            // GridMenu active, ignore LB
        } else if (m_radialMenu) {
            if (m_radialMenu->IsVisible()) {
                m_radialMenu->Hide();
            } else {
                m_radialMenu->Show(640.0f, 360.0f);
            }
        }
    }
}

void EditorScene::UpdateRBButton(Input::Gamepad* gamepad, bool anyMenuActive) {
    if (!anyMenuActive && !m_weightMenuVisible) {
        if (gamepad->IsButtonPressed(Input::GP_RB)) {
            ResetShaderState();
            HandleRBButtonAction();
        }
    }
}

void EditorScene::HandleRBButtonAction() {
    if (m_currentLayer == World::Nodes || m_currentLayer == World::Placement) {
        HandleWeightMenuToggle();
    } else if (m_currentLayer == World::Resources && m_currentState == STATE_IDLE) {
        HandleResourceMenuToggle();
    } else if (m_currentLayer == World::Roads) {
        HandleRoadMenuToggle();
    } else {
        HandleDefaultMenuToggle();
    }
}

void EditorScene::HandleWeightMenuToggle() {
    if (m_weightMenu) {
        if (m_weightMenuVisible) {
            m_weightMenu->Close();
            m_weightMenuVisible = false;
        } else {
            TextureRegistry::instance().refreshTexture("ui");
            std::tr1::shared_ptr<SpriteAtlas> uiAtl = TextureRegistry::instance().getAtlas("ui");
            LPDIRECT3DTEXTURE9 uiTex = uiAtl ? uiAtl->GetTexture() : NULL;
            if (m_spriteRenderer) {
                m_spriteRenderer->SetTextureSlot(6, uiTex);
                m_spriteRenderer->SetTextureSlot(7, uiTex);
                m_spriteRenderer->SetTextureSlot(13, uiTex);
                m_spriteRenderer->SetTextureSlot(14, uiTex);
            }
            m_weightMenu->SetPlacementMode(IsPlacementMode());
            m_weightMenu->Open(m_activeWeight);
            m_weightMenuVisible = true;
        }
    }
}

void EditorScene::HandleResourceMenuToggle() {
    if (!m_gridMenu) {
        CreateResourceGridMenu();
    } else if (m_gridMenu->IsVisible()) {
        m_gridMenu->Hide();
    } else {
        TextureRegistry::instance().refreshTexture("ui");
        LoadResourceGroupIcons();
        m_gridMenu->Show(640.0f, 330.0f);
    }
}

void EditorScene::HandleRoadMenuToggle() {
    if (!m_gridMenu) {
        CreateRoadGridMenu();
    } else if (m_gridMenu->IsVisible()) {
        m_gridMenu->Hide();
    } else {
        TextureRegistry::instance().refreshTexture("ui");
        std::vector<std::string> roadIcons;
        roadIcons.push_back("icon_Streets");
        roadIcons.push_back("icon_Flags");
        LoadUIAtlasGroup("Streets_Flags", roadIcons);
        m_gridMenu->Show(640.0f, 330.0f);
    }
}

void EditorScene::HandleDefaultMenuToggle() {
    if (!m_gridMenu) {
        CreateDefaultGridMenu();
    } else if (m_gridMenu->IsVisible()) {
        m_gridMenu->Hide();
    } else {
		TextureRegistry::instance().refreshTexture("ui");
		if (m_currentLayer == World::Objects) {
			LoadGridMenuGroup(kObjectGroupNames[m_objectGroupIndex]);
			if (m_mapEditor) m_mapEditor->SetObjectGroup(kObjectGroupNames[m_objectGroupIndex]);
		} else if (m_currentLayer == World::Buildings) {
			LoadGridMenuAtlas("Buildings");
		} else {
			LoadGridMenuAtlas("ground");
		}
        m_gridMenu->Show(640.0f, 330.0f);
    }
}

void EditorScene::CreateResourceGridMenu() {
    m_gridMenu = new GridMenu();
    if (m_gridMenu->Initialize()) {
        m_gridMenu->SetTextureSlots(6, 7, 8);
        LoadResourceGroupIcons();
    }
    m_gridMenu->Show(640.0f, 330.0f);
}

void EditorScene::CreateRoadGridMenu() {
    m_gridMenu = new GridMenu();
    if (m_gridMenu->Initialize()) {
        m_gridMenu->SetTextureSlots(6, 7, 8);
        std::vector<std::string> roadIcons;
        roadIcons.push_back("icon_Streets");
        roadIcons.push_back("icon_Flags");
        LoadUIAtlasGroup("Streets_Flags", roadIcons);
    }
    m_gridMenu->Show(640.0f, 330.0f);
}

void EditorScene::CreateDefaultGridMenu() {
    m_gridMenu = new GridMenu();
    if (m_gridMenu->Initialize()) {
        m_gridMenu->SetTextureSlots(6, 7, 8);
        if (m_currentLayer == World::Objects) {
            m_objectGroupIndex = 0;
            LoadGridMenuGroup(kObjectGroupNames[m_objectGroupIndex]);
            if (m_mapEditor) m_mapEditor->SetObjectGroup(kObjectGroupNames[m_objectGroupIndex]);
        } else if (m_currentLayer == World::Buildings) {
            LoadGridMenuAtlas("Buildings");
        } else {
            LoadGridMenuAtlas("ground");
        }
    }
    m_gridMenu->Show(640.0f, 330.0f);
}

void EditorScene::UpdateGridMenu(Input::Gamepad* gamepad, float deltaTime) {
    if (m_gridMenu && !m_gridMenu->IsVisible() && m_mapEditor) {
        m_mapEditor->SetCursorPreview(-1);
    }

    if (m_gridMenu && m_gridMenu->IsVisible()) {
        if (!m_spriteRenderer) {
            OutputDebugStringA("[EditorScene] WARNING: spriteRenderer is NULL during GridMenu update\n");
            return;
        }

        m_gridMenu->Update(gamepad, deltaTime);

        // Update cursor preview with currently highlighted sprite
        if (m_mapEditor) {
            if (m_gridMenu->IsVisible() && (m_currentLayer == World::Ground || m_currentLayer == World::Objects || m_currentLayer == World::Roads || m_currentLayer == World::Buildings)) {
                int previewIdx = m_gridMenu->GetSelectedSpriteIndex();
                m_mapEditor->SetCursorPreview(previewIdx);
            } else {
                m_mapEditor->SetCursorPreview(-1);
            }
        }

        HandleGridMenuInput(gamepad);
    }
}

void EditorScene::HandleGridMenuInput(Input::Gamepad* gamepad) {
    if (gamepad->IsButtonPressed(Input::GP_A)) {
        HandleGridMenuAButton();
    }

    if (gamepad->IsButtonPressed(Input::GP_B)) {
        HandleGridMenuBButton();
    }

    if (gamepad->IsButtonPressed(Input::GP_Y)) {
        HandleGridMenuYButton();
    }

    if (gamepad->IsButtonPressed(Input::GP_LB)) {
        m_gridMenu->PrevPage();
    }
    if (gamepad->IsButtonPressed(Input::GP_RB)) {
        m_gridMenu->NextPage();
    }
}

void EditorScene::HandleGridMenuAButton() {
    if (m_gridMenu && m_gridMenu->IsVisible()) {
        int selectedIndex = m_gridMenu->GetSelectedSpriteIndex();
        if (selectedIndex >= 0 && m_mapEditor) {
            if (m_currentLayer == World::Roads) {
                HandleRoadSelection(selectedIndex);
            } else if (m_currentLayer != World::Resources) {
                m_mapEditor->SetTileByIndex(selectedIndex);
                m_gridMenu->Hide();
                m_mapEditor->SetCursorPreview(-1);
            }
        }
    }
}

void EditorScene::HandleRoadSelection(int selectedIndex) {
    if (!m_mapEditor) return;
    
    TextureRegistry& registry = TextureRegistry::instance();
    std::tr1::shared_ptr<SpriteAtlas> uiAtl = registry.getAtlas("ui");
    if (!uiAtl) return;

    // Получаем имя выбранной иконки
    const SpriteRegion* selectedRegion = uiAtl->GetRegion(selectedIndex);
    if (!selectedRegion) return;

    // Определяем, что было выбрано
    if (selectedRegion->name == "icon_Streets") {
        m_mapEditor->SetRoadFlagMode(false);
        OutputDebugStringA("[EditorScene] Road building mode selected\n");
    } else if (selectedRegion->name == "icon_Flags") {
        m_mapEditor->SetRoadFlagMode(true);
        OutputDebugStringA("[EditorScene] Flag placement mode selected\n");
    }
    
    // Скрываем меню
    if (m_gridMenu) {
        m_gridMenu->Hide();
    }
    m_mapEditor->SetCursorPreview(-1);
}

void EditorScene::HandleGridMenuBButton() {
    if (m_currentLayer == World::Resources && !m_resourceMenuShowingGroups) {
        LoadResourceGroupIcons();
    } else {
        m_gridMenu->Hide();
        m_mapEditor->SetCursorPreview(-1);
    }
}

void EditorScene::HandleGridMenuYButton() {
    if (m_currentLayer == World::Objects) {
        CycleObjectGroup();
    } else if (m_currentLayer == World::Roads) {
        Editor::RoadBuildState state = m_mapEditor->GetRoadBuildState();
        if (state == Editor::ROAD_IDLE || state == Editor::ROAD_FLAG) {
            m_mapEditor->SetRoadFlagMode(state != Editor::ROAD_FLAG);
            OutputDebugStringA(state == Editor::ROAD_FLAG ? "[Editor] Road building mode\n" : "[Editor] Flag placement mode\n");
        }
    }
}

void EditorScene::UpdateRadialMenu(Input::Gamepad* gamepad) {
    if (m_radialMenu && m_radialMenu->IsVisible()) {
        m_radialMenu->Update(gamepad);

        if (!m_radialMenu->IsVisible()) {
            m_blockCameraUntilStickNeutral = true;
        }

        if (m_radialMenu->HasSelection()) {
            HandleRadialMenuSelection();
        }
    }
}

void EditorScene::HandleRadialMenuSelection() {
    UI::UiAction action = m_radialMenu->GetSelectedAction();
    m_currentLayer = static_cast<World::LayerType>(action.value);

    if (m_currentLayer != World::Objects) {
        m_yButtonWasPressed = false;
    }

    m_editorMode = MODE_TERRAIN;

    if (m_mapEditor) {
        m_mapEditor->SetLayer(m_currentLayer);

        m_currentState = STATE_IDLE;
        m_activeResourceType = World::ResourceType_None;
        m_depositConfirmPending = false;
        m_depositBuildingSpriteIdx = -1;

        switch (m_currentLayer) {
            case World::Ground:
                m_mapEditor->SetShowObjects(false);
                m_mapEditor->SetShowOverlay(false);
                m_mapEditor->SetShowResourceIcons(false);
                break;
            case World::Objects:
                m_mapEditor->SetShowObjects(true);
                m_mapEditor->SetShowOverlay(false);
                m_mapEditor->SetShowResourceIcons(false);
                break;
            case World::Resources:
                m_mapEditor->SetShowObjects(true);
                m_mapEditor->SetShowOverlay(true);
                m_mapEditor->SetShowResourceIcons(true);
                if (!m_resourcesInitialized) {
                    m_mapEditor->AutoAssignResourcesForTrees();
                    m_resourcesInitialized = true;
                }
                break;
            case World::Overlay:
                m_mapEditor->SetShowObjects(true);
                m_mapEditor->SetShowOverlay(true);
                m_mapEditor->SetShowResourceIcons(false);
                break;
            case World::Buildings:
                m_mapEditor->SetShowObjects(false);
                m_mapEditor->SetShowOverlay(false);
                m_mapEditor->SetShowResourceIcons(false);
                break;
            default:
                m_mapEditor->SetShowObjects(true);
                m_mapEditor->SetShowOverlay(true);
                m_mapEditor->SetShowResourceIcons(false);
                break;
        }

        if (m_currentLayer == World::Objects) {
            m_mapEditor->SetObjectGroup(kObjectGroupNames[m_objectGroupIndex]);
        }
    }

	if (m_spriteRenderer) {
        TextureRegistry& reg = TextureRegistry::instance();
        const char* atlasName;
        if (m_currentLayer == World::Buildings)
            atlasName = "Buildings";
        else if (m_currentLayer == World::Objects)
            atlasName = "maptiles";
        else
            atlasName = "ground";
        std::tr1::shared_ptr<SpriteAtlas> atlas = reg.getAtlas(atlasName);
        if (atlas) {
            m_spriteRenderer->SetTextureSlot(8, atlas->GetTexture());
        }
    }
}

void EditorScene::UpdateInputController(float deltaTime) {
    m_inputController->Update(deltaTime);
}

void EditorScene::UpdateCursorAndTiles() {
    if (m_editorMode == MODE_WEIGHTS && !m_weightMenuVisible) {
        float centerWorldX, centerWorldY;
        m_camera->GetWorldCenter(centerWorldX, centerWorldY);
        if (m_mapEditor && m_mapEditor->GetMap()) {
            int tileX, tileY;
            if (m_mapEditor->GetMap()->GetTileAt(centerWorldX, centerWorldY, m_currentLayer, tileX, tileY)) {
                m_selectedTileX = tileX;
                m_selectedTileY = tileY;
                m_hasSelection = true;
            }
            m_mapEditor->SetCursorWorldPosition(centerWorldX, centerWorldY);
        }
    } else {
        float offsetX, offsetY;
        m_inputController->GetCursorOffset(offsetX, offsetY);

        float camX, camY;
        m_camera->GetWorldCenter(camX, camY);

        float worldX = camX + offsetX;
        float worldY = camY + offsetY;

        m_mapEditor->SetCursorWorldPosition(worldX, worldY);

        if (m_mapEditor && m_mapEditor->GetMap()) {
            int tileX, tileY;
            if (m_mapEditor->GetMap()->GetTileAt(worldX, worldY, m_currentLayer, tileX, tileY)) {
                if (m_currentState == STATE_PLACING) {
                    m_phantomTileX = tileX;
                    m_phantomTileY = tileY;
                }
            }
        }
    }
}

void EditorScene::UpdateResourcePlacementFSM() {
    switch (m_currentState) {
        case STATE_IDLE:
            if (m_editorMode == MODE_WEIGHTS && !m_weightMenuVisible) {
                if (m_inputController->IsButtonAPressed()) {
                    if (m_mapEditor && m_mapEditor->GetMap() && m_hasSelection) {
                        m_mapEditor->GetMap()->SetNodeWeight(m_selectedTileX, m_selectedTileY, m_activeWeight);
                    }
                }
            }
            break;

        case STATE_SELECTING:
            break;

        case STATE_PLACING:
            if (m_inputController->IsButtonAPressed()) {
                if (m_mapEditor && m_mapEditor->GetMap()) {
                    bool canPlace = true;
                    int placeX = m_phantomTileX;
                    int placeY = m_phantomTileY;
                    if (World::ResourceRequiresMountain(m_activeResourceType)) {
                        m_mapEditor->RebuildObjectInteractionZones();
                        if (!m_mapEditor->FindMountainObjectForResource(m_phantomTileX, m_phantomTileY, placeX, placeY)) {
                            OutputDebugStringA("[Editor] Deposits can only be placed on mountains!\n");
                            canPlace = false;
                        }
                    }
                    if (canPlace) {
                        m_phantomTileX = placeX;
                        m_phantomTileY = placeY;
                        m_mapEditor->GetMap()->SetResourceNode(placeX, placeY, m_activeResourceType, m_resourceAmount, true);
                        m_currentState = STATE_INPUT_AMOUNT;
                    }
                }
            }
            if (m_inputController->IsButtonBPressed()) {
                m_currentState = STATE_IDLE;
                m_activeResourceType = World::ResourceType_None;
            }
            break;

        case STATE_INPUT_AMOUNT:
            if (m_inputController->IsButtonYPressed()) {
                if (m_mapEditor && m_mapEditor->GetMap()) {
                    m_mapEditor->GetMap()->SetResourceNode(m_phantomTileX, m_phantomTileY, m_activeResourceType, m_resourceAmount, true);
                }
                m_currentState = STATE_IDLE;
            }
            if (m_inputController->IsButtonBPressed()) {
                m_currentState = STATE_IDLE;
                m_activeResourceType = World::ResourceType_None;
                OutputDebugStringA("[Editor] Resource edit cancelled\n");
            }
            break;
    }
}

void EditorScene::UpdateCamera(Input::Gamepad* gamepad, float deltaTime) {
    float moveSpeed = 2000.0f * deltaTime;
    float stickX, stickY;
    gamepad->GetLeftStick(stickX, stickY);

    if (m_blockCameraUntilStickNeutral) {
        if (fabsf(stickX) <= 0.1f && fabsf(stickY) <= 0.1f) {
            m_blockCameraUntilStickNeutral = false;
        }
    } else if (fabsf(stickX) > 0.1f || fabsf(stickY) > 0.1f) {
        m_camera->Move(stickX * moveSpeed, stickY * moveSpeed);
    }

    float rightX, rightY;
    gamepad->GetRightStick(rightX, rightY);
    if (fabsf(rightY) > 0.1f) {
        m_camera->Zoom(rightY * 0.3f * deltaTime);
    }

    m_camera->Update(deltaTime);

    if (m_shaderManager) {
        m_shaderManager->UpdateGlobalMatrices(&m_camera->GetViewMatrix(), &m_camera->GetProjectionMatrix());
    }
}

void EditorScene::UpdateMapEditor(float deltaTime, Input::Gamepad* gamepad) {
    m_mapEditor->Update(deltaTime);

    if (m_currentLayer == World::Resources) {
        if (gamepad->IsButtonPressed(Input::GP_DPadUp)) {
            m_resourceAmount += 5;
            if (m_resourceAmount > 999) m_resourceAmount = 999;
        }
        if (gamepad->IsButtonPressed(Input::GP_DPadDown)) {
            m_resourceAmount -= 5;
            if (m_resourceAmount < 1) m_resourceAmount = 1;
        }

        if (gamepad->IsButtonPressed(Input::GP_X)) {
            int tx = m_mapEditor->GetCursorTileX();
            int ty = m_mapEditor->GetCursorTileY();
            m_mapEditor->GetMap()->SetResourceNode(tx, ty, World::ResourceType_None, 0, false);
        }

        if (gamepad->IsButtonPressed(Input::GP_A) && m_currentState == STATE_IDLE) {
            int tx = m_mapEditor->GetCursorTileX();
            int ty = m_mapEditor->GetCursorTileY();

            const World::ResourceNode& existingNode = m_mapEditor->GetMap()->GetResourceNode(tx, ty);
            if (existingNode.type != World::ResourceType_None) {
                m_mapEditor->GetMap()->SetResourceNode(tx, ty, existingNode.type, m_resourceAmount, true);
                char buf[128];
                sprintf_s(buf, "[Editor] Resource updated at (%d,%d): %s amount=%d\n",
                    tx, ty, World::ResourceTypeToString(existingNode.type), m_resourceAmount);
                OutputDebugStringA(buf);
            } else if (m_activeResourceType != World::ResourceType_None) {
                bool canPlace = true;
                int placeX = tx;
                int placeY = ty;
                if (World::ResourceRequiresMountain(m_activeResourceType)) {
                    m_mapEditor->RebuildObjectInteractionZones();
                    if (!m_mapEditor->FindMountainObjectForResource(tx, ty, placeX, placeY)) {
                        OutputDebugStringA("[Editor] Deposits can only be placed on mountains!\n");
                        canPlace = false;
                    }
                }
                if (canPlace) {
                    m_mapEditor->GetMap()->SetResourceNode(placeX, placeY, m_activeResourceType, m_resourceAmount, true);
                }
            }
        }
        return;
    }

    // Cancel road building with B
    if (gamepad->IsButtonPressed(Input::GP_B)) {
        if (m_currentLayer == World::Roads && m_mapEditor->GetRoadBuildState() == Editor::ROAD_PLACING) {
            m_mapEditor->CancelRoad();
            OutputDebugStringA("[Editor] Road building cancelled\n");
        }
    }

    if (gamepad->IsButtonPressed(Input::GP_X)) {
        // In flag mode, X removes only the flag (road stays)
        if (m_currentLayer == World::Roads && m_mapEditor->GetRoadBuildState() == Editor::ROAD_FLAG) {
            // If flag exists at cursor, ToggleFlag removes it; otherwise no-op
            m_mapEditor->ToggleFlag(m_mapEditor->GetCursorTileX(), m_mapEditor->GetCursorTileY());
        } else {
            m_mapEditor->DeleteObjectAt(m_mapEditor->GetCursorTileX(), m_mapEditor->GetCursorTileY());
        }
    }

    if (gamepad->IsButtonPressed(Input::GP_A)) {
        if (m_currentLayer == World::Roads) {
            Editor::RoadBuildState rs = m_mapEditor->GetRoadBuildState();
            if (rs == Editor::ROAD_FLAG) {
                m_mapEditor->ToggleFlag(m_mapEditor->GetCursorTileX(), m_mapEditor->GetCursorTileY());
            } else if (rs == Editor::ROAD_IDLE) {
                int tx = m_mapEditor->GetCursorTileX();
                int ty = m_mapEditor->GetCursorTileY();
                m_mapEditor->StartRoad(tx, ty);
                if (m_mapEditor->GetRoadBuildState() == Editor::ROAD_PLACING) {
                    char buf[128];
                    sprintf_s(buf, "[Editor] Road started at (%d,%d), move cursor to set end point, A to confirm, B to cancel\n", tx, ty);
                    OutputDebugStringA(buf);
                }
            } else {
                m_mapEditor->CommitRoad();
            }
        } else if (m_currentLayer != World::Placement || m_editorMode == MODE_PLACEMENT) {
            if (m_currentLayer == World::Placement) {
                m_mapEditor->SetPlacementOccupied(m_activeWeight == World::Weight_Block);
            }
            // Reject placement if the selected building can't be placed
            if (m_currentLayer == World::Buildings && m_mapEditor) {
                std::tr1::shared_ptr<SpriteAtlas> ba = TextureRegistry::instance().getAtlas("Buildings");
                if (ba) {
                    const SpriteRegion* reg = ba->GetRegion((uint32_t)m_mapEditor->GetCurrentTileIndex());
                    if (reg && !CanPlaceBuilding(GetBuildingType(reg->name))) {
                        OutputDebugStringA("[EditorScene] Can't place this building (unique limit reached)\n");
                    } else {
                        m_mapEditor->PaintCurrentTile();
                    }
                }
            } else {
                m_mapEditor->PaintCurrentTile();
            }
        }
    }
}

void EditorScene::UpdateSaveLoadMenu(Input::Gamepad* gamepad, float deltaTime) {
    const int kMainItems = 4;     // Save, Load, Main Menu, Close
    const int kSlotItems = SAVE_SLOT_COUNT + 1; // 10 slots + Back

    // Left stick navigation (joystick) with input delay
    m_saveLoadMenuInputTimer += deltaTime;
    float stickX = 0.0f, stickY = 0.0f;
    gamepad->GetLeftStick(stickX, stickY);
    bool stickBeyondDeadzone = (stickY < -0.3f || stickY > 0.3f || stickX < -0.3f || stickX > 0.3f);
    bool stickAllowed = stickBeyondDeadzone && m_saveLoadMenuInputTimer >= 0.15f;

    bool stickUp = stickAllowed && (stickY < -0.3f);
    bool stickDown = stickAllowed && (stickY > 0.3f);
    bool stickLeft = stickAllowed && (stickX < -0.3f);
    bool stickRight = stickAllowed && (stickX > 0.3f);

    bool navUp = gamepad->IsButtonPressed(Input::GP_DPadUp) || stickUp;
    bool navDown = gamepad->IsButtonPressed(Input::GP_DPadDown) || stickDown;
    bool navLeft = gamepad->IsButtonPressed(Input::GP_DPadLeft) || stickLeft;
    bool navRight = gamepad->IsButtonPressed(Input::GP_DPadRight) || stickRight;

    switch (m_saveLoadMenuSection) {
    case 0: // Main menu
        if (navUp)
            m_saveLoadMenuSelection = (m_saveLoadMenuSelection - 1 + kMainItems) % kMainItems;
        if (navDown)
            m_saveLoadMenuSelection = (m_saveLoadMenuSelection + 1) % kMainItems;
        if (gamepad->IsButtonPressed(Input::GP_A)) {
            switch (m_saveLoadMenuSelection) {
            case 0: m_saveLoadMenuSection = 1; m_saveLoadMenuSelection = 0; break;
            case 1: m_saveLoadMenuSection = 2; m_saveLoadMenuSelection = 0; break;
            case 2: m_saveLoadMenuSection = 4; m_saveLoadMenuSelection = 0; break;
            case 3: m_saveLoadMenuActive = false; break;
            }
        }
        if (gamepad->IsButtonPressed(Input::GP_B))
            m_saveLoadMenuActive = false;
        break;

    case 1: // Save slots
        if (navUp) {
            m_saveLoadMenuSelection--;
            if (m_saveLoadMenuSelection < 0) m_saveLoadMenuSelection = kSlotItems - 1;
        }
        if (navDown)
            m_saveLoadMenuSelection = (m_saveLoadMenuSelection + 1) % kSlotItems;
        if (gamepad->IsButtonPressed(Input::GP_A)) {
            if (m_saveLoadMenuSelection < SAVE_SLOT_COUNT) {
                m_saveLoadMenuPendingSlot = m_saveLoadMenuSelection;
                char slotPath[256];
                sprintf_s(slotPath, "game:\\Media\\Maps\\slot_%02d.bin", m_saveLoadMenuSelection + 1);
                HANDLE hCheck = CreateFileA(slotPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                bool slotExists = (hCheck != INVALID_HANDLE_VALUE);
                if (slotExists) CloseHandle(hCheck);
                if (slotExists) {
                    m_saveLoadMenuSection = 3; // confirm overwrite
                    m_saveLoadMenuSelection = 0; // 0=Yes, 1=No
                } else {
                    m_mapEditor->SaveMap(slotPath);
                    m_saveLoadMenuSelection = 0;
                }
            } else {
                m_saveLoadMenuSection = 0;
                m_saveLoadMenuSelection = 1; // "Load" is at index 1
            }
        }
        if (gamepad->IsButtonPressed(Input::GP_B)) {
            m_saveLoadMenuSection = 0;
            m_saveLoadMenuSelection = 1;
        }
        break;

    case 2: // Load list
        {
            WIN32_FIND_DATAA ffd;
            HANDLE hFind = FindFirstFileA("game:\\Media\\Maps\\*.bin", &ffd);
            if (hFind == INVALID_HANDLE_VALUE) {
                if (gamepad->IsButtonPressed(Input::GP_A) || gamepad->IsButtonPressed(Input::GP_B)) {
                    m_saveLoadMenuSection = 0;
                    m_saveLoadMenuSelection = 2;
                }
                break;
            }

            struct { char path[260]; char name[64]; } files[SAVE_SLOT_COUNT];
            int fileCount = 0;
            do {
                if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && fileCount < SAVE_SLOT_COUNT) {
                    sprintf_s(files[fileCount].path, "game:\\Media\\Maps\\%s", ffd.cFileName);
                    strcpy_s(files[fileCount].name, ffd.cFileName);
                    fileCount++;
                }
            } while (FindNextFileA(hFind, &ffd) != 0);
            FindClose(hFind);

            int items = fileCount + 1;
            if (navUp) {
                m_saveLoadMenuSelection--;
                if (m_saveLoadMenuSelection < 0) m_saveLoadMenuSelection = items - 1;
            }
            if (navDown)
                m_saveLoadMenuSelection = (m_saveLoadMenuSelection + 1) % items;
            if (gamepad->IsButtonPressed(Input::GP_A)) {
                if (m_saveLoadMenuSelection < fileCount) {
                    m_mapEditor->LoadMap(files[m_saveLoadMenuSelection].path);
                    m_saveLoadMenuActive = false;
                } else {
                    m_saveLoadMenuSection = 0;
                    m_saveLoadMenuSelection = 2;
                }
            }
            if (gamepad->IsButtonPressed(Input::GP_B)) {
                m_saveLoadMenuSection = 0;
                m_saveLoadMenuSelection = 2;
            }
        }
        break;

    case 3: // Confirm overwrite
        if (navLeft || navRight)
            m_saveLoadMenuSelection = 1 - m_saveLoadMenuSelection;
        if (gamepad->IsButtonPressed(Input::GP_A)) {
            if (m_saveLoadMenuSelection == 0) {
                char slotPath[256];
                sprintf_s(slotPath, "game:\\Media\\Maps\\slot_%02d.bin", m_saveLoadMenuPendingSlot + 1);
                m_mapEditor->SaveMap(slotPath);
            }
            m_saveLoadMenuSection = 1;
            m_saveLoadMenuSelection = 0;
        }
        if (gamepad->IsButtonPressed(Input::GP_B)) {
            m_saveLoadMenuSection = 1;
            m_saveLoadMenuSelection = 0;
        }
        break;

    case 4: // Confirm exit to MenuScene
        if (navLeft || navRight)
            m_saveLoadMenuSelection = 1 - m_saveLoadMenuSelection;
        if (gamepad->IsButtonPressed(Input::GP_A)) {
            if (m_saveLoadMenuSelection == 0) { // Yes
                RequestSceneSwitch("MenuScene");
            } else { // No
                m_saveLoadMenuSection = 0;
                m_saveLoadMenuSelection = 2;
            }
        }
        if (gamepad->IsButtonPressed(Input::GP_B)) {
            m_saveLoadMenuSection = 0;
            m_saveLoadMenuSelection = 2;
        }
        break;
    }

    if (stickUp || stickDown || stickLeft || stickRight) {
        m_saveLoadMenuInputTimer = 0.0f;
    }
}

void EditorScene::RenderSaveLoadMenu(Graphics::RenderQueue* renderQueue) {
    if (!m_saveLoadMenuActive || !m_textManager) return;

    // Draw editor_menu sprite as decoration
    {
        TextureRegistry& reg = TextureRegistry::instance();
        std::tr1::shared_ptr<SpriteAtlas> uiAtl = reg.getAtlas("ui");
        if (uiAtl) {
            uint32_t menuIdx = uiAtl->GetIndex("editor_menu");
            if (menuIdx != 0xFFFFFFFF) {
                const SpriteRegion* menuReg = uiAtl->GetRegion(menuIdx);
                if (menuReg) {
                    Graphics::RenderCommandBuilder()
                        .UIElement(440.0f, 60.0f, 400.0f, 400.0f,
                            menuReg->u0, menuReg->v0, menuReg->u1, menuReg->v1,
                            14, 199)
                        .Submit(renderQueue);
                }
            }
        }
    }

    float startX = 580.0f;
    float startY = 175.0f;
    float lineH = 50.0f;

    D3DCOLOR titleColor = 0xFFFFCC00;
    D3DCOLOR normalColor = 0xFFFFFFFF;
    D3DCOLOR selColor = 0xFF00FF00;
    D3DCOLOR emptyColor = 0xFF888888;

    if (m_saveLoadMenuSection == 0) {
        // Main menu
        m_textManager->DrawTextToScreen("MENU", startX, startY, titleColor, 0.30f);
        const char* items[] = { "Save", "Load", "Main Menu", "Close" };
        for (int i = 0; i < 4; ++i) {
            float y = startY + 60.0f + i * lineH;
            D3DCOLOR c = (i == m_saveLoadMenuSelection) ? selColor : normalColor;
            m_textManager->DrawTextToScreen(items[i], startX - 40, y, c, 0.25f);
        }

    } else if (m_saveLoadMenuSection == 1) {
        // Save slots
        m_textManager->DrawTextToScreen("SAVE", startX, startY, titleColor, 0.30f);
        for (int i = 0; i < SAVE_SLOT_COUNT; ++i) {
            char slotPath[256];
            sprintf_s(slotPath, "game:\\Media\\Maps\\slot_%02d.bin", i + 1);
            HANDLE hCheck = CreateFileA(slotPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            bool exists = (hCheck != INVALID_HANDLE_VALUE);
            if (exists) CloseHandle(hCheck);
            char display[128];
            if (exists) {
                sprintf_s(display, "Slot %02d: %s", i + 1, slotPath + strlen("game:\\Media\\Maps\\"));
            } else {
                sprintf_s(display, "Slot %02d", i + 1);
            }
            float y = startY + 65.0f + i * lineH;
            D3DCOLOR c = (i == m_saveLoadMenuSelection) ? selColor : (exists ? normalColor : emptyColor);
            m_textManager->DrawTextToScreen(display, startX - 40.0f, y, c, 0.20f);
        }
        // Back button
        {
            float y = startY + 60.0f + SAVE_SLOT_COUNT * lineH;
            D3DCOLOR c = (SAVE_SLOT_COUNT == m_saveLoadMenuSelection) ? selColor : normalColor;
            m_textManager->DrawTextToScreen("Back", startX + 20.0f, y, c, 0.25f);
        }

    } else if (m_saveLoadMenuSection == 2) {
        // Load list
        m_textManager->DrawTextToScreen("LOAD", startX, startY, titleColor, 0.30f);

        WIN32_FIND_DATAA ffd;
        HANDLE hFind = FindFirstFileA("game:\\Media\\Maps\\*.bin", &ffd);
        if (hFind == INVALID_HANDLE_VALUE) {
            m_textManager->DrawTextToScreen("No saves found", startX + 20.0f, startY + 50.0f, emptyColor, 0.25f);
            m_textManager->DrawTextToScreen("Press A or B to go back", startX + 20.0f, startY + 90.0f, normalColor, 0.20f);
        } else {
            struct { char name[64]; } files[SAVE_SLOT_COUNT];
            int fileCount = 0;
            do {
                if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && fileCount < SAVE_SLOT_COUNT) {
                    strcpy_s(files[fileCount].name, ffd.cFileName);
                    fileCount++;
                }
            } while (FindNextFileA(hFind, &ffd) != 0);
            FindClose(hFind);

            for (int i = 0; i < fileCount; ++i) {
                float y = startY + 50.0f + i * lineH;
                D3DCOLOR c = (i == m_saveLoadMenuSelection) ? selColor : normalColor;
                m_textManager->DrawTextToScreen(files[i].name, startX - 40.0f, y, c, 0.20f);
            }
            // Back
            {
                float y = startY + 65.0f + fileCount * lineH;
                D3DCOLOR c = (fileCount == m_saveLoadMenuSelection) ? selColor : normalColor;
                m_textManager->DrawTextToScreen("Back", startX + 20.0f, y, c, 0.25f);
            }
        }

    } else if (m_saveLoadMenuSection == 3) {
        // Confirm overwrite
        char buf[128];
        sprintf_s(buf, "Overwrite slot %02d?", m_saveLoadMenuPendingSlot + 1);
        m_textManager->DrawTextToScreen(buf, startX, startY + 50.0f, titleColor, 0.30f);
        m_textManager->DrawTextToScreen("Yes", startX + 40.0f, startY + 110.0f, m_saveLoadMenuSelection == 0 ? selColor : normalColor, 0.25f);
        m_textManager->DrawTextToScreen("No", startX + 200.0f, startY + 110.0f, m_saveLoadMenuSelection == 1 ? selColor : normalColor, 0.25f);
    } else if (m_saveLoadMenuSection == 4) {
        // Confirm exit
        m_textManager->DrawTextToScreen("Return to Menu?", startX, startY + 50.0f, titleColor, 0.30f);
        m_textManager->DrawTextToScreen("Yes", startX + 40.0f, startY + 110.0f, m_saveLoadMenuSelection == 0 ? selColor : normalColor, 0.25f);
        m_textManager->DrawTextToScreen("No", startX + 200.0f, startY + 110.0f, m_saveLoadMenuSelection == 1 ? selColor : normalColor, 0.25f);
    }
}

void EditorScene::ResetShaderState() {
    // Shader state resets every frame via SpriteRenderer::BeginFrame()
    // Texture slots are preserved — clearing them here would break
    // subsequent EditorScene tile rendering until they are re-registered.
    
    if (m_renderer && m_renderer->GetDevice()) {
        IDirect3DDevice9* dev = m_renderer->GetDevice();
        dev->SetTexture(0, NULL);
    }
}

bool EditorScene::HandleGridMenuResourceSelection(Input::Gamepad* gamepad) {
    if (!m_gridMenu || !m_gridMenu->IsVisible() || m_currentLayer != World::Resources)
        return false;

    bool hasSel = m_gridMenu->HasSelection();
    int selIdx = m_gridMenu->GetSelectedSpriteIndex();
    char dbg[256];
    sprintf_s(dbg, "[EditorScene] HandleSelection: hasSel=%d selIdx=%d showingGroups=%d groupIdx=%d\n",
        hasSel, selIdx, m_resourceMenuShowingGroups, m_resourceMenuGroupIndex);
    OutputDebugStringA(dbg);

    if (!hasSel)
        return false;
    if (selIdx < 0 || !m_mapEditor)
        return false;

    if (m_resourceMenuShowingGroups) {
        const std::tr1::shared_ptr<SpriteAtlas> uiAtl = TextureRegistry::instance().getAtlas("ui");
        if (!uiAtl)
            return false;
        const SpriteRegion* region = uiAtl->GetRegion(selIdx);
        if (!region)
            return false;
        for (int i = 0; i < kResourceMenuGroupCount; ++i) {
            if (region->name == kResourceMenuGroups[i].iconName) {
                m_resourceMenuGroupIndex = i;
                LoadResourceGroupResources(i);
                return true;
            }
        }
    } else {
        // Match selected icon against deposit icon names in the current group
        if (m_resourceMenuGroupIndex < 0 || m_resourceMenuGroupIndex >= kResourceMenuGroupCount)
            return false;
        const ResourceMenuGroupDef& group = kResourceMenuGroups[m_resourceMenuGroupIndex];
        const std::tr1::shared_ptr<SpriteAtlas> iconAtl = TextureRegistry::instance().getAtlas("Icon");
        if (!iconAtl)
            return false;
        const SpriteRegion* region = iconAtl->GetRegion(selIdx);
        if (!region)
            return false;
        for (int i = 0; i < group.count; ++i) {
            if (region->name == group.depositIconNames[i]) {
                World::ResourceType rt = group.resources[i];
                m_activeResourceType = rt;
                m_resourceAmount = World::GetDefaultResourceAmount(rt);
                m_currentState = STATE_PLACING;
                m_depositConfirmPending = false;
                m_depositBuildingSpriteIdx = -1;
                {
                    std::tr1::shared_ptr<SpriteAtlas> iconAtl = TextureRegistry::instance().getAtlas("Icon");
                    if (iconAtl) {
                        uint32_t bidx = iconAtl->GetIndex(group.depositIconNames[i]);
                        if (bidx != 0xFFFFFFFF)
                            m_depositBuildingSpriteIdx = (int)bidx;
                    }
                }
                m_gridMenu->Hide();
                return true;
            }
        }
    }
    return false;
}

void EditorScene::Render(Graphics::RenderQueue* renderQueue) {
    if (!m_mapEditor) return;

    if (m_mapEditor) {
        m_mapEditor->SetRenderQueue(renderQueue);
    }

    if (m_camera && m_shaderManager) {
        m_camera->Update();
        D3DXMATRIX viewProj = m_camera->GetViewMatrix() * m_camera->GetProjectionMatrix();
        m_shaderManager->SetFrameViewProj(&viewProj);
    }

    // Background via render queue (slot 10, LAYER_UI so it renders on top of world tiles)
    if (m_bgEditorTexture.GetTexture() && m_spriteRenderer) {
        m_spriteRenderer->SetTextureSlot(10, m_bgEditorTexture.GetTexture());
        Graphics::RenderCommandBuilder()
            .UIElement(0.0f, 0.0f, 1280.0f, 720.0f,
                0.0f, 0.0f, 1.0f, 1.0f,
                10, 255)
            .Submit(renderQueue);
    }
    // Render world content
    m_mapEditor->RenderGeometry();
    if (m_saveLoadMenuActive) {
        m_mapEditor->SetShowCursor(false);
    }
    m_mapEditor->RenderUI();

    // Render units after roads/flags
    if (m_unitManager) {
        m_unitManager->Render();
    }

    // ─── Render wildlife (animal sprites) ───────────────────────────
    if (m_wildlife && m_spriteRenderer) {
        const std::vector<World::Animal>& animals = m_wildlife->GetAllAnimals();
        if (!animals.empty()) {
            TextureRegistry& registry = TextureRegistry::instance();
            registry.getTextureOrLoad("Units");
            std::tr1::shared_ptr<SpriteAtlas> unitsAtlas = registry.getAtlas("Units");
            if (unitsAtlas && unitsAtlas->GetTexture()) {
                LPDIRECT3DTEXTURE9 unitsTex = unitsAtlas->GetTexture();
                m_spriteRenderer->SetTextureSlot(SLOT_UNITS, unitsTex);
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

    if (m_saveLoadMenuActive) {
        m_mapEditor->SetShowCursor(true);
    }

    if (m_textManager) {
        char fpsText[64];
        sprintf(fpsText, "FPS: %d", m_fps);
        m_textManager->DrawTextToScreen(fpsText, 650.0f, 720.0f - 60.0f, 0xFF00FF00, 0.25f);

        static const char* layerNames[] = {
            "Roads", "Nodes", "Placement", "Resources", "Ground", "Objects", "Overlay", "Buildings"
        };
        const char* layerName = "Unknown";
        int layerIdx = static_cast<int>(m_currentLayer);
        if (layerIdx >= 0 && layerIdx < World::LayerCount) {
            layerName = layerNames[layerIdx];
        }
        char layerText[64];
        sprintf(layerText, "Layer: %s", layerName);
        m_textManager->DrawTextToScreen(layerText, 200, 720.0f - 60.0f, 0xFFFFFFFF, 0.25f);

        // Show resource info when Resources layer is active
        if (m_currentLayer == World::Resources) {
            char resInfo[256];
            if (m_currentState == STATE_INPUT_AMOUNT) {
                if (World::IsDepositResource(m_activeResourceType)) {
                    sprintf_s(resInfo, "Deposit at (%d,%d): %s [%d]  D-pad: adjust  Y: save  B: cancel  X: remove",
                        m_phantomTileX, m_phantomTileY,
                        World::ResourceTypeToString(m_activeResourceType), m_resourceAmount);
                } else {
                    sprintf_s(resInfo, "Editing at (%d,%d): %s [%d]  D-pad: adjust  Y: save  B: cancel  X: remove",
                        m_phantomTileX, m_phantomTileY,
                        World::ResourceTypeToString(m_activeResourceType), m_resourceAmount);
                }
            } else if (m_currentState == STATE_PLACING) {
                sprintf_s(resInfo, "Place: %s [%d]  D-pad: adjust  A: place  X: remove  B: cancel",
                    World::ResourceTypeToString(m_activeResourceType), m_resourceAmount);
            } else if (m_activeResourceType != World::ResourceType_None) {
                sprintf_s(resInfo, "Resource: %s [%d]  D-pad: adjust  A: place  X: remove",
                    World::ResourceTypeToString(m_activeResourceType), m_resourceAmount);
            } else {
                sprintf_s(resInfo, "Press RB to select resource type, then A to place on tile. X: remove");
            }
            m_textManager->DrawTextToScreen(resInfo, 150.0f, 50.0f, 0xFF0000FF, 0.22f);
        }
    }

    // Render GridMenu (submits to queue via renderQueue)
    if (m_gridMenu) {
        m_gridMenu->SetRenderQueue(renderQueue);
        if (m_gridMenu->IsVisible()) {
            // Ensure UI textures are valid
            TextureRegistry::instance().refreshTexture("ui");

            // Unbind texture stages before UI rendering.
            // Do NOT set render target to NULL — that kills all subsequent draw calls.
            IDirect3DDevice9* dev = m_renderer->GetDevice();
            if (dev) {
                for (int i = 0; i < 4; ++i) {
                    dev->SetTexture(i, NULL);
                }
            }

            m_gridMenu->Render();
            // Show title above grid
            char titleText[64];
            if (m_currentLayer == World::Ground) {
                sprintf_s(titleText, "Ground");
            } else if (m_currentLayer == World::Objects) {
                sprintf_s(titleText, "%s", kObjectGroupNames[m_objectGroupIndex]);
            } else if (m_currentLayer == World::Buildings) {
                sprintf_s(titleText, "Buildings");
            } else {
                sprintf_s(titleText, "");
            }
            if (titleText[0]) {
                std::string translatedTitle = LanguageManager::instance().GetString(titleText);
                m_textManager->DrawTextCenteredToScreen(translatedTitle.c_str(), 640.0f, 50.0f, 0xFFFFFFFF, 0.35f);
            }
            
            // Show section info above grid cells
            char sectionText[64];
            sprintf_s(sectionText, "Section %d / %d", m_gridMenu->GetCurrentPage() + 1, m_gridMenu->GetTotalPages());
            m_textManager->DrawTextCenteredToScreen(sectionText, 640.0f, 600.0f, 0xFFFFFFFF, 0.20f);

            // Bottom-left: button_A + Select (cell start + 5px = 407)
            {
                Graphics::RenderCommandBuilder()
                    .UIElement(407.0f, 567.0f, 32.0f, 32.0f,
                        m_buttonAUV.u0, m_buttonAUV.v0, m_buttonAUV.u1, m_buttonAUV.v1,
                        13, 60)
                    .Submit(renderQueue);
            }
            m_textManager->DrawTextToScreen("Select", 424.0f, 563.0f, 0xFF44FF44, 0.22f);
            // Bottom-right: Close + button_B (cell end - 5px = 871, 15px gap from Close to B)
            m_textManager->DrawTextToScreen("Close", 784.0f, 563.0f, 0xFFFF4444, 0.22f);
            {
                Graphics::RenderCommandBuilder()
                    .UIElement(839.0f, 567.0f, 32.0f, 32.0f,
                        m_buttonBUV.u0, m_buttonBUV.v0, m_buttonBUV.u1, m_buttonBUV.v1,
                        13, 60)
                    .Submit(renderQueue);
            }
        }
    }

    // Render WeightMenu (submits to queue via renderQueue)
    if (m_weightMenu) {
        m_weightMenu->SetRenderQueue(renderQueue);
        if (m_weightMenu->IsVisible()) {
            // Ensure UI textures are valid
            TextureRegistry::instance().refreshTexture("ui");

            // Unbind texture stages before UI rendering.
            // Do NOT set render target to NULL — that kills all subsequent draw calls.
            IDirect3DDevice9* dev = m_renderer->GetDevice();
            if (dev) {
                for (int i = 0; i < 4; ++i) {
                    dev->SetTexture(i, NULL);
                }
            }

            m_weightMenu->Render();
        }
    }

    // Active sprite preview in top-left corner
    if (m_mapEditor && m_spriteRenderer && renderQueue) {
        int tileIdx = m_mapEditor->GetCurrentTileIndex();
        if (tileIdx >= 0) {
            TextureRegistry& reg = TextureRegistry::instance();
            const char* atlasName = "ground";
            WORD texSlot = 8;
            if (m_currentLayer == World::Objects) {
                atlasName = "maptiles";
                texSlot = 8;
            } else if (m_currentLayer == World::Roads) {
                atlasName = "streets";
                texSlot = 3;
            } else if (m_currentLayer == World::Buildings) {
                atlasName = "Buildings";
                texSlot = 1;
            }
            std::tr1::shared_ptr<SpriteAtlas> atlas = reg.getAtlas(atlasName);
            if (atlas && tileIdx < (int)atlas->GetRegionCount()) {
                const SpriteRegion* region = atlas->GetRegion(tileIdx);
                if (region) {
                    Graphics::RenderCommandBuilder()
                        .UIElement(10.0f, 40.0f, 64.0f, 64.0f,
                            region->u0, region->v0, region->u1, region->v1,
                            texSlot, 200)
                        .Submit(renderQueue);
                }
            }
        }
    }

    // Deposit preview: render building sprite at placed position during amount editing
    if (m_currentState == STATE_INPUT_AMOUNT && m_mapEditor && m_spriteRenderer && renderQueue
        && World::IsDepositResource(m_activeResourceType)) {
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        float wx, wy;
        coords.NodeTileToWorld(m_phantomTileX, m_phantomTileY, wx, wy);

        std::tr1::shared_ptr<SpriteAtlas> iconAtl = TextureRegistry::instance().getAtlas("Icon");
        if (iconAtl && m_depositBuildingSpriteIdx >= 0 && m_depositBuildingSpriteIdx < (int)iconAtl->GetRegionCount()) {
            const SpriteRegion* region = iconAtl->GetRegion(m_depositBuildingSpriteIdx);
            if (region) {
                float previewW = (float)region->width;
                float previewH = (float)region->height;

                if (m_spriteRenderer)
                    m_spriteRenderer->SetTextureSlot(12, iconAtl->GetTexture());

                Graphics::RenderCommandBuilder()
                    .WorldSprite(wx - region->pivotX, wy - region->pivotY,
                        previewW, previewH,
                        region->u0, region->v0, region->u1, region->v1,
                        12, static_cast<WORD>(0.99f * 65535.0f))
                    .Submit(renderQueue);
            }
        }
    }

    // Save/Load menu overlay (on top of everything)
    RenderSaveLoadMenu(renderQueue);
}

void EditorScene::RenderOverlay() {
    if (!m_radialMenu || !m_radialMenu->IsVisible()) return;
    if (!m_renderer || !m_shaderManager || !m_spriteRenderer) return;

    m_radialMenu->Render();

    LPDIRECT3DDEVICE9 dev2 = m_renderer->GetDevice();
    if (!dev2) return;

    m_radialMenu->RenderIconsDirect(dev2, m_shaderManager, m_spriteRenderer->GetVertexDeclaration());
}

void EditorScene::OnEnter() {
}

void EditorScene::OnExit() {
    OutputDebugStringA("[EditorScene] OnExit - cleaning up resources\n");

    if (m_mapEditor) {
        delete m_mapEditor;
        m_mapEditor = nullptr;
    }
    
    m_saveLoadMenuActive = false;
    m_hasSelection = false;
    
    // Сбрасываем флаг загрузки
    m_loaded = false; 
}

void EditorScene::BindGridMenuTextures(LPDIRECT3DTEXTURE9 bgTexture, LPDIRECT3DTEXTURE9 cellTexture, LPDIRECT3DTEXTURE9 atlasTexture)
{
    if (m_gridMenu) {
        TextureRegistry& reg = TextureRegistry::instance();
        std::tr1::shared_ptr<SpriteAtlas> uiAtl = reg.getAtlas("ui");
        LPDIRECT3DTEXTURE9 uiTex = uiAtl ? uiAtl->GetTexture() : NULL;
        m_gridMenu->SetTextures(uiTex, cellTexture, atlasTexture);
        if (m_spriteRenderer) {
            m_spriteRenderer->SetTextureSlot(6, uiTex);
            m_spriteRenderer->SetTextureSlot(7, cellTexture);
            m_spriteRenderer->SetTextureSlot(8, atlasTexture);
        }
        OutputDebugStringA("[EditorScene] BindGridMenuTextures called\n");
    } else {
        OutputDebugStringA("[EditorScene] BindGridMenuTextures called but GridMenu not initialized\n");
    }
}

bool EditorScene::CanPlaceBuilding(World::BuildingType type) const {
    if (type == World::Storehouse && HasTownHall()) return false;
    return true;
}


bool EditorScene::HasTownHall() const {
    if (!m_mapEditor || !m_mapEditor->GetMap())
        return false;
    World::TileLayer* buildingsLayer = m_mapEditor->GetMap()->GetLayer(World::Buildings);
    if (!buildingsLayer)
        return false;
    std::tr1::shared_ptr<SpriteAtlas> atlas = TextureRegistry::instance().getAtlas("Buildings");
    if (!atlas)
        return false;
    for (int y = 0; y < buildingsLayer->GetHeight(); ++y) {
        for (int x = 0; x < buildingsLayer->GetWidth(); ++x) {
            const World::Tile& tile = buildingsLayer->GetTile(x, y);
            if (tile.regionIndex >= 0 && tile.atlasName == "Buildings") {
                const SpriteRegion* tr = atlas->GetRegion(tile.regionIndex);
                if (tr && tr->name == "b_townhall")
                    return true;
            }
        }
    }
    return false;
}



void EditorScene::LoadGridMenuAtlas(const char* atlasName) {
    if (!m_gridMenu) return;

    TextureRegistry& registry = TextureRegistry::instance();
    std::tr1::shared_ptr<SpriteAtlas> atlas = registry.getAtlas(atlasName);
    if (atlas) {
        LPDIRECT3DTEXTURE9 atlasTex = atlas->GetTexture();
        std::tr1::shared_ptr<SpriteAtlas> uiAtl = registry.getAtlas("ui");
        LPDIRECT3DTEXTURE9 uiTex = uiAtl ? uiAtl->GetTexture() : NULL;
        m_gridMenu->SetTextures(uiTex, uiTex, atlasTex);
        m_gridMenu->SetIconAtlas(atlas);
        if (m_spriteRenderer) {
            m_spriteRenderer->SetTextureSlot(6, uiTex);
            m_spriteRenderer->SetTextureSlot(7, uiTex);
            m_spriteRenderer->SetTextureSlot(8, atlasTex);
        }

        // Set bg/cell UV from UI atlas (menu_Grid, menu_cell1)
        GridMenu::TileUV bgUV = {0,0,1,1}, cellUV = {0,0,1,1};
        if (uiAtl) {
            uint32_t bgIdx = uiAtl->GetIndex("menu_Grid");
            if (bgIdx != 0xFFFFFFFF) {
                const SpriteRegion* reg = uiAtl->GetRegion(bgIdx);
                if (reg) { bgUV.u0 = reg->u0; bgUV.v0 = reg->v0; bgUV.u1 = reg->u1; bgUV.v1 = reg->v1; }
                else { OutputDebugStringA("[EditorScene LoadGridMenuAtlas] WARNING: 'menu_Grid' found but GetRegion NULL\n"); }
            } else {
                OutputDebugStringA("[EditorScene LoadGridMenuAtlas] WARNING: 'menu_Grid' NOT FOUND\n");
            }
            uint32_t cellIdx = uiAtl->GetIndex("menu_cell1");
            if (cellIdx != 0xFFFFFFFF) {
                const SpriteRegion* reg = uiAtl->GetRegion(cellIdx);
                if (reg) { cellUV.u0 = reg->u0; cellUV.v0 = reg->v0; cellUV.u1 = reg->u1; cellUV.v1 = reg->v1; }
                else { OutputDebugStringA("[EditorScene LoadGridMenuAtlas] WARNING: 'menu_cell1' found but GetRegion NULL\n"); }
            } else {
                OutputDebugStringA("[EditorScene LoadGridMenuAtlas] WARNING: 'menu_cell1' NOT FOUND\n");
            }
        }
    m_gridMenu->SetBackgroundUV(bgUV);
    m_gridMenu->SetCellUV(cellUV);
    m_gridMenu->SetCellSpacing(139.0f, 94.0f);
    m_gridMenu->SetCellVisualSize(117.0f, 72.0f);
    m_gridMenu->SetCellPadding(15.0f);

    std::vector<GridMenu::TileUV> uvs;
    std::vector<int> globalIndices;
    std::vector<std::string> labels;
        uvs.reserve(atlas->GetRegionCount());
        globalIndices.reserve(atlas->GetRegionCount());
        labels.reserve(atlas->GetRegionCount());
        for (uint32_t i = 0; i < atlas->GetRegionCount(); ++i) {
            const SpriteRegion* reg = atlas->GetRegion(i);
            GridMenu::TileUV tu;
            if (reg) { tu.u0 = reg->u0; tu.v0 = reg->v0; tu.u1 = reg->u1; tu.v1 = reg->v1; }
            else { tu.u0 = 0.0f; tu.v0 = 0.0f; tu.u1 = 1.0f; tu.v1 = 1.0f; }
            uvs.push_back(tu);
            globalIndices.push_back((int)i);
            std::string label;
            if (reg && !reg->name.empty())
                label = LanguageManager::instance().GetString(reg->name);
            labels.push_back(label);
        }

        // Filter out unique buildings that can't be placed (e.g. town hall)
        if (strcmp(atlasName, "Buildings") == 0) {
            for (int i = (int)globalIndices.size() - 1; i >= 0; --i) {
                const SpriteRegion* reg = atlas->GetRegion((uint32_t)globalIndices[i]);
                    if (reg && !CanPlaceBuilding(GetBuildingType(reg->name))) {

                    uvs.erase(uvs.begin() + i);
                    globalIndices.erase(globalIndices.begin() + i);
                    labels.erase(labels.begin() + i);
                    char dbg[256];
                    sprintf_s(dbg, "[EditorScene] Hiding '%s' from Buildings menu\n", reg->name.c_str());
                    OutputDebugStringA(dbg);
                }
            }
        }

        m_gridMenu->SetTileData(uvs, globalIndices);
        m_gridMenu->SetSpriteRenderer(m_spriteRenderer);
        m_gridMenu->ResetSelection();
        m_gridMenu->SetCellLabels(labels);

        char buf[128];
        sprintf_s(buf, "[EditorScene] Loaded atlas '%s' into GridMenu (%d regions)\n", atlasName, (int)atlas->GetRegionCount());
        OutputDebugStringA(buf);
    } else {
        char buf[128];
        sprintf_s(buf, "[EditorScene] Atlas '%s' not found!\n", atlasName);
        OutputDebugStringA(buf);
    }
}

void EditorScene::LoadGridMenuGroup(const char* groupName) {
    if (!m_gridMenu) return;

    TextureRegistry& registry = TextureRegistry::instance();
    std::tr1::shared_ptr<SpriteAtlas> maptiles = registry.getAtlas("maptiles");
    if (!maptiles) {
        char buf[128];
        sprintf_s(buf, "[EditorScene] maptiles atlas not found for group '%s'\n", groupName);
        OutputDebugStringA(buf);
        return;
    }

    LPDIRECT3DTEXTURE9 atlasTex = maptiles->GetTexture();
    std::tr1::shared_ptr<SpriteAtlas> uiAtl = registry.getAtlas("ui");
    LPDIRECT3DTEXTURE9 uiTex = uiAtl ? uiAtl->GetTexture() : NULL;
    m_gridMenu->SetTextures(uiTex, uiTex, atlasTex);
    m_gridMenu->SetIconAtlas(maptiles);
    if (m_spriteRenderer) {
        m_spriteRenderer->SetTextureSlot(6, uiTex);
        m_spriteRenderer->SetTextureSlot(7, uiTex);
        m_spriteRenderer->SetTextureSlot(8, atlasTex);
    }

    // Set bg/cell UV from UI atlas (menu_Grid, menu_cell1)
    GridMenu::TileUV bgUV = {0,0,1,1}, cellUV = {0,0,1,1};
    if (uiAtl) {
        uint32_t bgIdx = uiAtl->GetIndex("menu_Grid");
        if (bgIdx != 0xFFFFFFFF) {
            const SpriteRegion* reg = uiAtl->GetRegion(bgIdx);
            if (reg) { bgUV.u0 = reg->u0; bgUV.v0 = reg->v0; bgUV.u1 = reg->u1; bgUV.v1 = reg->v1; }
            else { OutputDebugStringA("[EditorScene LoadGridMenuGroup] WARNING: 'menu_Grid' found but GetRegion NULL\n"); }
        } else {
            OutputDebugStringA("[EditorScene LoadGridMenuGroup] WARNING: 'menu_Grid' NOT FOUND\n");
        }
        uint32_t cellIdx = uiAtl->GetIndex("menu_cell1");
        if (cellIdx != 0xFFFFFFFF) {
            const SpriteRegion* reg = uiAtl->GetRegion(cellIdx);
            if (reg) { cellUV.u0 = reg->u0; cellUV.v0 = reg->v0; cellUV.u1 = reg->u1; cellUV.v1 = reg->v1; }
            else { OutputDebugStringA("[EditorScene LoadGridMenuGroup] WARNING: 'menu_cell1' found but GetRegion NULL\n"); }
        } else {
            OutputDebugStringA("[EditorScene LoadGridMenuGroup] WARNING: 'menu_cell1' NOT FOUND\n");
        }
    }
    m_gridMenu->SetBackgroundUV(bgUV);
    m_gridMenu->SetCellUV(cellUV);
    m_gridMenu->SetCellSpacing(139.0f, 94.0f);
    m_gridMenu->SetCellVisualSize(117.0f, 72.0f);
    m_gridMenu->SetCellPadding(15.0f);

    const std::vector<uint32_t>* groupIndices = maptiles->GetGroup(groupName);
    std::vector<GridMenu::TileUV> uvs;
    std::vector<int> globalIndices;
    if (groupIndices) {
        uvs.reserve(groupIndices->size());
        globalIndices.reserve(groupIndices->size());
        for (uint32_t i = 0; i < groupIndices->size(); ++i) {
            uint32_t spriteIdx = (*groupIndices)[i];
            const SpriteRegion* reg = maptiles->GetRegion(spriteIdx);
            GridMenu::TileUV tu;
            if (reg) {
                tu.u0 = reg->u0; tu.v0 = reg->v0; tu.u1 = reg->u1; tu.v1 = reg->v1;
            } else {
                tu.u0 = 0.0f; tu.v0 = 0.0f; tu.u1 = 1.0f; tu.v1 = 1.0f;
            }
            uvs.push_back(tu);
            globalIndices.push_back((int)spriteIdx);
        }
    }
    m_gridMenu->SetTileData(uvs, globalIndices);
    m_gridMenu->SetSpriteRenderer(m_spriteRenderer);
    m_gridMenu->ResetSelection();

    // Build cell labels from maptiles region names via language.ini
    {
        std::vector<std::string> labels;
        labels.reserve(uvs.size());
        for (size_t i = 0; i < uvs.size(); ++i) {
            uint32_t spriteIdx = (uint32_t)globalIndices[i];
            const SpriteRegion* reg = maptiles->GetRegion(spriteIdx);
            std::string label;
            if (reg && !reg->name.empty())
                label = LanguageManager::instance().GetString(reg->name);
            labels.push_back(label);
        }
        m_gridMenu->SetCellLabels(labels);
    }

    char buf[128];
    sprintf_s(buf, "[EditorScene] Loaded group '%s' from maptiles (%d sprites)\n", groupName, (int)uvs.size());
    OutputDebugStringA(buf);
}



void EditorScene::LoadResourceIcons() {
    LoadResourceGroupIcons();
}

void EditorScene::LoadUIAtlasGroup(const char* groupName) {
    std::vector<std::string> emptyFilters;
    LoadUIAtlasGroup(groupName, emptyFilters);
}

void EditorScene::LoadUIAtlasGroup(const char* groupName, const std::vector<std::string>& filterNames) {
    if (!m_gridMenu) return;

    TextureRegistry& registry = TextureRegistry::instance();
    std::tr1::shared_ptr<SpriteAtlas> uiAtl = registry.getAtlas("ui");
    if (!uiAtl) {
        OutputDebugStringA("[EditorScene] UI atlas not found\n");
        return;
    }

    // Устанавливаем текстуры для GridMenu
    LPDIRECT3DTEXTURE9 uiTex = uiAtl->GetTexture();
    m_gridMenu->SetTextures(uiTex, uiTex, uiTex);
    m_gridMenu->SetIconAtlas(uiAtl);

    if (m_spriteRenderer) {
        m_spriteRenderer->SetTextureSlot(6, uiTex);  // background
        m_spriteRenderer->SetTextureSlot(7, uiTex);  // cell
        m_spriteRenderer->SetTextureSlot(8, uiTex);  // atlas/icons
    }

    // Загружаем UV координаты для фона и ячеек
    GridMenu::TileUV bgUV = {0,0,1,1}, cellUV = {0,0,1,1};
    uint32_t bgIdx = uiAtl->GetIndex("menu_Grid");
    if (bgIdx != 0xFFFFFFFF) {
        const SpriteRegion* reg = uiAtl->GetRegion(bgIdx);
        if (reg) { 
            bgUV.u0 = reg->u0; bgUV.v0 = reg->v0; 
            bgUV.u1 = reg->u1; bgUV.v1 = reg->v1; 
        }
    }
    
    uint32_t cellIdx = uiAtl->GetIndex("menu_cell1");
    if (cellIdx != 0xFFFFFFFF) {
        const SpriteRegion* reg = uiAtl->GetRegion(cellIdx);
        if (reg) { 
            cellUV.u0 = reg->u0; cellUV.v0 = reg->v0; 
            cellUV.u1 = reg->u1; cellUV.v1 = reg->v1; 
        }
    }
    
    m_gridMenu->SetBackgroundUV(bgUV);
    m_gridMenu->SetCellUV(cellUV);

    // Собираем иконки по фильтру
    std::vector<GridMenu::TileUV> uvs;
    std::vector<int> globalIndices;
    std::vector<std::string> labels;

    for (size_t i = 0; i < filterNames.size(); ++i) {
        const std::string& iconName = filterNames[i];
        uint32_t spriteIdx = uiAtl->GetIndex(iconName.c_str());
        
        if (spriteIdx == 0xFFFFFFFF) {
            char buf[128];
            sprintf_s(buf, "[EditorScene] Icon '%s' not found in UI atlas\n", iconName.c_str());
            OutputDebugStringA(buf);
            continue;
        }

        const SpriteRegion* reg = uiAtl->GetRegion(spriteIdx);
        if (!reg) continue;

        GridMenu::TileUV tu = {reg->u0, reg->v0, reg->u1, reg->v1};
        uvs.push_back(tu);
        globalIndices.push_back((int)spriteIdx);
        labels.push_back(reg->name.empty() ? "" : LanguageManager::instance().GetString(reg->name));
    }

    if (uvs.empty()) {
        OutputDebugStringA("[EditorScene] No matching icons found in UI atlas\n");
        return;
    }

    m_gridMenu->SetTileData(uvs, globalIndices);
    m_gridMenu->SetCellLabels(labels);
    m_gridMenu->SetSpriteRenderer(m_spriteRenderer);
    m_gridMenu->ResetSelection();
    
    // Устанавливаем параметры отображения
    m_gridMenu->SetCellSpacing(139.0f, 94.0f);
    m_gridMenu->SetCellVisualSize(117.0f, 72.0f);
    m_gridMenu->SetCellPadding(15.0f);
}

void EditorScene::LoadResourceGroupIcons() {
    if (!m_gridMenu) return;

    TextureRegistry& registry = TextureRegistry::instance();
    std::tr1::shared_ptr<SpriteAtlas> uiAtl = registry.getAtlas("ui");
    if (!uiAtl) {
        OutputDebugStringA("[EditorScene] UI atlas not found for resource icons\n");
        return;
    }

    LPDIRECT3DTEXTURE9 uiTex = uiAtl->GetTexture();
    m_gridMenu->SetTextures(uiTex, uiTex, uiTex);
    if (m_spriteRenderer) {
        m_spriteRenderer->SetTextureSlot(6, uiTex);
        m_spriteRenderer->SetTextureSlot(7, uiTex);
        m_spriteRenderer->SetTextureSlot(8, uiTex);
    }

    GridMenu::TileUV bgUV = {0,0,1,1}, cellUV = {0,0,1,1};
    if (uiAtl) {
        uint32_t bgIdx = uiAtl->GetIndex("menu_Grid");
        if (bgIdx != 0xFFFFFFFF) {
            const SpriteRegion* reg = uiAtl->GetRegion(bgIdx);
            if (reg) { bgUV.u0 = reg->u0; bgUV.v0 = reg->v0; bgUV.u1 = reg->u1; bgUV.v1 = reg->v1; }
        }
        uint32_t cellIdx = uiAtl->GetIndex("menu_cell1");
        if (cellIdx != 0xFFFFFFFF) {
            const SpriteRegion* reg = uiAtl->GetRegion(cellIdx);
            if (reg) { cellUV.u0 = reg->u0; cellUV.v0 = reg->v0; cellUV.u1 = reg->u1; cellUV.v1 = reg->v1; }
        }
    }
    m_gridMenu->SetBackgroundUV(bgUV);
    m_gridMenu->SetCellUV(cellUV);
    m_gridMenu->SetCellSpacing(119.0f, 100.0f);

    std::vector<GridMenu::TileUV> uvs;
    std::vector<int> globalIndices;
    std::vector<std::string> groupLabels;

    for (int i = 0; i < kResourceMenuGroupCount; ++i) {
        uint32_t spriteIdx = uiAtl->GetIndex(kResourceMenuGroups[i].iconName);
        if (spriteIdx == 0xFFFFFFFF) {
            char buf[128];
            sprintf_s(buf, "[EditorScene] Resource group icon '%s' not found in UI atlas\n",
                kResourceMenuGroups[i].iconName);
            OutputDebugStringA(buf);
            continue;
        }

        const SpriteRegion* reg = uiAtl->GetRegion(spriteIdx);
        GridMenu::TileUV tu;
        if (reg) {
            tu.u0 = reg->u0; tu.v0 = reg->v0; tu.u1 = reg->u1; tu.v1 = reg->v1;
        } else {
            tu.u0 = 0.0f; tu.v0 = 0.0f; tu.u1 = 1.0f; tu.v1 = 1.0f;
        }
        uvs.push_back(tu);
        globalIndices.push_back((int)spriteIdx);
        static const char* kGroupDisplayNames[] = { "Wood", "Stone", "Ore", "Food" };
        groupLabels.push_back((i < 4) ? kGroupDisplayNames[i] : "");
    }

    m_gridMenu->SetTileData(uvs, globalIndices);
    m_gridMenu->SetCellLabels(groupLabels);
    m_gridMenu->SetSpriteRenderer(m_spriteRenderer);
    m_gridMenu->ResetSelection();
    m_resourceMenuShowingGroups = true;
    m_resourceMenuGroupIndex = -1;

    char buf[128];
    sprintf_s(buf, "[EditorScene] Loaded resource groups into GridMenu (%d sprites)\n", (int)uvs.size());
    OutputDebugStringA(buf);
}

void EditorScene::LoadResourceGroupResources(int groupIndex) {
    if (!m_gridMenu) return;
    if (groupIndex < 0 || groupIndex >= kResourceMenuGroupCount) return;

    TextureRegistry& registry = TextureRegistry::instance();
    std::tr1::shared_ptr<SpriteAtlas> uiAtl = registry.getAtlas("ui");
    std::tr1::shared_ptr<SpriteAtlas> iconAtl = registry.getAtlas("Icon");
    if (!uiAtl || !iconAtl) return;

    LPDIRECT3DTEXTURE9 uiTex = uiAtl->GetTexture();
    LPDIRECT3DTEXTURE9 iconTex = iconAtl->GetTexture();

    // Background and cells from UI atlas, icons from Icon atlas
    m_gridMenu->SetTextures(uiTex, uiTex, iconTex);
    if (m_spriteRenderer) {
        m_spriteRenderer->SetTextureSlot(6, uiTex);   // background menu_Grid
        m_spriteRenderer->SetTextureSlot(7, uiTex);   // cell menu_cell1
        m_spriteRenderer->SetTextureSlot(8, iconTex); // icons r_xxx
    }

    GridMenu::TileUV cellUV = {0,0,1,1};
    uint32_t cellIdx = uiAtl->GetIndex("menu_cell1");
    if (cellIdx != 0xFFFFFFFF) {
        const SpriteRegion* reg = uiAtl->GetRegion(cellIdx);
        if (reg) { cellUV.u0 = reg->u0; cellUV.v0 = reg->v0; cellUV.u1 = reg->u1; cellUV.v1 = reg->v1; }
    }
    m_gridMenu->SetCellUV(cellUV);

    std::vector<GridMenu::TileUV> uvs;
    std::vector<int> globalIndices;
    std::vector<std::string> itemLabels;
    const ResourceMenuGroupDef& group = kResourceMenuGroups[groupIndex];

    for (int i = 0; i < group.count; ++i) {
        World::ResourceType rt = group.resources[i];
        const char* depositName = group.depositIconNames[i];
        if (!depositName || !depositName[0]) continue;

        uint32_t spriteIdx = iconAtl->GetIndex(depositName);
        if (spriteIdx == 0xFFFFFFFF) {
            char buf[128];
            sprintf_s(buf, "[EditorScene] Deposit icon '%s' not found in Icon atlas\n", depositName);
            OutputDebugStringA(buf);
            continue;
        }

        const SpriteRegion* reg = iconAtl->GetRegion(spriteIdx);
        GridMenu::TileUV tu;
        if (reg) {
            tu.u0 = reg->u0; tu.v0 = reg->v0; tu.u1 = reg->u1; tu.v1 = reg->v1;
        } else {
            tu.u0 = 0.0f; tu.v0 = 0.0f; tu.u1 = 1.0f; tu.v1 = 1.0f;
        }
        uvs.push_back(tu);
        globalIndices.push_back((int)spriteIdx);
        const char* nameStr = World::ResourceTypeToString(rt);
        itemLabels.push_back(nameStr ? nameStr : "");
    }

    m_gridMenu->SetTileData(uvs, globalIndices);
    m_gridMenu->SetCellLabels(itemLabels);
    m_gridMenu->SetSpriteRenderer(m_spriteRenderer);
    m_gridMenu->ResetSelection();
    m_resourceMenuShowingGroups = false;
    m_resourceMenuGroupIndex = groupIndex;

    char buf[160];
    sprintf_s(buf, "[EditorScene] Loaded resource subgroup '%s' into GridMenu (%d sprites)\n",
        group.iconName, (int)uvs.size());
    OutputDebugStringA(buf);
}

void EditorScene::CycleObjectGroup() {
    m_objectGroupIndex = (m_objectGroupIndex + 1) % kObjectGroupCount;
    const char* groupName = kObjectGroupNames[m_objectGroupIndex];
    LoadGridMenuGroup(groupName);
    if (m_mapEditor) {
        m_mapEditor->SetObjectGroup(groupName);
    }
    char buf[128];
    sprintf_s(buf, "[EditorScene] CycleObjectGroup: index=%d, group='%s'\n", m_objectGroupIndex, groupName);
    OutputDebugStringA(buf);
}

} // namespace Scene
