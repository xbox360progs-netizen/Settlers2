#include "stdafx.h"
#include "EditorScene.h"
#include "../Editor/MapEditor.h"
#include "../Graphics/SpriteAtlas.h"
#include "../Graphics/SpriteRenderer.h"
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
    , m_radialMenu(nullptr)
    , m_gridMenu(nullptr)
    , m_mapEditor(nullptr)
    , m_currentLayer(World::Ground)
    , m_objectAtlasIndex(0)
    , m_yButtonWasPressed(false)
    , m_fps(0)
    , m_frameCount(0)
    , m_lastFpsTime(0)

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

    OutputDebugStringA("[EditorScene] All dependencies OK, loading RadialMenu...\n");
    
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
        m_gridMenu = new GridMenu(m_renderer->GetDevice());
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

	if (!menuActive) {
		// Toggle GridMenu with RB
		if (gamepad->IsButtonPressed(Input::GP_RB)) {
			if (!m_gridMenu) {
				m_gridMenu = new GridMenu(m_renderer->GetDevice());
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
        if (m_mapEditor) {
            m_mapEditor->SetLayer(m_currentLayer);
            if (m_currentLayer != World::Objects) {
                m_yButtonWasPressed = false;
            }

            // ��������� ����������
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
	

	} else {
// LB opens RadialMenu
		if (gamepad->IsButtonPressed(Input::GP_LB)) {
			if (m_radialMenu) {
				if (!m_radialMenu->IsVisible()) {
					m_radialMenu->Show(640.0f, 360.0f);
				}
			}
			if (m_gridMenu && m_gridMenu->IsVisible()) {
				m_gridMenu->Hide();
			}
		}
		// RB opens GridMenu; hide RadialMenu if visible
		if (gamepad->IsButtonPressed(Input::GP_RB)) {
			if (!m_gridMenu) {
				m_gridMenu = new GridMenu(m_renderer->GetDevice());
				if (m_gridMenu && m_gridMenu->Initialize()) {
					if (m_currentLayer == World::Objects) {
						m_objectAtlasIndex = 0;
						LoadGridMenuAtlas(kObjectAtlasNames[m_objectAtlasIndex]);
					} else {
						LoadGridMenuAtlas("ground");
					}
				}
			}
			if (m_gridMenu && !m_gridMenu->IsVisible()) {
				if (m_currentLayer == World::Objects) {
					LoadGridMenuAtlas(kObjectAtlasNames[m_objectAtlasIndex]);
				} else {
					LoadGridMenuAtlas("ground");
				}
				m_gridMenu->Show(640.0f, 360.0f);
			}
			if (m_radialMenu && m_radialMenu->IsVisible()) {
				m_radialMenu->Hide();
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
			// Y button - cycle object atlases (only in Objects layer) with debounce
			if (m_currentLayer == World::Objects) {
				if (gamepad->IsButtonDown(Input::GP_Y)) {
					if (!m_yButtonWasPressed) {
						CycleObjectAtlas();
						m_yButtonWasPressed = true;
					}
				} else {
					m_yButtonWasPressed = false;
				}
			}
			// Shoulder triggers to navigate windows
			if (gamepad->IsButtonPressed(Input::GP_LB)) {
				m_gridMenu->PrevWindow();
			}
			if (gamepad->IsButtonPressed(Input::GP_RB)) {
				m_gridMenu->NextWindow();
			}
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

    // Render map through MapEditor
    if (m_mapEditor) {
        m_mapEditor->Render();
    }

    // Reset to orthographic projection for UI rendering (separate from world rendering)
    if (m_radialMenu && m_radialMenu->IsVisible() || m_gridMenu && m_gridMenu->IsVisible()) {
        D3DXMATRIX ortho;
        D3DXMatrixOrthoOffCenterLH(&ortho, 0.0f, 1280.0f, 720.0f, 0.0f, 0.0f, 1.0f);
        if (m_spriteRenderer) {
            m_spriteRenderer->SetProjection(ortho);
            m_spriteRenderer->Flush(); // Apply new matrix separately for UI
        }
    }

    if (m_radialMenu && m_radialMenu->IsVisible()) {
        m_spriteRenderer->Flush();
        m_radialMenu->Render();
        m_radialMenu->RenderIcons(m_spriteRenderer);
        m_spriteRenderer->Flush();
    }
    if (m_gridMenu && m_gridMenu->IsVisible()) {
        m_spriteRenderer->Flush();
        m_gridMenu->Render(m_spriteRenderer);
        m_spriteRenderer->Flush();
    }

    // Flush to ensure all sprites are rendered before UI text
    if (m_spriteRenderer) {
        m_spriteRenderer->Flush();
    }

    // Draw FPS counter (top-left)
    if (m_textManager) {
        m_textManager->Begin();
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
        m_textManager->RenderScreen();
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
