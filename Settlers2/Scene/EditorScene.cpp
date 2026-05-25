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
                m_gridMenu->Show(640.0f, 280.0f);
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
                m_gridMenu->Show(640.0f, 280.0f);
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
					// Update phantom tile position when in PLACING state
					if (m_currentState == STATE_PLACING) {
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
				m_mapEditor->SetTileByIndex(selectedIndex);
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
		// Shoulder triggers to navigate pages (Objects) or windows (Ground)
		if (gamepad->IsButtonPressed(Input::GP_LB)) {
			if (m_currentLayer == World::Objects) {
				m_gridMenu->PrevPage();
			} else {
				m_gridMenu->PrevWindow();
			}
		}
		if (gamepad->IsButtonPressed(Input::GP_RB)) {
			if (m_currentLayer == World::Objects) {
				m_gridMenu->NextPage();
			} else {
				m_gridMenu->NextWindow();
			}
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
                    break;
                case World::Objects:
                    m_mapEditor->SetShowObjects(true);
                    m_mapEditor->SetShowOverlay(false);
                    break;
                case World::Overlay:
                    m_mapEditor->SetShowObjects(true);
                    m_mapEditor->SetShowOverlay(true);
                    break;
                default:
                    m_mapEditor->SetShowObjects(true);
                    m_mapEditor->SetShowOverlay(true);
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
        m_textManager->DrawTextToScreen(layerText, 1280.0f - 200.0f, 720.0f - 30.0f, 0xFFFFFFFF, 0.25f);
    }

    // Render GridMenu (submits to queue via renderQueue)
    if (m_gridMenu) {
        m_gridMenu->SetRenderQueue(renderQueue);
        if (m_gridMenu->IsVisible()) {
            m_gridMenu->Render();
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

        std::vector<GridMenu::TileUV> uvs;
        uvs.reserve(atlas->GetRegionCount());
        for (uint32_t i = 0; i < atlas->GetRegionCount(); ++i) {
            const SpriteRegion* reg = atlas->GetRegion(i);
            GridMenu::TileUV tu;
            if (reg) { tu.u0 = reg->u0; tu.v0 = reg->v0; tu.u1 = reg->u1; tu.v1 = reg->v1; }
            else { tu.u0 = 0.0f; tu.v0 = 0.0f; tu.u1 = 1.0f; tu.v1 = 1.0f; }
            uvs.push_back(tu);
        }
        m_gridMenu->SetAllTileUVs(uvs);
        m_gridMenu->SetWindowStart(0);
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
