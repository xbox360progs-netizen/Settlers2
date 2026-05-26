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
const int EditorScene::kResourceTypeCount = 7;

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
    , m_resourcesInitialized(false)
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
        m_camera->Zoom(1.5f);
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

    // Ground atlas (loaded by LoadingScene)
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
            m_radialMenu->SetIconTextureSlot(5);
        }
    }

    // Extract bg/cell UVs from maptiles atlas (menu_background_cell, menu_cell)
    GridMenu::TileUV bgUV = {0,0,1,1}, cellUV = {0,0,1,1};
    if (maptilesAtlas) {
        uint32_t bgIdx = maptilesAtlas->GetIndex("menu_background_cell");
        if (bgIdx != 0xFFFFFFFF) {
            const SpriteRegion* reg = maptilesAtlas->GetRegion(bgIdx);
            if (reg) { bgUV.u0 = reg->u0; bgUV.v0 = reg->v0; bgUV.u1 = reg->u1; bgUV.v1 = reg->v1; }
        } else {
            OutputDebugStringA("[EditorScene] WARNING: 'menu_background_cell' NOT FOUND in maptiles atlas!\n");
        }
        uint32_t cellIdx = maptilesAtlas->GetIndex("menu_cell");
        if (cellIdx != 0xFFFFFFFF) {
            const SpriteRegion* reg = maptilesAtlas->GetRegion(cellIdx);
            if (reg) { cellUV.u0 = reg->u0; cellUV.v0 = reg->v0; cellUV.u1 = reg->u1; cellUV.v1 = reg->v1; }
            else { OutputDebugStringA("[EditorScene] WARNING: 'menu_cell' found but GetRegion returned NULL!\n"); }
        } else {
            OutputDebugStringA("[EditorScene] WARNING: 'menu_cell' NOT FOUND in maptiles atlas!\n");
        }
    }

    char logMsg[256];
    _snprintf(logMsg, sizeof(logMsg), "[EditorScene] Textures: Ground=%p, Maptiles=%p\n", 
              (void*)groundTexture, (void*)maptilesTex);
    OutputDebugStringA(logMsg);

    // Register GridMenu texture slots (bg/cell use maptiles texture with UV sub-rects)
    if (m_spriteRenderer) {
        m_spriteRenderer->SetTextureSlot(6, maptilesTex);
        m_spriteRenderer->SetTextureSlot(7, maptilesTex);
        m_spriteRenderer->SetTextureSlot(8, groundTexture);
        m_spriteRenderer->SetTextureSlot(9, groundTexture);
    }

    // Initialize WeightMenu textures
    if (m_weightMenu) {
        std::tr1::shared_ptr<SpriteAtlas> wmMaptiles = registry.getAtlas("maptiles");
        LPDIRECT3DTEXTURE9 dpadCrossTex = NULL;
        float dpadU0 = 0.0f, dpadV0 = 0.0f, dpadU1 = 1.0f, dpadV1 = 1.0f;
        if (wmMaptiles) {
            uint32_t dpadIdx = wmMaptiles->GetIndex("dpad_cross");
            if (dpadIdx != 0xFFFFFFFF) {
                const SpriteRegion* dpadReg = wmMaptiles->GetRegion(dpadIdx);
                if (dpadReg) {
                    dpadU0 = dpadReg->u0; dpadV0 = dpadReg->v0;
                    dpadU1 = dpadReg->u1; dpadV1 = dpadReg->v1;
                }
            }
            dpadCrossTex = wmMaptiles->GetTexture();
        }
        m_weightMenu->SetTextureSlots(10, 11);
        m_weightMenu->SetTextures(maptilesTex, dpadCrossTex);
        m_weightMenu->SetDpadUV(dpadU0, dpadV0, dpadU1, dpadV1);
        m_weightMenu->SetBackgroundUV(bgUV.u0, bgUV.v0, bgUV.u1, bgUV.v1);
        if (m_spriteRenderer) {
            m_spriteRenderer->SetTextureSlot(10, maptilesTex);
            m_spriteRenderer->SetTextureSlot(11, dpadCrossTex);
        }
        OutputDebugStringA("[EditorScene] WeightMenu textures set (dpad_cross from maptiles)\n");
    }

    // Look up button hint textures from maptiles atlas
    m_buttonAUV.u0 = 0.0f; m_buttonAUV.v0 = 0.0f; m_buttonAUV.u1 = 1.0f; m_buttonAUV.v1 = 1.0f;
    m_buttonBUV.u0 = 0.0f; m_buttonBUV.v0 = 0.0f; m_buttonBUV.u1 = 1.0f; m_buttonBUV.v1 = 1.0f;
    if (maptilesAtlas) {
        uint32_t btnAIdx = maptilesAtlas->GetIndex("button_A");
        if (btnAIdx != 0xFFFFFFFF) {
            const SpriteRegion* reg = maptilesAtlas->GetRegion(btnAIdx);
            if (reg) { m_buttonAUV.u0 = reg->u0; m_buttonAUV.v0 = reg->v0; m_buttonAUV.u1 = reg->u1; m_buttonAUV.v1 = reg->v1; }
        }
        uint32_t btnBIdx = maptilesAtlas->GetIndex("button_B");
        if (btnBIdx != 0xFFFFFFFF) {
            const SpriteRegion* reg = maptilesAtlas->GetRegion(btnBIdx);
            if (reg) { m_buttonBUV.u0 = reg->u0; m_buttonBUV.v0 = reg->v0; m_buttonBUV.u1 = reg->u1; m_buttonBUV.v1 = reg->v1; }
        }
    }
    if (m_spriteRenderer) {
        m_spriteRenderer->SetTextureSlot(13, maptilesTex);
    }

    // Initialize GridMenu with textures (bg/cell from maptiles atlas)
    if (!m_gridMenu && m_renderer) {
        m_gridMenu = new GridMenu();
        if (m_gridMenu->Initialize()) {
            m_gridMenu->SetTextures(maptilesTex, maptilesTex, groundTexture);
            m_gridMenu->SetTextureSlots(6, 7, 8);
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

    OutputDebugStringA("[EditorScene] Load() complete\n");
}

void EditorScene::Unload() {
    if (m_radialMenu) {
        m_radialMenu->Shutdown();
    }
}

void EditorScene::Update(float deltaTime) {
    // FPS calculation
    DWORD now = GetTickCount();
    m_frameCount++;
    if (now - m_lastFpsTime >= 1000) {
        m_fps = m_frameCount;
        m_frameCount = 0;
        m_lastFpsTime = now;
    }

	if (!m_inputManager) return;

	Input::Gamepad* gamepad = m_inputManager->GetGamepad();
	if (!gamepad) return;

	bool menuActive = (m_gridMenu && m_gridMenu->IsVisible()) || (m_radialMenu && m_radialMenu->IsVisible());

	// === MENU INPUT (priority over camera when menu is open) ===
	// Toggle RadialMenu with LB - only when GridMenu is NOT visible
	if (gamepad->IsButtonPressed(Input::GP_LB)) {
		if (m_gridMenu && m_gridMenu->IsVisible()) {
			// GridMenu is active, don't show RadialMenu
		} else if (m_radialMenu) {
			if (m_radialMenu->IsVisible()) {
				m_radialMenu->Hide();
			} else {
				m_radialMenu->Show(640.0f, 360.0f);
			}
		}
	}

	// Handle D-pad input for weight selection when Nodes menu is visible
	if (m_weightMenuVisible && m_weightMenu) {
		m_weightMenu->Update(gamepad, deltaTime);
		m_weightMenuVisible = m_weightMenu->IsVisible();
		if (!m_weightMenuVisible) {
			// Menu was closed by B button (handled internally in WeightMenu::Update)
		} else {
			bool selected = false;
			if (m_weightMenuPlacementMode) {
				// Placement mode: Up=Occupied, Down=Free
				if (gamepad->IsButtonPressed(Input::GP_DPadUp)) {
					m_activeWeight = World::Weight_Block;
					selected = true;
				}
				else if (gamepad->IsButtonPressed(Input::GP_DPadDown)) {
					m_activeWeight = World::Weight_Land;
					selected = true;
				}
			} else {
				if (gamepad->IsButtonPressed(Input::GP_DPadUp)) {
					m_activeWeight = World::Weight_Block;
					selected = true;
				}
				else if (gamepad->IsButtonPressed(Input::GP_DPadDown)) {
					m_activeWeight = World::Weight_Deep;
					selected = true;
				}
				else if (gamepad->IsButtonPressed(Input::GP_DPadLeft)) {
					m_activeWeight = World::Weight_Shallow;
					selected = true;
				}
				else if (gamepad->IsButtonPressed(Input::GP_DPadRight)) {
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

	if (!menuActive && !m_weightMenuVisible) {
		// Toggle GridMenu with RB (blocked for Nodes layer — WeightMenu used instead)
		if (gamepad->IsButtonPressed(Input::GP_RB)) {
			if (m_currentLayer == World::Nodes || m_currentLayer == World::Placement) {
				if (m_weightMenu) {
					if (m_weightMenuVisible) {
						m_weightMenu->Close();
						m_weightMenuVisible = false;
					} else {
						m_weightMenuPlacementMode = (m_currentLayer == World::Placement);
						m_weightMenu->SetPlacementMode(m_weightMenuPlacementMode);
						m_weightMenu->Open(m_activeWeight);
						m_weightMenuVisible = true;
					}
				}
			} else if (m_currentLayer == World::Resources && m_currentState != STATE_DEPOSIT_PREVIEW) {
				// (blocked during deposit preview to avoid interrupting the flow)
				if (!m_gridMenu) {
					m_gridMenu = new GridMenu();
					if (m_gridMenu->Initialize()) {
						m_gridMenu->SetTextureSlots(6, 7, 8);
						LoadResourceIcons();
					}
					m_gridMenu->Show(640.0f, 330.0f);
				} else if (m_gridMenu->IsVisible()) {
					m_gridMenu->Hide();
				} else {
					LoadResourceIcons();
					m_gridMenu->Show(640.0f, 330.0f);
				}
			} else {
        if (!m_gridMenu) {
                m_gridMenu = new GridMenu();
                if (m_gridMenu->Initialize()) {
                    m_gridMenu->SetTextureSlots(6, 7, 8);
                    // Load atlas based on current layer
                    if (m_currentLayer == World::Objects) {
                        m_objectGroupIndex = 0;
                        LoadGridMenuGroup(kObjectGroupNames[m_objectGroupIndex]);
                        if (m_mapEditor) m_mapEditor->SetObjectGroup(kObjectGroupNames[m_objectGroupIndex]);
                    } else {
                        LoadGridMenuAtlas("ground");
                    }
                }
                m_gridMenu->Show(640.0f, 330.0f);
			} else if (m_gridMenu->IsVisible()) {
				m_gridMenu->Hide();
        } else {
                // Show GridMenu with appropriate group/atlas for current layer
                if (m_currentLayer == World::Objects) {
                    LoadGridMenuGroup(kObjectGroupNames[m_objectGroupIndex]);
                    if (m_mapEditor) m_mapEditor->SetObjectGroup(kObjectGroupNames[m_objectGroupIndex]);
                } else {
                    LoadGridMenuAtlas("ground");
                }
                m_gridMenu->Show(640.0f, 330.0f);
            }
		}
	}
	}

	// Update InputController for button events
	if (m_inputController) {
		m_inputController->Update();

		// For MODE_WEIGHTS: compute tile at camera center (screen center), not from stick cursor
		if (m_editorMode == MODE_WEIGHTS && !m_weightMenuVisible) {
			float centerWorldX, centerWorldY;
			m_camera->GetWorldCenter(centerWorldX, centerWorldY);

			if (m_mapEditor && m_mapEditor->GetMap()) {
				int tileX, tileY;
				if (m_mapEditor->GetMap()->GetTileAt(centerWorldX, centerWorldY, m_currentLayer, tileX, tileY)) {
					m_selectedTileX = tileX;
					m_selectedTileY = tileY;
					m_hasSelection = true;
					float tileWorldX, tileWorldY;
					CoordinateSystem::GetInstance().NodeTileToWorld(tileX, tileY, tileWorldX, tileWorldY);
					char buf[256];
					sprintf_s(buf, "[WEIGHT] Center world=(%.0f,%.0f) tile=(%d,%d) tileWorld=(%.0f,%.0f) layer=%d\n",
						centerWorldX, centerWorldY, tileX, tileY, tileWorldX, tileWorldY, m_currentLayer);
					OutputDebugStringA(buf);
				}
			}
		} else {
			// Get world cursor position from stick for object placement
			float worldX, worldY;
			m_inputController->GetWorldCursor(worldX, worldY);

			// Get tile at world coordinates directly (no camera round-trip)
			if (m_mapEditor && m_mapEditor->GetMap()) {
				int tileX, tileY;
                if (m_mapEditor->GetMap()->GetTileAt(worldX, worldY, m_currentLayer, tileX, tileY)) {
                    // Update phantom tile position when in PLACING or DEPOSIT_PREVIEW state
                    if (m_currentState == STATE_PLACING || m_currentState == STATE_DEPOSIT_PREVIEW) {
                        m_phantomTileX = tileX;
                        m_phantomTileY = tileY;
                    }
                }
			}
		}

		// FSM: Handle resource placement state machine
		switch (m_currentState) {
			case STATE_IDLE:
				// Normal camera movement and tile painting
				// Handle weight painting in MODE_WEIGHTS
				if (m_editorMode == MODE_WEIGHTS && !m_weightMenuVisible) {
					if (m_inputController->IsButtonAPressed()) {
						if (m_mapEditor && m_mapEditor->GetMap() && m_hasSelection) {
							m_mapEditor->GetMap()->SetNodeWeight(m_selectedTileX, m_selectedTileY, m_activeWeight);
							char buf[256];
							sprintf_s(buf, "[WEIGHT] PAINT tile=(%d,%d) weight=%d\n",
								m_selectedTileX, m_selectedTileY, m_activeWeight);
							OutputDebugStringA(buf);
						}
					}
				}
				break;
				
			case STATE_SELECTING:
				// GridMenu is open, waiting for selection
				break;
				
			case STATE_PLACING:
				// Phantom resource follows cursor
				// A button: Place resource and open OSK
				if (m_inputController->IsButtonAPressed()) {
					// Transition to INPUT_AMOUNT state
					m_currentState = STATE_INPUT_AMOUNT;
					// TODO: Open OSK for amount input
					// For now, use default amount of 500
					if (m_mapEditor && m_mapEditor->GetMap()) {
						m_mapEditor->GetMap()->SetResourceNode(m_phantomTileX, m_phantomTileY, m_activeResourceType, 500, true);
					}
					m_currentState = STATE_PLACING; // Return to placing for multiple placements
				}
				// B button: Cancel placement
				if (m_inputController->IsButtonBPressed()) {
					m_currentState = STATE_IDLE;
					m_activeResourceType = World::ResourceType_None;
				}
				break;
				
            case STATE_DEPOSIT_PREVIEW:
            {
                // Building preview follows cursor (phantom tile updated in the world cursor block above)
                if (m_inputController->IsButtonAPressed()) {
                    if (!m_depositConfirmPending) {
                        // First A press: show confirmation prompt
                        m_depositConfirmPending = true;
                    } else {
                        // Second A press: confirm placement, set resource node
                        if (m_mapEditor && m_mapEditor->GetMap()) {
                            m_mapEditor->GetMap()->SetResourceNode(
                                m_phantomTileX, m_phantomTileY,
                                m_activeResourceType, m_resourceAmount, true);
                        }
                        m_currentState = STATE_IDLE;
                        m_depositConfirmPending = false;
                        m_activeResourceType = World::ResourceType_None;
                        m_depositBuildingSpriteIdx = -1;
                    }
                }
                if (m_inputController->IsButtonBPressed()) {
                    // B: cancel everything
                    m_currentState = STATE_IDLE;
                    m_depositConfirmPending = false;
                    m_activeResourceType = World::ResourceType_None;
                    m_depositBuildingSpriteIdx = -1;
                }
                break;
            }
            case STATE_INPUT_AMOUNT:
                // OSK is open, waiting for input
                // This will be handled by OSK callback
                break;
        }
	}

	// === CAMERA CONTROL (only when menu is NOT active and not in PLACING state) ===
	if (!menuActive && m_camera) {
		float moveSpeed = 500.0f * deltaTime; // pixels per second
		float stickX, stickY;
		gamepad->GetLeftStick(stickX, stickY);
		
		if (fabsf(stickX) > 0.1f || fabsf(stickY) > 0.1f) {
			m_camera->Move(stickX * moveSpeed, stickY * moveSpeed);
		}
		
		// Right stick: zoom camera
		float rightX, rightY;
		gamepad->GetRightStick(rightX, rightY);
		if (fabsf(rightY) > 0.1f) {
			float zoomSpeed = 1.0f * deltaTime;
			m_camera->Zoom(rightY * zoomSpeed);
		}
		
		m_camera->Update();
		
		if (m_shaderManager) {
			m_shaderManager->UpdateGlobalMatrices(&m_camera->GetViewMatrix(), &m_camera->GetProjectionMatrix());
		}
	}

	// When GridMenu is visible, update it and handle selection
	if (m_gridMenu && m_gridMenu->IsVisible()) {
		m_gridMenu->Update(gamepad, deltaTime);
		if (gamepad->IsButtonPressed(Input::GP_A)) {
			int selectedIndex = m_gridMenu->GetSelectedSpriteIndex();
			if (selectedIndex >= 0 && m_mapEditor) {
            if (m_currentLayer == World::Resources) {
                // Resource layer: map selected icon to ResourceType
                const std::tr1::shared_ptr<SpriteAtlas>& maptiles = TextureRegistry::instance().getAtlas("maptiles");
                if (maptiles) {
                    const SpriteRegion* region = maptiles->GetRegion(selectedIndex);
                    if (region) {
                        for (int i = 1; i <= kResourceTypeCount; ++i) {
                            World::ResourceType rt = static_cast<World::ResourceType>(i);
                            if (region->name == World::ResourceTypeToIconName(rt)) {
                                m_activeResourceType = rt;
                                m_resourceAmount = World::GetDefaultResourceAmount(rt);
                                if (World::IsDepositResource(rt)) {
                                    // Deposit resources: enter preview mode with building sprite
                                    m_currentState = STATE_DEPOSIT_PREVIEW;
                                    m_depositConfirmPending = false;
                                    // Cache the building sprite index
                                    const char* buildingName = World::ResourceTypeToBuildingSpriteName(rt);
                                    m_depositBuildingSpriteIdx = (buildingName && buildingName[0])
                                        ? (int)maptiles->GetIndex(buildingName)
                                        : -1;
                                    // If building sprite not found, use the resource icon itself
                                    if (m_depositBuildingSpriteIdx < 0) {
                                        m_depositBuildingSpriteIdx = selectedIndex;
                                    }
                                } else {
                                    // Tree: direct placement (existing behavior)
                                    m_currentState = STATE_PLACING;
                                }
                                char buf[128];
                                sprintf_s(buf, "[EditorScene] Selected resource type: %s, amount: %d (deposit=%d, buildingIdx=%d)\n",
                                    World::ResourceTypeToString(rt), m_resourceAmount,
                                    World::IsDepositResource(rt), m_depositBuildingSpriteIdx);
                                OutputDebugStringA(buf);
                                break;
                            }
                        }
                    }
                }
				} else {
					m_mapEditor->SetTileByIndex(selectedIndex);
				}
			}
			m_gridMenu->Hide();
		}
		if (gamepad->IsButtonPressed(Input::GP_B)) {
			m_gridMenu->Hide();
		}
		// Y button - cycle object groups (only in Objects layer)
		if (gamepad->IsButtonPressed(Input::GP_Y) && m_currentLayer == World::Objects) {
            CycleObjectGroup();
		}
		// Shoulder triggers to navigate pages (all layers use SetTileData)
		if (gamepad->IsButtonPressed(Input::GP_LB)) {
			m_gridMenu->PrevPage();
		}
		if (gamepad->IsButtonPressed(Input::GP_RB)) {
			m_gridMenu->NextPage();
		}
	}

	// Also update RadialMenu if visible
	if (m_radialMenu && m_radialMenu->IsVisible()) {
    m_radialMenu->Update(gamepad);
    if (m_radialMenu->HasSelection()) {
        int selectedType = m_radialMenu->GetSelectedTypeId();
        m_currentLayer = static_cast<World::LayerType>(selectedType);
        
        if (m_currentLayer != World::Objects) {
            m_yButtonWasPressed = false;
        }

        // Update map editor visibility and layer sync
        if (m_mapEditor) {
            m_mapEditor->SetLayer(m_currentLayer);
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
                    // Auto-assign resources for trees on first activation
                    if (!m_resourcesInitialized) {
                        m_mapEditor->AutoAssignResourcesForTrees();
                        m_resourcesInitialized = true;
                    }
                    // Reset resource placement state
                    m_currentState = STATE_IDLE;
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

        // Update sprite slot 8 with the active layer's atlas for preview
        if (m_spriteRenderer) {
            TextureRegistry& reg = TextureRegistry::instance();
            const char* atlasName = (m_currentLayer == World::Objects)
                ? "maptiles" : "ground";
            std::tr1::shared_ptr<SpriteAtlas> atlas = reg.getAtlas(atlasName);
            if (atlas) {
                m_spriteRenderer->SetTextureSlot(8, atlas->GetTexture());
            }
        }
    }
}

	// Update MapEditor (camera movement, painting) only when no menu active
	if (m_mapEditor && !menuActive) {
		m_mapEditor->Update(deltaTime);

		if (m_currentLayer == World::Resources) {
			// Resources layer: D-pad up/down to adjust amount
			if (gamepad->IsButtonPressed(Input::GP_DPadUp)) {
				m_resourceAmount += 5;
				if (m_resourceAmount > 999) m_resourceAmount = 999;
				char buf[64];
				sprintf_s(buf, "[EditorScene] Resource amount: %d\n", m_resourceAmount);
				OutputDebugStringA(buf);
			}
			if (gamepad->IsButtonPressed(Input::GP_DPadDown)) {
				m_resourceAmount -= 5;
				if (m_resourceAmount < 1) m_resourceAmount = 1;
				char buf[64];
				sprintf_s(buf, "[EditorScene] Resource amount: %d\n", m_resourceAmount);
				OutputDebugStringA(buf);
			}

			// X button - remove resource at cursor
			if (gamepad->IsButtonPressed(Input::GP_X)) {
				int tx = m_mapEditor->GetCursorTileX();
				int ty = m_mapEditor->GetCursorTileY();
				m_mapEditor->GetMap()->SetResourceNode(tx, ty, World::ResourceType_None, 0, false);
				char buf[128];
				sprintf_s(buf, "[EditorScene] Removed resource at (%d,%d)\n", tx, ty);
				OutputDebugStringA(buf);
			}

			// A button - place/edit resource at cursor (only in IDLE state, otherwise FSM handles it)
			if (gamepad->IsButtonPressed(Input::GP_A) && m_currentState == STATE_IDLE) {
				if (m_activeResourceType != World::ResourceType_None) {
					int tx = m_mapEditor->GetCursorTileX();
					int ty = m_mapEditor->GetCursorTileY();
					m_mapEditor->GetMap()->SetResourceNode(tx, ty, m_activeResourceType, m_resourceAmount, true);
					char buf[128];
					sprintf_s(buf, "[EditorScene] Placed resource %s at (%d,%d) amount=%d\n",
						World::ResourceTypeToString(m_activeResourceType), tx, ty, m_resourceAmount);
					OutputDebugStringA(buf);
				}
			}
		} else {
			// X button - delete object at cursor
			if (gamepad->IsButtonPressed(Input::GP_X)) {
				m_mapEditor->DeleteObjectAt(m_mapEditor->GetCursorTileX(), m_mapEditor->GetCursorTileY());
			}

			// A button - paint current tile on each press (not held, to avoid accidental placement after menu close)
			if (gamepad->IsButtonPressed(Input::GP_A)) {
				if (m_currentLayer != World::Placement || m_editorMode == MODE_PLACEMENT) {
					if (m_currentLayer == World::Placement) {
						m_mapEditor->SetPlacementOccupied(m_activeWeight == World::Weight_Block);
					}
					m_mapEditor->PaintCurrentTile();
				}
			}
		}
	}

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
//        OutputDebugStringA("[EditorScene::Render] SetFrameViewProj called with camera matrix\n");
    }

    m_mapEditor->RenderGeometry();
    m_mapEditor->RenderUI();

    if (m_textManager) {
        char fpsText[64];
        sprintf(fpsText, "FPS: %d", m_fps);
        m_textManager->DrawTextToScreen(fpsText, 10.0f, 10.0f, 0xFF00FF00, 0.25f);

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
        m_textManager->DrawTextToScreen(layerText, 1280.0f - 280.0f, 720.0f - 30.0f, 0xFFFFFFFF, 0.25f);

        // Show resource info when Resources layer is active
        if (m_currentLayer == World::Resources) {
            char resInfo[256];
            if (m_currentState == STATE_DEPOSIT_PREVIEW) {
                if (m_depositConfirmPending) {
                    sprintf_s(resInfo, "Place %s deposit here?  A: confirm  B: cancel",
                        World::ResourceTypeToString(m_activeResourceType));
                } else {
                    sprintf_s(resInfo, "Deposit: %s [%d]  A: place building preview  B: cancel  D-pad: adjust amount",
                        World::ResourceTypeToString(m_activeResourceType), m_resourceAmount);
                }
            } else if (m_activeResourceType != World::ResourceType_None) {
                sprintf_s(resInfo, "Resource: %s [%d]  D-pad Up/Down: +/-5  A: place  X: remove",
                    World::ResourceTypeToString(m_activeResourceType), m_resourceAmount);
            } else {
                sprintf_s(resInfo, "Press RB to select resource type, then A to place on tile");
            }
            m_textManager->DrawTextToScreen(resInfo, 10.0f, 720.0f - 60.0f, 0xFFFFCC00, 0.22f);
        }
    }

    // Render GridMenu (submits to queue via renderQueue)
    if (m_gridMenu) {
        m_gridMenu->SetRenderQueue(renderQueue);
        if (m_gridMenu->IsVisible()) {
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
                m_textManager->DrawTextCenteredToScreen(titleText, 640.0f, 50.0f, 0xFFFFFFFF, 0.35f);
            }
            // Show section info above grid cells
            char sectionText[64];
            sprintf_s(sectionText, "Section %d / %d", m_gridMenu->GetCurrentPage() + 1, m_gridMenu->GetTotalPages());
            m_textManager->DrawTextCenteredToScreen(sectionText, 640.0f, 145.0f, 0xFFFFFFFF, 0.20f);
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
            m_textManager->DrawTextToScreen("Select", 444.0f, 573.0f, 0xFF44FF44, 0.22f);
            // Bottom-right: Close + button_B (cell end - 5px = 871, 15px gap from Close to B)
            m_textManager->DrawTextToScreen("Close", 764.0f, 573.0f, 0xFFFF4444, 0.22f);
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
            m_weightMenu->Render();
        }
    }

    // Active sprite preview in top-left corner
    if (m_mapEditor && m_spriteRenderer && renderQueue) {
        int tileIdx = m_mapEditor->GetCurrentTileIndex();
        if (tileIdx >= 0) {
            TextureRegistry& reg = TextureRegistry::instance();
            const char* atlasName = (m_currentLayer == World::Objects)
                ? "maptiles" : "ground";
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
                    cmd.textureID = 8;
                    cmd.blendMode = 1;
                    cmd.layer = LAYER_UI;
                    cmd.depth = 200;
                    cmd.sortKey = Graphics::BuildSortKey(LAYER_UI, 1, SHADER_UI, 8, 200);
                    renderQueue->Submit(cmd);
                }
            }
        }
    }

    // Deposit preview: render building sprite at cursor position
    if (m_currentState == STATE_DEPOSIT_PREVIEW && m_mapEditor && m_spriteRenderer && renderQueue) {
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        float wx, wy;
        coords.NodeTileToWorld(m_phantomTileX, m_phantomTileY, wx, wy);

        std::tr1::shared_ptr<SpriteAtlas> maptiles = TextureRegistry::instance().getAtlas("maptiles");
        if (maptiles && m_depositBuildingSpriteIdx >= 0 && m_depositBuildingSpriteIdx < (int)maptiles->GetRegionCount()) {
            const SpriteRegion* region = maptiles->GetRegion(m_depositBuildingSpriteIdx);
            if (region) {
                float previewW = 64.0f;
                float previewH = 64.0f;
                // Semi-transparent preview before confirmation, solid after
                D3DCOLOR previewColor = m_depositConfirmPending ? 0xFFFFFFFF : 0xAAFFFFFF;

                Graphics::RenderCommand cmd = {};
                cmd.x = wx + coords.GetNodeWidth() * 0.5f - previewW * 0.5f;
                cmd.y = wy - previewH * 0.5f;
                cmd.width = previewW;
                cmd.height = previewH;
                cmd.u0 = region->u0; cmd.v0 = region->v0;
                cmd.u1 = region->u1; cmd.v1 = region->v1;
                cmd.color = previewColor;
                cmd.shaderID = SHADER_TERRAIN;
                cmd.textureID = 9;
                cmd.blendMode = 1;
                cmd.layer = 0;
                cmd.depth = static_cast<WORD>(0.99f * 65535.0f);
                renderQueue->Submit(cmd);
            }
        }
    }
}

void EditorScene::RenderOverlay() {
    if (!m_radialMenu || !m_radialMenu->IsVisible()) return;
    if (!m_renderer || !m_shaderManager || !m_spriteRenderer) return;

    m_radialMenu->Render();

    LPDIRECT3DDEVICE9 device = m_renderer->GetDevice();
    if (!device) return;

    m_radialMenu->RenderIconsDirect(device, m_shaderManager, m_spriteRenderer->GetVertexDeclaration());
}

void EditorScene::OnEnter() {
    OutputDebugStringA("[EditorScene] OnEnter\n");
}

void EditorScene::OnExit() {
    OutputDebugStringA("[EditorScene] OnExit\n");
}

void EditorScene::BindGridMenuTextures(LPDIRECT3DTEXTURE9 bgTexture, LPDIRECT3DTEXTURE9 cellTexture, LPDIRECT3DTEXTURE9 atlasTexture)
{
    if (m_gridMenu) {
        m_gridMenu->SetTextures(bgTexture, cellTexture, atlasTexture);
        if (m_spriteRenderer) {
            m_spriteRenderer->SetTextureSlot(6, bgTexture);
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
        std::tr1::shared_ptr<SpriteAtlas> maptiles = registry.getAtlas("maptiles");
        LPDIRECT3DTEXTURE9 maptilesTex = (maptiles ? maptiles->GetTexture() : NULL);
        m_gridMenu->SetTextures(maptilesTex, maptilesTex, atlasTex);
        m_gridMenu->SetIconAtlas(atlas);
        if (m_spriteRenderer) {
            m_spriteRenderer->SetTextureSlot(6, maptilesTex);
            m_spriteRenderer->SetTextureSlot(7, maptilesTex);
            m_spriteRenderer->SetTextureSlot(8, atlasTex);
        }

        // Set bg/cell UVs from maptiles atlas
        GridMenu::TileUV bgUV = {0,0,1,1}, cellUV = {0,0,1,1};
        if (maptiles) {
            uint32_t bgIdx = maptiles->GetIndex("menu_background_cell");
            if (bgIdx != 0xFFFFFFFF) {
                const SpriteRegion* reg = maptiles->GetRegion(bgIdx);
                if (reg) { bgUV.u0 = reg->u0; bgUV.v0 = reg->v0; bgUV.u1 = reg->u1; bgUV.v1 = reg->v1; }
                else { OutputDebugStringA("[EditorScene LoadGridMenuAtlas] WARNING: 'menu_background_cell' found but GetRegion NULL\n"); }
            } else {
                OutputDebugStringA("[EditorScene LoadGridMenuAtlas] WARNING: 'menu_background_cell' NOT FOUND\n");
            }
            uint32_t cellIdx = maptiles->GetIndex("menu_cell");
            if (cellIdx != 0xFFFFFFFF) {
                const SpriteRegion* reg = maptiles->GetRegion(cellIdx);
                if (reg) { cellUV.u0 = reg->u0; cellUV.v0 = reg->v0; cellUV.u1 = reg->u1; cellUV.v1 = reg->v1; }
                else { OutputDebugStringA("[EditorScene LoadGridMenuAtlas] WARNING: 'menu_cell' found but GetRegion NULL\n"); }
            } else {
                OutputDebugStringA("[EditorScene LoadGridMenuAtlas] WARNING: 'menu_cell' NOT FOUND\n");
            }
        }
    m_gridMenu->SetBackgroundUV(bgUV);
    m_gridMenu->SetCellUV(cellUV);
    m_gridMenu->SetCellSpacing(119.0f, 74.0f);

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
    m_gridMenu->SetTextures(atlasTex, atlasTex, atlasTex);
    m_gridMenu->SetIconAtlas(maptiles);
    if (m_spriteRenderer) {
        m_spriteRenderer->SetTextureSlot(6, atlasTex);
        m_spriteRenderer->SetTextureSlot(7, atlasTex);
        m_spriteRenderer->SetTextureSlot(8, atlasTex);
    }

    // Set bg/cell UVs from maptiles atlas
    GridMenu::TileUV bgUV = {0,0,1,1}, cellUV = {0,0,1,1};
    {
        uint32_t bgIdx = maptiles->GetIndex("menu_background_cell");
        if (bgIdx != 0xFFFFFFFF) {
            const SpriteRegion* reg = maptiles->GetRegion(bgIdx);
            if (reg) { bgUV.u0 = reg->u0; bgUV.v0 = reg->v0; bgUV.u1 = reg->u1; bgUV.v1 = reg->v1; }
            else { OutputDebugStringA("[EditorScene LoadGridMenuGroup] WARNING: 'menu_background_cell' found but GetRegion NULL\n"); }
        } else {
            OutputDebugStringA("[EditorScene LoadGridMenuGroup] WARNING: 'menu_background_cell' NOT FOUND\n");
        }
        uint32_t cellIdx = maptiles->GetIndex("menu_cell");
        if (cellIdx != 0xFFFFFFFF) {
            const SpriteRegion* reg = maptiles->GetRegion(cellIdx);
            if (reg) { cellUV.u0 = reg->u0; cellUV.v0 = reg->v0; cellUV.u1 = reg->u1; cellUV.v1 = reg->v1; }
            else { OutputDebugStringA("[EditorScene LoadGridMenuGroup] WARNING: 'menu_cell' found but GetRegion NULL\n"); }
        } else {
            OutputDebugStringA("[EditorScene LoadGridMenuGroup] WARNING: 'menu_cell' NOT FOUND\n");
        }
    }
    m_gridMenu->SetBackgroundUV(bgUV);
    m_gridMenu->SetCellUV(cellUV);
    m_gridMenu->SetCellSpacing(110.0f, 74.0f);

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

    char buf[128];
    sprintf_s(buf, "[EditorScene] Loaded group '%s' from maptiles (%d sprites)\n", groupName, (int)uvs.size());
    OutputDebugStringA(buf);
}

void EditorScene::LoadResourceIcons() {
    if (!m_gridMenu) return;

    TextureRegistry& registry = TextureRegistry::instance();
    std::tr1::shared_ptr<SpriteAtlas> maptiles = registry.getAtlas("maptiles");
    if (!maptiles) {
        OutputDebugStringA("[EditorScene] maptiles atlas not found for resource icons\n");
        return;
    }

    LPDIRECT3DTEXTURE9 atlasTex = maptiles->GetTexture();
    m_gridMenu->SetTextures(atlasTex, atlasTex, atlasTex);
    m_gridMenu->SetIconAtlas(maptiles);
    if (m_spriteRenderer) {
        m_spriteRenderer->SetTextureSlot(6, atlasTex);
        m_spriteRenderer->SetTextureSlot(7, atlasTex);
        m_spriteRenderer->SetTextureSlot(8, atlasTex);
    }

    GridMenu::TileUV bgUV = {0,0,1,1}, cellUV = {0,0,1,1};
    {
        uint32_t bgIdx = maptiles->GetIndex("menu_background_cell");
        if (bgIdx != 0xFFFFFFFF) {
            const SpriteRegion* reg = maptiles->GetRegion(bgIdx);
            if (reg) { bgUV.u0 = reg->u0; bgUV.v0 = reg->v0; bgUV.u1 = reg->u1; bgUV.v1 = reg->v1; }
        }
        uint32_t cellIdx = maptiles->GetIndex("menu_cell");
        if (cellIdx != 0xFFFFFFFF) {
            const SpriteRegion* reg = maptiles->GetRegion(cellIdx);
            if (reg) { cellUV.u0 = reg->u0; cellUV.v0 = reg->v0; cellUV.u1 = reg->u1; cellUV.v1 = reg->v1; }
        }
    }
    m_gridMenu->SetBackgroundUV(bgUV);
    m_gridMenu->SetCellUV(cellUV);
    m_gridMenu->SetCellSpacing(119.0f, 74.0f);

    // Try loading ResourceIcons group, fall back to individual sprite names
    const std::vector<uint32_t>* groupIndices = maptiles->GetGroup(kResourceGroupName);
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
    } else {
        // Fallback: look up individual resource icons by name
        for (int i = 1; i <= kResourceTypeCount; ++i) {
            World::ResourceType rt = static_cast<World::ResourceType>(i);
            const char* iconName = World::ResourceTypeToIconName(rt);
            if (!iconName || !iconName[0]) continue;

            uint32_t spriteIdx = maptiles->GetIndex(iconName);
            if (spriteIdx == 0xFFFFFFFF) {
                char buf[128];
                sprintf_s(buf, "[EditorScene] Resource icon '%s' not found in maptiles atlas\n", iconName);
                OutputDebugStringA(buf);
                continue;
            }

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

    char buf[128];
    sprintf_s(buf, "[EditorScene] Loaded resource icons into GridMenu (%d sprites)\n", (int)uvs.size());
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
