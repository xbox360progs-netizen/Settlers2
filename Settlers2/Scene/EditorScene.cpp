#include "stdafx.h"
#include "EditorScene.h"
#include "../Input/InputController.h"
#include "../Logic/CoordinateSystem.h"
#include "../Logic/MapConstants.h"
#include "../World/TileLayer.h"
#include "../Graphics/RadialMenu.h"
#include "../Graphics/TextManager.h"
#include "../Graphics/GridMenu.h"
#include "../Graphics/Texture.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/BinFileManager.h"
#include "../Input/InputManager.h"
#include "../Editor/MapEditor.h"
#include "../Graphics/SpriteAtlas.h"
#include "../Graphics/TextureRegistry.h"
#include <iostream>
#include <cstdio>

namespace Scene {

const char* EditorScene::kObjectAtlasNames[] = {
    "icon_tree",
    "icon_mountains",
    "icon_mountains_water",
    "icon_rocks"
};
const int EditorScene::kObjectAtlasCount = 4;

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
    , m_objectAtlasIndex(0)
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
        m_camera->Initialize(1280.0f, 720.0f);
        OutputDebugStringA("[EditorScene] Camera initialized\n");
    }

    // Initialize InputController for world coordinate translation
    m_inputController = new Logic::InputController();
    if (m_inputController && m_inputManager) {
        m_inputController->Initialize(m_camera, m_inputManager->GetGamepad());
        OutputDebugStringA("[EditorScene] InputController initialized\n");
    }

    // Initialize WeightMenu for D-pad weight selection
    m_weightMenu = new UI::WeightMenu();
    if (m_weightMenu) {
        // TODO: Load menu_background.png and dpad_cross.png textures
        // For now, initialize with nullptr (textures can be loaded later)
        m_weightMenu->Initialize(nullptr, nullptr, m_spriteRenderer, m_textManager);
        OutputDebugStringA("[EditorScene] WeightMenu initialized\n");
    }
    
    // Create and initialize RadialMenu
    m_radialMenu = new RadialMenu(m_renderer->GetDevice(), m_renderer->GetShaderManager(), m_binFileManager);
    if (m_radialMenu) {
        m_radialMenu->Initialize();

// Add menu items for layers (name, typeId, spriteIndex for icon_menu atlas)
        std::vector<RadialMenu::MenuItem> items;
        items.push_back(RadialMenu::MenuItem(std::wstring(L"Roads"), World::Roads, 0));
        items.push_back(RadialMenu::MenuItem(std::wstring(L"Nodes"), World::Nodes, 1));
        items.push_back(RadialMenu::MenuItem(std::wstring(L"Placement"), World::Placement, 2));
        items.push_back(RadialMenu::MenuItem(std::wstring(L"Resources"), World::Resources, 3));
        items.push_back(RadialMenu::MenuItem(std::wstring(L"Ground"), World::Ground, 4));
        items.push_back(RadialMenu::MenuItem(std::wstring(L"Objects"), World::Objects, 5));
        items.push_back(RadialMenu::MenuItem(std::wstring(L"Overlay"), World::Overlay, 6));
        m_radialMenu->SetItems(items);
    }

    // === TEXTURE SETUP ===

    // 1.   ,     (SingleSprites  textures.ini)
    TextureRegistry& registry = TextureRegistry::instance();
    registry.initializeFromManifest("game:\\Media\\Config\\textures.ini", "UI");
    registry.initializeFromManifest("game:\\Media\\Config\\textures.ini", "AtlasTextures");
    registry.initializeFromManifest("game:\\Media\\Config\\textures.ini", "ResourceIcons");

    // 2. ,     .    ,   ,    .
    LPDIRECT3DTEXTURE9 bgTexture    = registry.getTextureOrLoad("menu_bd");
    LPDIRECT3DTEXTURE9 cellTexture  = registry.getTextureOrLoad("menu_cell");
    LPDIRECT3DTEXTURE9 groundTexture = registry.getTextureOrLoad("ground");
    LPDIRECT3DTEXTURE9 iconMenuTexture = registry.getTextureOrLoad("icon_menu");

    // 3.    UV-   (   )
    std::tr1::shared_ptr<SpriteAtlas> groundAtlas = registry.getAtlas("ground");

    // 
    char logMsg[256];
    _snprintf(logMsg, sizeof(logMsg), "[EditorScene] Final Textures: Bg=%p, Cell=%p, Ground=%p, IconMenu=%p\n", 
              (void*)bgTexture, (void*)cellTexture, (void*)groundTexture, (void*)iconMenuTexture);
    OutputDebugStringA(logMsg);

    // Initialize GridMenu with textures
    if (!m_gridMenu && m_renderer) {
        m_gridMenu = new GridMenu();
        if (m_gridMenu->Initialize()) {
            m_gridMenu->SetTextures(bgTexture, cellTexture, groundTexture);
            
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
        World::Map* map = new World::Map(Editor::MapEditor::GRID_WIDTH, Editor::MapEditor::GRID_HEIGHT,Editor::MapEditor::GRID_WIDTH * 2, Editor::MapEditor::GRID_HEIGHT * 2);
        m_mapEditor->Initialize(map, m_renderer, m_inputManager, m_renderer->GetDevice());
        m_mapEditor->SetSpriteRenderer(m_spriteRenderer);
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

	// Toggle WeightMenu with LT in WEIGHTS mode
	if (m_editorMode == MODE_WEIGHTS && gamepad->IsButtonPressed(Input::GP_LT)) {
		if (m_weightMenu) {
			if (m_weightMenuVisible) {
				m_weightMenu->Close();
				m_weightMenuVisible = false;
			} else {
				m_weightMenu->Open();
				m_weightMenuVisible = true;
			}
		}
	}

	// Handle D-pad input for weight selection when menu is visible
	if (m_weightMenuVisible && m_weightMenu) {
		if (gamepad->IsButtonPressed(Input::GP_DPadUp)) {
			m_activeWeight = World::Weight_Block;  // 3
			m_weightMenu->Close();
			m_weightMenuVisible = false;
		}
		else if (gamepad->IsButtonPressed(Input::GP_DPadDown)) {
			m_activeWeight = World::Weight_Deep;  // 0
			m_weightMenu->Close();
			m_weightMenuVisible = false;
		}
		else if (gamepad->IsButtonPressed(Input::GP_DPadLeft)) {
			m_activeWeight = World::Weight_Shallow;  // 1
			m_weightMenu->Close();
			m_weightMenuVisible = false;
		}
		else if (gamepad->IsButtonPressed(Input::GP_DPadRight)) {
			m_activeWeight = World::Weight_Land;  // 2
			m_weightMenu->Close();
			m_weightMenuVisible = false;
		}
	}

	if (!menuActive && !m_weightMenuVisible) {
		// Toggle GridMenu with RB
		if (gamepad->IsButtonPressed(Input::GP_RB)) {
			if (!m_gridMenu) {
				m_gridMenu = new GridMenu();
				if (m_gridMenu->Initialize()) {
					// Load atlas based on current layer
					if (m_currentLayer == World::Objects) {
						m_objectAtlasIndex = 0;
						LoadGridMenuAtlas(kObjectAtlasNames[m_objectAtlasIndex]);
					} else {
						LoadGridMenuAtlas("ground");
					}
				}
				m_gridMenu->Show(640.0f, 360.0f);
			} else if (m_gridMenu->IsVisible()) {
				m_gridMenu->Hide();
			} else {
				// Show GridMenu with appropriate atlas for current layer
				if (m_currentLayer == World::Objects) {
					LoadGridMenuAtlas(kObjectAtlasNames[m_objectAtlasIndex]);
				} else {
					LoadGridMenuAtlas("ground");
				}
				m_gridMenu->Show(640.0f, 360.0f);
			}
		}
	}

	// Update InputController for world coordinate translation
	if (m_inputController) {
		m_inputController->Update();
		
		// Get world cursor position
		float worldX, worldY;
		m_inputController->GetWorldCursor(worldX, worldY);
		
		// Convert screen cursor position to tile selection
		float screenX = (worldX + m_camera->GetPosX()) / m_camera->GetZoom();
		float screenY = (worldY + m_camera->GetPosY()) / m_camera->GetZoom();
		
		// Get tile under cursor using Grid Picking
		if (m_mapEditor && m_mapEditor->GetMap()) {
			int tileX, tileY;
			if (m_mapEditor->GetMap()->GetTileUnderMouse(screenX, screenY, m_camera, m_currentLayer, tileX, tileY)) {
				m_selectedTileX = tileX;
				m_selectedTileY = tileY;
				m_hasSelection = true;
				
				// Update phantom tile position when in PLACING state
				if (m_currentState == STATE_PLACING) {
					m_phantomTileX = tileX;
					m_phantomTileY = tileY;
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
						// Paint weight at cursor position using staggered grid math
						float worldX, worldY;
						m_inputController->GetWorldCursor(worldX, worldY);
						
						// Staggered grid calculation (40x40 Objects layer)
						// Tile height = 72, Tile width = 119, Offset = 59.5
						float row = worldY / 72.0f;
						float offsetX = (fmodf(row, 2.0f) > 0.5f) ? 59.5f : 0.0f;
						float col = (worldX - offsetX) / 119.0f;
						
						int tileX = static_cast<int>(col);
						int tileY = static_cast<int>(row);
						
						// Set weight on the map
						if (m_mapEditor && m_mapEditor->GetMap()) {
							m_mapEditor->GetMap()->SetNodeWeight(tileX, tileY, m_activeWeight);
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
			m_camera->Move(stickX * moveSpeed, -stickY * moveSpeed); // Y inverted for screen coords
		}
		
		// Right stick: zoom camera
		float rightX, rightY;
		gamepad->GetRightStick(rightX, rightY);
		if (fabsf(rightY) > 0.1f) {
			float zoomSpeed = 1.0f * deltaTime;
			m_camera->Zoom(-rightY * zoomSpeed); // Push up to zoom in
		}
		
		m_camera->Update();
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
		// Y button - cycle object atlases (only in Objects layer)
		if (gamepad->IsButtonPressed(Input::GP_Y) && m_currentLayer == World::Objects) {
			CycleObjectAtlas();
		}
		// Shoulder triggers to navigate windows
		if (gamepad->IsButtonPressed(Input::GP_LB)) {
			m_gridMenu->PrevWindow();
		}
		if (gamepad->IsButtonPressed(Input::GP_RB)) {
			m_gridMenu->NextWindow();
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

        // Update map editor visibility based on current layer
        if (m_mapEditor) {
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
                m_mapEditor->SetObjectAtlas(kObjectAtlasNames[m_objectAtlasIndex]);
            }
        }
    }
}

	// Update MapEditor (camera movement, painting) only when no menu active
	if (m_mapEditor && !menuActive) {
		m_mapEditor->Update(deltaTime);

		// X button - reset to first tile from ground
		if (gamepad->IsButtonPressed(Input::GP_X)) {
			m_mapEditor->SetTileByIndex(0);
		}

		// A button - paint current tile while held
		if (gamepad->IsButtonDown(Input::GP_A)) {
			m_mapEditor->PaintCurrentTile();
		}
	}
}

void EditorScene::Render() {
    if (!m_renderer) return;
    
    // FIX: Clear entire screen at start of render loop to fix icon tails (Xbox 360 EDRAM issue)
    if (m_renderer->GetDevice()) {
        m_renderer->GetDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
    }
    
    m_renderer->Clear(D3DCOLOR_XRGB(50, 50, 50));

    // === STEP 1: Update global camera matrices (once per frame) ===
    if (m_shaderManager && m_camera) {
        m_camera->Update();
        m_shaderManager->UpdateGlobalMatrices(&m_camera->GetViewMatrix(), &m_camera->GetProjectionMatrix());
    }

    // === STEP 2: Render map through MapEditor (fills command queue) ===
    if (m_mapEditor) {
        m_mapEditor->Render();
    }

    // === STEP 2.5: Render selection hex (white outline) ===
    if (m_hasSelection && m_mapEditor && m_mapEditor->GetMap()) {
        float tileCenterX, tileCenterY;
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        
        if (m_currentLayer == World::Ground) {
            coords.GroundTileToWorldCenter(m_selectedTileX, m_selectedTileY, tileCenterX, tileCenterY);
        } else {
            coords.NodeTileToWorld(m_selectedTileX, m_selectedTileY, tileCenterX, tileCenterY);
        }
        
        // Submit selection hex as a render command
        // For now, we'll just render a simple white outline using the sprite renderer
        // This should be replaced with a proper hex outline shader later
        if (m_spriteRenderer) {
            // Create a simple white outline effect
            // This is a placeholder - should use a proper selection shader
            D3DXVECTOR3 selectionPos(tileCenterX, tileCenterY, 0.1f);
            // Submit to render queue (implementation depends on your sprite renderer API)
        }
    }

    // === STEP 2.6: Render phantom resource (alpha 0.5) ===
    if (m_currentState == STATE_PLACING && m_activeResourceType != World::ResourceType_None) {
        float phantomX, phantomY;
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        
        if (m_currentLayer == World::Ground) {
            coords.GroundTileToWorldCenter(m_phantomTileX, m_phantomTileY, phantomX, phantomY);
        } else {
            coords.NodeTileToWorld(m_phantomTileX, m_phantomTileY, phantomX, phantomY);
        }
        
        // Render phantom resource with alpha 0.5
        // This is a placeholder - should use proper sprite rendering with alpha
        if (m_spriteRenderer && m_shaderManager) {
            // TODO: Get texture for the active resource type
            // For now, just a placeholder
            D3DXVECTOR3 phantomPos(phantomX, phantomY, 0.05f);
            // Submit render command with alpha = 0.5
        }
    }

    // === STEP 2.7: Render placed resources on map ===
    if (m_mapEditor && m_mapEditor->GetMap()) {
        World::Map* map = m_mapEditor->GetMap();
        int layerWidth = map->GetWidth() * 2;  // Objects layer is 40x40
        int layerHeight = map->GetHeight() * 2;
        
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        
        for (int y = 0; y < layerHeight; y++) {
            for (int x = 0; x < layerWidth; x++) {
                const World::ResourceNode& node = map->GetResourceNode(x, y);
                if (node.type != World::ResourceType_None && node.isVisible) {
                    float worldX, worldY;
                    coords.NodeTileToWorld(x, y, worldX, worldY);
                    
                    // TODO: Render resource icon at (worldX, worldY)
                    // Placeholder for resource rendering
                    if (m_spriteRenderer) {
                        D3DXVECTOR3 resourcePos(worldX, worldY, 0.02f);
                        // Submit render command for resource icon
                    }
                    
                    // Render amount text next to resource
                    if (m_textManager && node.amount > 0) {
                        char amountStr[32];
                        sprintf_s(amountStr, "%d", node.amount);
                        // Convert world coordinates to screen coordinates for text
                        float screenX, screenY;
                        m_camera->WorldToScreen(worldX, worldY, screenX, screenY);
                        m_textManager->DrawTextToScreen(amountStr, screenX + 20.0f, screenY, 0xFFFFFFFF, 0.8f);
                    }
                }
            }
        }
    }

    // === STEP 2.8: Render weight debug overlay (in WEIGHTS mode) ===
    if (m_editorMode == MODE_WEIGHTS && m_mapEditor && m_mapEditor->GetMap()) {
        World::Map* map = m_mapEditor->GetMap();
        int layerWidth = map->GetWidth() * 2;  // Objects layer is 40x40
        int layerHeight = map->GetHeight() * 2;

        CoordinateSystem& coords = CoordinateSystem::GetInstance();

        // Color coding for weights:
        // 0 (Deep): 0xCC0000FF (Blue)
        // 1 (Shallow): 0xCC00FFFF (Cyan)
        // 2 (Land): 0xCC00FF00 (Green)
        // 3 (Block): 0xCCFF0000 (Red)

        for (int y = 0; y < layerHeight; y++) {
            for (int x = 0; x < layerWidth; x++) {
                BYTE weight = map->GetNodeWeight(x, y);

                // Skip default land weight to reduce clutter
                if (weight == World::Weight_Land) {
                    continue;
                }

                float worldX, worldY;
                coords.NodeTileToWorld(x, y, worldX, worldY);

                // Determine color based on weight
                D3DCOLOR color = 0xCC00FF00; // Default green (Land)
                switch (weight) {
                    case World::Weight_Deep:    color = 0xCC0000FF; break; // Blue
                    case World::Weight_Shallow: color = 0xCC00FFFF; break; // Cyan
                    case World::Weight_Land:    color = 0xCC00FF00; break; // Green
                    case World::Weight_Block:   color = 0xCCFF0000; break; // Red
                }

                // Render small square at tile center to indicate weight
                // This is a placeholder - should use dynamic vertex buffer for performance
                if (m_spriteRenderer) {
                    D3DXVECTOR3 weightPos(worldX, worldY, 0.01f);
                    // TODO: Submit render command with color
                    // For now, this is just a placeholder
                }
            }
        }
    }

    // === STEP 3: Render UI elements (RadialMenu, GridMenu, WeightMenu) ===
    // These submit commands with isUI=true and depth=0.0f
    if (m_radialMenu && m_radialMenu->IsVisible()) {
        m_radialMenu->Render();
        m_radialMenu->RenderIcons(m_spriteRenderer);
    }
    
    if (m_gridMenu && m_gridMenu->IsVisible()) {
        m_gridMenu->Render(m_spriteRenderer);
    }

    // Render WeightMenu when visible
    // TEMPORARILY COMMENTED OUT TO ISOLATE GPU HANG
    // if (m_weightMenuVisible && m_weightMenu) {
    //     m_weightMenu->Render();
    // }

    // Draw FPS counter (top-left)
    if (m_textManager) {
        char fpsText[64];
        sprintf(fpsText, "FPS: %d", m_fps);
        m_textManager->DrawTextToScreen(fpsText, 10.0f, 10.0f, 0xFF00FF00, 0.25f);

        // Draw current layer name (bottom-left)
        const char* layerNames[] = { "Roads", "Nodes", "Placement", "Resources" , "Ground", "Objects", "Overlay"};
        int layerIdx = static_cast<int>(m_currentLayer);
        if (layerIdx >= 0 && layerIdx < 7) {
            m_textManager->DrawTextToScreen("Layer:", 10.0f, m_renderer->GetScreenHeight() - 40.0f, 0xFFAAAAAA, 0.25f);
            m_textManager->DrawTextToScreen(layerNames[layerIdx], 100.0f, m_renderer->GetScreenHeight() - 40.0f, 0xFFFFFFFF, 0.25f);
        }
        // Text is now submitted to queue via DrawTextToScreen, no need to call RenderScreen()
    }
    
    // === STEP 4: MASTER LOOP - Execute all queued render commands ===
    // Camera ViewProj already set via UpdateGlobalMatrices
    if (m_shaderManager && m_spriteRenderer) {
        LPDIRECT3DVERTEXBUFFER9 pVB = m_spriteRenderer->GetVertexBuffer();
        LPDIRECT3DINDEXBUFFER9 pIB = m_spriteRenderer->GetIndexBuffer();
        LPDIRECT3DVERTEXDECLARATION9 pDecl = m_spriteRenderer->GetVertexDeclaration();
        
        // Pass ViewProj to ExecuteQueue (used for per-command isUI override)
        D3DXMATRIX viewProj;
        if (m_camera) {
            D3DXMatrixMultiply(&viewProj, &m_camera->GetViewMatrix(), &m_camera->GetProjectionMatrix());
        } else {
            D3DXMatrixIdentity(&viewProj);
        }
        
        if (pVB && pIB && pDecl) {
            m_shaderManager->ExecuteQueue(pVB, pIB, pDecl, 32, &viewProj);
        }
    }

    m_renderer->EndFrame();
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
        LPDIRECT3DTEXTURE9 bgTex = registry.getTextureOrLoad("menu_bd");
        LPDIRECT3DTEXTURE9 cellTex = registry.getTextureOrLoad("menu_cell");
        m_gridMenu->SetTextures(bgTex, cellTex, atlasTex);
        m_gridMenu->SetIconAtlas(atlas);

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

void EditorScene::CycleObjectAtlas() {
    m_objectAtlasIndex = (m_objectAtlasIndex + 1) % kObjectAtlasCount;
    const char* atlasName = kObjectAtlasNames[m_objectAtlasIndex];
    LoadGridMenuAtlas(atlasName);
    if (m_mapEditor) {
        m_mapEditor->SetObjectAtlas(atlasName);
    }
    char buf[128];
    sprintf_s(buf, "[EditorScene] CycleObjectAtlas: index=%d, name='%s'\n", m_objectAtlasIndex, atlasName);
    OutputDebugStringA(buf);
}

} // namespace Scene
