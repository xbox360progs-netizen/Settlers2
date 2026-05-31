#include "stdafx.h"
#include "EditorScene.h"
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
#include "../Core/LanguageManager.h"
#include <iostream>
#include <cstdio>

namespace Scene {

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
    int count;
};

static const ResourceMenuGroupDef kResourceMenuGroups[] = {
    { "icon_resource_wood",  { World::ResourceType_Wood, World::ResourceType_RealWood, World::ResourceType_ExoticWood }, 3 },
    { "icon_resource_stone", { World::ResourceType_Stone, World::ResourceType_Marble, World::ResourceType_Granite }, 3 },
    { "icon_resource_mine",  { World::ResourceType_Iron, World::ResourceType_Gold, World::ResourceType_Coal, World::ResourceType_BronzeOre, World::ResourceType_Titanium, World::ResourceType_Salpeter }, 6 },
    { "icon_resource_food",  { World::ResourceType_Corn, World::ResourceType_Water, World::ResourceType_Meat, World::ResourceType_Fish }, 4 }
};

EditorScene::EditorScene()
    : Scene("Editor")
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
    , m_weightMenuPlacementMode(false)
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
{
}

EditorScene::~EditorScene() {
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
		items.push_back(RadialMenu::MenuItem(std::wstring(L"Roads"), World::Roads, "build_way"));
		items.push_back(RadialMenu::MenuItem(std::wstring(L"Nodes"), World::Nodes, "set_nodes"));
		items.push_back(RadialMenu::MenuItem(std::wstring(L"Placement"), World::Placement, "set_placement"));
		items.push_back(RadialMenu::MenuItem(std::wstring(L"Resources"), World::Resources, "set_resources"));
		items.push_back(RadialMenu::MenuItem(std::wstring(L"Ground"), World::Ground, "set_bg"));
		items.push_back(RadialMenu::MenuItem(std::wstring(L"Objects"), World::Objects, "set_landscape"));
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

	// Streets atlas for Roads layer — slot 16
	LPDIRECT3DTEXTURE9 streetsTex = registry.getTextureOrLoad("streets");
	if (streetsTex && m_spriteRenderer) {
		m_spriteRenderer->SetTextureSlot(16, streetsTex);
		OutputDebugStringA("[EditorScene] Streets atlas loaded and bound to slot 16\n");
	}

	// UI atlas (loaded by LoadingScene) — slot 14 for cursor, button hints, menu sprites
	std::tr1::shared_ptr<SpriteAtlas> uiAtlas = registry.getAtlas("ui");
	LPDIRECT3DTEXTURE9 uiTex = (uiAtlas ? uiAtlas->GetTexture() : NULL);
	if (uiTex && m_spriteRenderer) {
		m_spriteRenderer->SetTextureSlot(14, uiTex);
	}

	// Extract bg/cell UVs from UI atlas (menu_Grid, menu_cell)
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
			uint32_t cellIdx = uiAtl->GetIndex("menu_cell");
			if (cellIdx != 0xFFFFFFFF) {
				const SpriteRegion* reg = uiAtl->GetRegion(cellIdx);
				if (reg) { cellUV.u0 = reg->u0; cellUV.v0 = reg->v0; cellUV.u1 = reg->u1; cellUV.v1 = reg->v1; }
			} else {
				OutputDebugStringA("[EditorScene] WARNING: 'menu_cell' NOT FOUND in UI atlas!\n");
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
		World::Map* map = new World::Map(Editor::MapEditor::GRID_WIDTH, Editor::MapEditor::GRID_HEIGHT, Editor::MapEditor::GRID_WIDTH * 2, Editor::MapEditor::GRID_HEIGHT * 4);
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

	OutputDebugStringA("[EditorScene] Load() complete\n");
	m_loaded = true;
}

void EditorScene::Unload() {
    if (m_radialMenu) {
        m_radialMenu->Shutdown();
    }
}

void EditorScene::Update(float deltaTime) {
    UpdateFPS();

    if (!m_inputManager) return;

    Input::Gamepad* gamepad = m_inputManager->GetGamepad();
    if (!gamepad) return;

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
            if (m_weightMenuPlacementMode) {
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
                m_editorMode = m_weightMenuPlacementMode ? MODE_PLACEMENT : MODE_WEIGHTS;
                m_weightMenuPlacementMode = false;
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
            m_weightMenuPlacementMode = (m_currentLayer == World::Placement);
            m_weightMenu->SetPlacementMode(m_weightMenuPlacementMode);
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
            if (m_gridMenu->IsVisible() && (m_currentLayer == World::Ground || m_currentLayer == World::Objects || m_currentLayer == World::Roads)) {
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
    int selectedType = m_radialMenu->GetSelectedTypeId();
    m_currentLayer = static_cast<World::LayerType>(selectedType);

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
        const char* atlasName = (m_currentLayer == World::Objects) ? "maptiles" : "ground";
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
    float moveSpeed = 500.0f * deltaTime;
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
        m_camera->Zoom(rightY * 1.0f * deltaTime);
    }

    m_camera->Update();

    if (m_shaderManager) {
        m_shaderManager->UpdateGlobalMatrices(&m_camera->GetViewMatrix(), &m_camera->GetProjectionMatrix());
    }
}

void EditorScene::UpdateMapEditor(float deltaTime, Input::Gamepad* gamepad) {
    m_mapEditor->Update(deltaTime);

    // Open save/load menu with Start
    if (!m_saveLoadMenuActive && gamepad->IsButtonPressed(Input::GP_Start)) {
        m_saveLoadMenuActive = true;
        m_saveLoadMenuSection = 0;
        m_saveLoadMenuSelection = 0;
    }

    // If menu is active, handle it instead of normal editor input
    if (m_saveLoadMenuActive) {
        UpdateSaveLoadMenu(gamepad);
        return;
    }

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
    } else {
        // Cancel road building with B
        if (gamepad->IsButtonPressed(Input::GP_B)) {
            if (m_currentLayer == World::Roads && m_mapEditor->GetRoadBuildState() == Editor::ROAD_PLACING) {
                m_mapEditor->CancelRoad();
                OutputDebugStringA("[Editor] Road building cancelled\n");
            }
        }

        if (gamepad->IsButtonPressed(Input::GP_X)) {
            m_mapEditor->DeleteObjectAt(m_mapEditor->GetCursorTileX(), m_mapEditor->GetCursorTileY());
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
                m_mapEditor->PaintCurrentTile();
            }
        }
    }
}

void EditorScene::UpdateSaveLoadMenu(Input::Gamepad* gamepad) {
    const int kMainItems = 4;     // Save, Load, Main Menu, Close
    const int kSlotItems = SAVE_SLOT_COUNT + 1; // 10 slots + Back

    switch (m_saveLoadMenuSection) {
    case 0: // Main menu
        if (gamepad->IsButtonPressed(Input::GP_DPadUp))
            m_saveLoadMenuSelection = (m_saveLoadMenuSelection - 1 + kMainItems) % kMainItems;
        if (gamepad->IsButtonPressed(Input::GP_DPadDown))
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
        if (gamepad->IsButtonPressed(Input::GP_DPadUp)) {
            m_saveLoadMenuSelection--;
            if (m_saveLoadMenuSelection < 0) m_saveLoadMenuSelection = kSlotItems - 1;
        }
        if (gamepad->IsButtonPressed(Input::GP_DPadDown))
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
            if (gamepad->IsButtonPressed(Input::GP_DPadUp)) {
                m_saveLoadMenuSelection--;
                if (m_saveLoadMenuSelection < 0) m_saveLoadMenuSelection = items - 1;
            }
            if (gamepad->IsButtonPressed(Input::GP_DPadDown))
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
        if (gamepad->IsButtonPressed(Input::GP_DPadLeft) || gamepad->IsButtonPressed(Input::GP_DPadRight))
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
        if (gamepad->IsButtonPressed(Input::GP_DPadLeft) || gamepad->IsButtonPressed(Input::GP_DPadRight))
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
}

void EditorScene::RenderSaveLoadMenu(Graphics::RenderQueue* renderQueue) {
    if (!m_saveLoadMenuActive || !m_textManager) return;
/*
    // Draw semi-transparent overlay
    Graphics::RenderCommand overlay = {};
    overlay.x = 0.0f; overlay.y = 0.0f;
    overlay.width = 1280.0f; overlay.height = 720.0f;
    overlay.u0 = 0.0f; overlay.v0 = 0.0f; overlay.u1 = 1.0f; overlay.v1 = 1.0f;
    overlay.color = 0x80000000;
    overlay.textureID = 0;
    overlay.shaderID = SHADER_UI;
    overlay.blendMode = 1;
    overlay.layer = LAYER_UI;
    overlay.depth = 200;
    overlay.sortKey = Graphics::BuildSortKey(LAYER_UI, 1, SHADER_UI, 0, 200);
    renderQueue->Submit(overlay);
*/
    // Draw editor_menu sprite as decoration
    {
        TextureRegistry& reg = TextureRegistry::instance();
        std::tr1::shared_ptr<SpriteAtlas> uiAtl = reg.getAtlas("ui");
        if (uiAtl) {
            uint32_t menuIdx = uiAtl->GetIndex("editor_menu");
            if (menuIdx != 0xFFFFFFFF) {
                const SpriteRegion* menuReg = uiAtl->GetRegion(menuIdx);
                if (menuReg) {
                    Graphics::RenderCommand menuCmd = {};
                    menuCmd.x = 440.0f; menuCmd.y = 60.0f;
                    menuCmd.width = 400.0f; menuCmd.height = 400.0f;
                    menuCmd.u0 = menuReg->u0; menuCmd.v0 = menuReg->v0;
                    menuCmd.u1 = menuReg->u1; menuCmd.v1 = menuReg->v1;
                    menuCmd.color = 0xFFFFFFFF;
                    menuCmd.textureID = 14;
                    menuCmd.shaderID = SHADER_UI;
                    menuCmd.blendMode = 1;
                    menuCmd.layer = LAYER_UI;
                    menuCmd.depth = 199;
                    menuCmd.sortKey = Graphics::BuildSortKey(LAYER_UI, 1, SHADER_UI, 14, 199);
                    renderQueue->Submit(menuCmd);
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

    if (!gamepad->IsButtonPressed(Input::GP_A))
        return false;

    int selectedIndex = m_gridMenu->GetSelectedSpriteIndex();
    if (selectedIndex < 0 || !m_mapEditor)
        return false;

    const std::tr1::shared_ptr<SpriteAtlas> uiAtl = TextureRegistry::instance().getAtlas("ui");
    if (!uiAtl)
        return false;

    const SpriteRegion* region = uiAtl->GetRegion(selectedIndex);
    if (!region)
        return false;

    if (m_resourceMenuShowingGroups) {
        for (int i = 0; i < kResourceMenuGroupCount; ++i) {
            if (region->name == kResourceMenuGroups[i].iconName) {
                m_resourceMenuGroupIndex = i;
                LoadResourceGroupResources(i);
                return true;
            }
        }
    } else {
        for (int i = 1; i < World::ResourceType_Count; ++i) {
            World::ResourceType rt = static_cast<World::ResourceType>(i);
            if (region->name == World::ResourceTypeToIconName(rt)) {
                m_activeResourceType = rt;
                m_resourceAmount = World::GetDefaultResourceAmount(rt);
                m_currentState = STATE_PLACING;
                m_depositConfirmPending = false;
                m_depositBuildingSpriteIdx = -1;
                const char* buildingName = World::ResourceTypeToBuildingSpriteName(rt);
                if (buildingName && buildingName[0]) {
                    std::tr1::shared_ptr<SpriteAtlas> maptiles = TextureRegistry::instance().getAtlas("maptiles");
                    if (maptiles) m_depositBuildingSpriteIdx = (int)maptiles->GetIndex(buildingName);
                }
                if (m_depositBuildingSpriteIdx < 0) {
                    m_depositBuildingSpriteIdx = selectedIndex;
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
        Graphics::RenderCommand cmd = {};
        cmd.x = 0.0f; cmd.y = 0.0f;
        cmd.width = 1280.0f; cmd.height = 720.0f;
        cmd.u0 = 0.0f; cmd.v0 = 0.0f;
        cmd.u1 = 1.0f; cmd.v1 = 1.0f;
        cmd.color = 0xFFFFFFFF;
        cmd.shaderID = SHADER_UI;
        cmd.textureID = 10;
        cmd.blendMode = 1;
        cmd.layer = LAYER_UI;
        cmd.depth = 255;
        renderQueue->Submit(cmd);
    }
    // Render world content
    m_mapEditor->RenderGeometry();
    m_mapEditor->RenderUI();

    if (m_textManager) {
        char fpsText[64];
        sprintf(fpsText, "FPS: %d", m_fps);
        m_textManager->DrawTextToScreen(fpsText, 650.0f, 720.0f - 60.0f, 0xFF00FF00, 0.25f);

        static const char* layerNames[] = {
            "Roads", "Nodes", "Placement", "Resources", "Ground", "Objects", "Overlay"
        };
        const char* layerName = "Unknown";
        int layerIdx = static_cast<int>(m_currentLayer);
        if (layerIdx >= 0 && layerIdx < 7) {
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
                Graphics::RenderCommand btnCmd = {};
                btnCmd.x = 407.0f; btnCmd.y = 567.0f;
                btnCmd.width = 32.0f; btnCmd.height = 32.0f;
                btnCmd.u0 = m_buttonAUV.u0; btnCmd.v0 = m_buttonAUV.v0;
                btnCmd.u1 = m_buttonAUV.u1; btnCmd.v1 = m_buttonAUV.v1;
                btnCmd.color = 0xFFFFFFFF;
                btnCmd.shaderID = SHADER_UI;
                btnCmd.textureID = 13;
                btnCmd.blendMode = 1;
                btnCmd.layer = LAYER_UI;
                btnCmd.depth = 60;
                btnCmd.sortKey = Graphics::BuildSortKey(LAYER_UI, 1, SHADER_UI, 13, 60);
                renderQueue->Submit(btnCmd);
            }
            m_textManager->DrawTextToScreen("Select", 424.0f, 563.0f, 0xFF44FF44, 0.22f);
            // Bottom-right: Close + button_B (cell end - 5px = 871, 15px gap from Close to B)
            m_textManager->DrawTextToScreen("Close", 784.0f, 563.0f, 0xFFFF4444, 0.22f);
            {
                Graphics::RenderCommand btnCmd = {};
                btnCmd.x = 839.0f; btnCmd.y = 567.0f;
                btnCmd.width = 32.0f; btnCmd.height = 32.0f;
                btnCmd.u0 = m_buttonBUV.u0; btnCmd.v0 = m_buttonBUV.v0;
                btnCmd.u1 = m_buttonBUV.u1; btnCmd.v1 = m_buttonBUV.v1;
                btnCmd.color = 0xFFFFFFFF;
                btnCmd.shaderID = SHADER_UI;
                btnCmd.textureID = 13;
                btnCmd.blendMode = 1;
                btnCmd.layer = LAYER_UI;
                btnCmd.depth = 60;
                btnCmd.sortKey = Graphics::BuildSortKey(LAYER_UI, 1, SHADER_UI, 13, 60);
                renderQueue->Submit(btnCmd);
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
                texSlot = 16;
            }
            std::tr1::shared_ptr<SpriteAtlas> atlas = reg.getAtlas(atlasName);
            if (atlas && tileIdx < (int)atlas->GetRegionCount()) {
                const SpriteRegion* region = atlas->GetRegion(tileIdx);
                if (region) {
                    Graphics::RenderCommand cmd = {};
                    cmd.x = 10.0f;
                    cmd.y = 40.0f;
                    cmd.width = 64.0f;
                    cmd.height = 64.0f;
                    cmd.u0 = region->u0; cmd.v0 = region->v0;
                    cmd.u1 = region->u1; cmd.v1 = region->v1;
                    cmd.color = 0xFFFFFFFF;
                    cmd.shaderID = SHADER_UI;
                    cmd.textureID = texSlot;
                    cmd.blendMode = 1;
                    cmd.layer = LAYER_UI;
                    cmd.depth = 200;
                    cmd.sortKey = Graphics::BuildSortKey(LAYER_UI, 1, SHADER_UI, texSlot, 200);
                    renderQueue->Submit(cmd);
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

        std::tr1::shared_ptr<SpriteAtlas> maptiles = TextureRegistry::instance().getAtlas("maptiles");
        if (maptiles && m_depositBuildingSpriteIdx >= 0 && m_depositBuildingSpriteIdx < (int)maptiles->GetRegionCount()) {
            const SpriteRegion* region = maptiles->GetRegion(m_depositBuildingSpriteIdx);
            if (region) {
                float previewW = (float)region->width;
                float previewH = (float)region->height;

                Graphics::RenderCommand cmd = {};
                cmd.x = wx - region->pivotX;
                cmd.y = wy - region->pivotY;
                cmd.width = previewW;
                cmd.height = previewH;
                cmd.u0 = region->u0; cmd.v0 = region->v0;
                cmd.u1 = region->u1; cmd.v1 = region->v1;
                cmd.color = 0xFFFFFFFF;
                cmd.shaderID = SHADER_TERRAIN;
                cmd.textureID = 9;
                cmd.blendMode = 1;
                cmd.layer = LAYER_WORLD;
                cmd.depth = static_cast<WORD>(0.99f * 65535.0f);
                renderQueue->Submit(cmd);
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
    OutputDebugStringA("[EditorScene] OnEnter\n");
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

        // Set bg/cell UV from UI atlas (menu_Grid, menu_cell)
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
            uint32_t cellIdx = uiAtl->GetIndex("menu_cell");
            if (cellIdx != 0xFFFFFFFF) {
                const SpriteRegion* reg = uiAtl->GetRegion(cellIdx);
                if (reg) { cellUV.u0 = reg->u0; cellUV.v0 = reg->v0; cellUV.u1 = reg->u1; cellUV.v1 = reg->v1; }
                else { OutputDebugStringA("[EditorScene LoadGridMenuAtlas] WARNING: 'menu_cell' found but GetRegion NULL\n"); }
            } else {
                OutputDebugStringA("[EditorScene LoadGridMenuAtlas] WARNING: 'menu_cell' NOT FOUND\n");
            }
        }
    m_gridMenu->SetBackgroundUV(bgUV);
    m_gridMenu->SetCellUV(cellUV);
    m_gridMenu->SetCellSpacing(139.0f, 94.0f);
    m_gridMenu->SetCellVisualSize(117.0f, 72.0f);
    m_gridMenu->SetCellPadding(15.0f);

    std::vector<GridMenu::TileUV> uvs;
    std::vector<int> globalIndices;
        uvs.reserve(atlas->GetRegionCount());
        globalIndices.reserve(atlas->GetRegionCount());
        for (uint32_t i = 0; i < atlas->GetRegionCount(); ++i) {
            const SpriteRegion* reg = atlas->GetRegion(i);
            GridMenu::TileUV tu;
            if (reg) { tu.u0 = reg->u0; tu.v0 = reg->v0; tu.u1 = reg->u1; tu.v1 = reg->v1; }
            else { tu.u0 = 0.0f; tu.v0 = 0.0f; tu.u1 = 1.0f; tu.v1 = 1.0f; }
            uvs.push_back(tu);
            globalIndices.push_back((int)i);
        }
        m_gridMenu->SetTileData(uvs, globalIndices);
        m_gridMenu->SetSpriteRenderer(m_spriteRenderer);
        m_gridMenu->ResetSelection();

        // Build cell labels from atlas region names via language.ini
        {
            std::vector<std::string> labels;
            labels.reserve(atlas->GetRegionCount());
            for (uint32_t i = 0; i < atlas->GetRegionCount(); ++i) {
                const SpriteRegion* reg = atlas->GetRegion(i);
                std::string label;
                if (reg && !reg->name.empty())
                    label = LanguageManager::instance().GetString(reg->name);
                labels.push_back(label);
            }
            m_gridMenu->SetCellLabels(labels);
        }

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

    // Set bg/cell UV from UI atlas (menu_Grid, menu_cell)
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
        uint32_t cellIdx = uiAtl->GetIndex("menu_cell");
        if (cellIdx != 0xFFFFFFFF) {
            const SpriteRegion* reg = uiAtl->GetRegion(cellIdx);
            if (reg) { cellUV.u0 = reg->u0; cellUV.v0 = reg->v0; cellUV.u1 = reg->u1; cellUV.v1 = reg->v1; }
            else { OutputDebugStringA("[EditorScene LoadGridMenuGroup] WARNING: 'menu_cell' found but GetRegion NULL\n"); }
        } else {
            OutputDebugStringA("[EditorScene LoadGridMenuGroup] WARNING: 'menu_cell' NOT FOUND\n");
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
    
    uint32_t cellIdx = uiAtl->GetIndex("menu_cell");
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
        uint32_t cellIdx = uiAtl->GetIndex("menu_cell");
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
    }

    m_gridMenu->SetTileData(uvs, globalIndices);
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
    if (!uiAtl) return;

    std::vector<GridMenu::TileUV> uvs;
    std::vector<int> globalIndices;
    const ResourceMenuGroupDef& group = kResourceMenuGroups[groupIndex];

    for (int i = 0; i < group.count; ++i) {
        World::ResourceType rt = group.resources[i];
        const char* iconName = World::ResourceTypeToIconName(rt);
        if (!iconName || !iconName[0]) continue;

        uint32_t spriteIdx = uiAtl->GetIndex(iconName);
        if (spriteIdx == 0xFFFFFFFF) {
            char buf[128];
            sprintf_s(buf, "[EditorScene] Resource icon '%s' not found in UI atlas\n", iconName);
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
    }

    m_gridMenu->SetTileData(uvs, globalIndices);
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
