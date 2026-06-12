#include "stdafx.h"
#include "MenuScene.h"
#include <iostream>
#include "SceneManager.h"
#include "LoadingScene.h"
#include "EditorScene.h"
#include "../Graphics/TextureLoader.h"
#include "../Graphics/Texture.h"
#include "../Graphics/TextureRegistry.h"
#include "../Graphics/RenderLayers.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/RenderQueue.h"

using namespace Scene;
#include <d3dx9.h>

// Simple UTF-8 converter for internal use
static std::string WToA(const std::wstring& w) {
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), NULL, 0, NULL, NULL);
    if (sizeNeeded <= 0) return std::string();
    std::string out(sizeNeeded, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &out[0], sizeNeeded, NULL, NULL);
    return out;
}

// Convert string to wide string for TextureLoader
static std::wstring ToWideString(const std::string& s) {
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
    if (sizeNeeded <= 0) return std::wstring();
    std::wstring out(sizeNeeded - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], sizeNeeded);
    return out;
}

MenuScene::MenuScene()
  : Scene("MenuScene"), m_backgroundPath(""), m_text("Settlers 2: Main Menu"),
    m_selectedIndex(0), m_device(NULL), m_spriteRenderer(NULL), m_renderer(NULL), m_gamepad(NULL),
    m_textManager(NULL), m_binFileManager(NULL), m_textureLoader(NULL), m_menuCount(4), m_stickTimer(0.0f),
    m_menuState(MENU_STATE_MAIN) {
  m_menuItems[0] = "New Game";
  m_menuItems[1] = "Map Editor";
  m_menuItems[2] = "Settings";
  m_menuItems[3] = "Exit";
}

MenuScene::~MenuScene() {
}

void MenuScene::Load() {
  m_loaded = true;
  OutputDebugStringA("[MenuScene] Load() called\n");

  // Debug logging for m_spriteRenderer initialization
  if (!m_spriteRenderer) {
    OutputDebugStringA("[MenuScene] WARNING: m_spriteRenderer is NULL in Load()\n");
  } else {
    OutputDebugStringA("[MenuScene] m_spriteRenderer is valid in Load()\n");
  }
}

void MenuScene::ResetTextureState() {
    OutputDebugStringA("[MenuScene::ResetTextureState] START\n");
    TextureRegistry& registry = TextureRegistry::instance();
    OutputDebugStringA("[MenuScene::ResetTextureState] Got registry\n");
    
    // Use getTextureOrLoad to load texture if not already loaded
    LPDIRECT3DTEXTURE9 tex = registry.getTextureOrLoad("menu_background");
    OutputDebugStringA("[MenuScene::ResetTextureState] Got texture\n");
    
    if (tex) {
        m_backgroundTexture.SetTexture(tex);
        if (m_spriteRenderer) {
            m_spriteRenderer->SetTextureSlot(0, tex);
        }
        OutputDebugStringA("[MenuScene::ResetTextureState] Texture set OK\n");
    } else {
        OutputDebugStringA("[MenuScene::ResetTextureState] ERROR no texture\n");
    }
    OutputDebugStringA("[MenuScene::ResetTextureState] END\n");
}

void MenuScene::LoadTextures() {
    OutputDebugStringA("[MenuScene::LoadTextures] START\n");
    ResetTextureState();
    OutputDebugStringA("[MenuScene::LoadTextures] END\n");
}

void MenuScene::Initialize(LPDIRECT3DDEVICE9 device, SpriteRenderer* spriteRenderer) {
  m_device = device;
  m_spriteRenderer = spriteRenderer;
  
  std::cout << "[MenuScene] Initialize called" << std::endl;
  
    
    if (!spriteRenderer) {
        OutputDebugStringA("[MenuScene::Initialize] ERROR: spriteRenderer is NULL!\n");
    }

  LoadTextures();
}

void MenuScene::Initialize(LPDIRECT3DDEVICE9 device, SpriteRenderer* spriteRenderer, Renderer* renderer, Input::Gamepad* gamepad, TextureLoader* textureLoader) {
  m_device = device;
  m_spriteRenderer = spriteRenderer;
  m_renderer = renderer;
  m_gamepad = gamepad;
  m_textureLoader = textureLoader;
  
  char buf[256];
  sprintf(buf, "[MenuScene::Initialize] EXTENDED this=%p, m_device=%p, m_spriteRenderer=%p, m_renderer=%p\n", this, m_device, m_spriteRenderer, m_renderer);
  OutputDebugStringA(buf);
  OutputDebugStringA("[MenuScene] Calling LoadTextures...\n");
  LoadTextures();
  OutputDebugStringA("[MenuScene] LoadTextures returned\n");
}

void MenuScene::Unload() {
  m_loaded = false;
}

void MenuScene::OnEnter() {
  OutputDebugStringA("[MenuScene::OnEnter] START\n");
  ClearExitRequest();
  OutputDebugStringA("[MenuScene::OnEnter] Calling ResetTextureState...\n");
  ResetTextureState();
  OutputDebugStringA("[MenuScene::OnEnter] END\n");
}

void MenuScene::OnExit() {
}

void MenuScene::ProcessInput(float deltaTime) {
  if (!m_gamepad) return;

  int itemCount = (m_menuState == MENU_STATE_MAIN) ? m_menuCount : MAP_SIZE_COUNT;

  if (m_gamepad->IsButtonPressed(Input::GP_DPadUp)) {
    m_selectedIndex--;
    if (m_selectedIndex < 0) m_selectedIndex = itemCount - 1;
  }
  if (m_gamepad->IsButtonPressed(Input::GP_DPadDown)) {
    m_selectedIndex++;
    if (m_selectedIndex >= itemCount) m_selectedIndex = 0;
  }

  float lx, ly;
  m_gamepad->GetLeftStick(lx, ly);
  if (fabsf(ly) > 0.5f) {
    m_stickTimer -= deltaTime;
    if (m_stickTimer <= 0.0f) {
      if (ly > 0) {
        m_selectedIndex++;
        if (m_selectedIndex >= itemCount) m_selectedIndex = 0;
      } else {
        m_selectedIndex--;
        if (m_selectedIndex < 0) m_selectedIndex = itemCount - 1;
      }
      m_stickTimer = 0.2f;
    }
  } else {
    m_stickTimer = 0.0f;
  }

  if (m_gamepad->IsButtonPressed(Input::GP_A)) {
    ExecuteMenuItem();
  }
  if (m_gamepad->IsButtonPressed(Input::GP_B)) {
    if (m_menuState == MENU_STATE_SIZE_SELECT) {
      m_menuState = MENU_STATE_MAIN;
      m_selectedIndex = 1;
    }
  }
}

void MenuScene::ExecuteMenuItem() {
  if (m_menuState == MENU_STATE_SIZE_SELECT) {
    int size = MAP_SIZES[m_selectedIndex];
    LaunchEditorWithSize(size, size);
    return;
  }

  char buf[256];
  sprintf(buf, "[MenuScene] ExecuteMenuItem called, selectedIndex = %d\n", m_selectedIndex);
  OutputDebugStringA(buf);
  
  SceneManager* sceneMgr = GetSceneManager();
  OutputDebugStringA(sceneMgr ? "[MenuScene] sceneMgr = VALID\n" : "[MenuScene] sceneMgr = NULL\n");
  
  switch (m_selectedIndex) {
    case 0: // New Game
      OutputDebugStringA("[MenuScene] New Game selected\n");
      if (sceneMgr) {
        OutputDebugStringA("[MenuScene] Calling GetScene(\"Loading\")...\n");
        Scene* rawScene = sceneMgr->GetScene("Loading");
        OutputDebugStringA(rawScene ? "[MenuScene] rawScene = VALID\n" : "[MenuScene] rawScene = NULL\n");
        if (rawScene) {
          OutputDebugStringA("[MenuScene] Attempting static_cast to LoadingScene...\n");
          LoadingScene* loadingScene = static_cast<LoadingScene*>(rawScene);
          OutputDebugStringA(loadingScene ? "[MenuScene] loadingScene = VALID\n" : "[MenuScene] loadingScene = NULL\n");
          if (loadingScene) {
            loadingScene->SetTargetScene("Game");
            OutputDebugStringA("[MenuScene] Set target scene to 'Game'\n");
          }
        }
      }
      OutputDebugStringA("[MenuScene] Calling RequestSceneSwitch(\"Loading\")...\n");
      RequestSceneSwitch("Loading");
      break;
    case 1: // Map Editor
      m_menuState = MENU_STATE_SIZE_SELECT;
      m_selectedIndex = 0;
      break;
    case 2: // Settings
      break;
    case 3: // Exit
      RequestExit();
      break;
  }
}

void MenuScene::LaunchEditorWithSize(int gridW, int gridH) {
  SceneManager* sceneMgr = GetSceneManager();
  EditorScene::s_mapGridWidth = gridW;
  EditorScene::s_mapGridHeight = gridH;

  if (sceneMgr) {
    Scene* rawScene = sceneMgr->GetScene("Loading");
    if (rawScene) {
      LoadingScene* loadingScene = static_cast<LoadingScene*>(rawScene);
      if (loadingScene) {
        loadingScene->SetTargetScene("Editor");
      }
    }
  }
  RequestSceneSwitch("Loading");
}

void MenuScene::Update(float deltaTime) {
  ProcessInput(deltaTime);
}

void MenuScene::Render(RenderQueue* renderQueue) {
    if (!renderQueue) {
        OutputDebugStringA("[MenuScene::Render] renderQueue=NULL\n");
        return;
    }

    if (!m_backgroundTexture.GetTexture()) {
        OutputDebugStringA("[MenuScene::Render] texture NULL - nothing to render\n");
        return;
    }

//    OutputDebugStringA("[MenuScene::Render] submitting background\n");
    // Background sprite
    Graphics::RenderCommand bgCmd = {};
    bgCmd.shaderID = SHADER_SPRITE;
    bgCmd.x = 0.0f;
    bgCmd.y = 0.0f;
    bgCmd.width = 1280.0f;
    bgCmd.height = 720.0f;
    bgCmd.u0 = 0.0f; bgCmd.v0 = 0.0f;
    bgCmd.u1 = 1.0f; bgCmd.v1 = 1.0f;
    bgCmd.color = 0xFFFFFFFF;
    bgCmd.depth = 900;
    bgCmd.layer = LAYER_UI;
    bgCmd.textureID = 0;
    renderQueue->Submit(bgCmd);

    // UI atlas sprites for menu
    TextureRegistry& reg = TextureRegistry::instance();
    std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
    LPDIRECT3DTEXTURE9 uiTex = uiAtlas ? uiAtlas->GetTexture() : NULL;
    if (uiTex && m_spriteRenderer) {
        m_spriteRenderer->SetTextureSlot(5, uiTex);
    }

    // Render menu_Grid as decorative frame
    if (uiAtlas) {
        uint32_t gridIdx = uiAtlas->GetIndex("menu_Grid");
        if (gridIdx != 0xFFFFFFFF) {
            const SpriteRegion* gridReg = uiAtlas->GetRegion(gridIdx);
            if (gridReg) {
                Graphics::RenderCommand gridCmd = {};
                gridCmd.x = 100.0f; gridCmd.y = 170.0f;
                gridCmd.width = 400.0f; gridCmd.height = 340.0f;
                gridCmd.u0 = gridReg->u0; gridCmd.v0 = gridReg->v0;
                gridCmd.u1 = gridReg->u1; gridCmd.v1 = gridReg->v1;
                gridCmd.color = 0xFFFFFFFF;
                gridCmd.shaderID = SHADER_UI;
                gridCmd.blendMode = 1;
                gridCmd.depth = 899;
                gridCmd.layer = LAYER_UI;
                gridCmd.textureID = 5;
                gridCmd.sortKey = Graphics::BuildSortKey(LAYER_UI, 1, SHADER_UI, 5, 899);
                renderQueue->Submit(gridCmd);
            }
        }
    }

    // Menu items with button_A sprite for selected item
    if (m_textManager && m_spriteRenderer) {
        if (m_menuState == MENU_STATE_SIZE_SELECT) {
            // Title
            m_textManager->DrawString("Select Map Size", 140.0f, 170.0f, 0xFFFFD700, 0.35f);

            float startY = 230.0f;
            float spacingY = 70.0f;
            for (int i = 0; i < MAP_SIZE_COUNT; ++i) {
                char sizeText[32];
                sprintf_s(sizeText, "%d x %d", MAP_SIZES[i], MAP_SIZES[i]);
                DWORD itemColor = (i == m_selectedIndex) ? 0xFFFFD700 : 0xFFFFFFFF;
                float itemX = (i == m_selectedIndex) ? 180.0f : 140.0f;
                m_textManager->DrawString(sizeText, itemX, startY + (i * spacingY), itemColor, 0.3f);

                if (i == m_selectedIndex && uiAtlas) {
                    uint32_t btnIdx = uiAtlas->GetIndex("button_A");
                    if (btnIdx != 0xFFFFFFFF) {
                        const SpriteRegion* btnReg = uiAtlas->GetRegion(btnIdx);
                        if (btnReg) {
                            Graphics::RenderCommand btnCmd = {};
                            btnCmd.x = 140.0f; btnCmd.y = startY + (i * spacingY) - 4.0f;
                            btnCmd.width = 32.0f; btnCmd.height = 32.0f;
                            btnCmd.u0 = btnReg->u0; btnCmd.v0 = btnReg->v0;
                            btnCmd.u1 = btnReg->u1; btnCmd.v1 = btnReg->v1;
                            btnCmd.color = 0xFFFFFFFF;
                            btnCmd.shaderID = SHADER_UI;
                            btnCmd.blendMode = 1;
                            btnCmd.depth = 898;
                            btnCmd.layer = LAYER_UI;
                            btnCmd.textureID = 5;
                            btnCmd.sortKey = Graphics::BuildSortKey(LAYER_UI, 1, SHADER_UI, 5, 898);
                            renderQueue->Submit(btnCmd);
                        }
                    }
                }
            }
        } else {
            float startY = 208.0f;
            float spacingY = 70.0f;
            for (int i = 0; i < m_menuCount; ++i) {
                DWORD itemColor = (i == m_selectedIndex) ? 0xFFFFD700 : 0xFFFFFFFF;
                float itemX = (i == m_selectedIndex) ? 180.0f : 140.0f;
                m_textManager->DrawString(m_menuItems[i], itemX, startY + (i * spacingY), itemColor, 0.3f);

                if (i == m_selectedIndex && uiAtlas) {
                    uint32_t btnIdx = uiAtlas->GetIndex("button_A");
                    if (btnIdx != 0xFFFFFFFF) {
                        const SpriteRegion* btnReg = uiAtlas->GetRegion(btnIdx);
                        if (btnReg) {
                            Graphics::RenderCommand btnCmd = {};
                            btnCmd.x = 140.0f; btnCmd.y = startY + (i * spacingY) - 4.0f;
                            btnCmd.width = 32.0f; btnCmd.height = 32.0f;
                            btnCmd.u0 = btnReg->u0; btnCmd.v0 = btnReg->v0;
                            btnCmd.u1 = btnReg->u1; btnCmd.v1 = btnReg->v1;
                            btnCmd.color = 0xFFFFFFFF;
                            btnCmd.shaderID = SHADER_UI;
                            btnCmd.blendMode = 1;
                            btnCmd.depth = 898;
                            btnCmd.layer = LAYER_UI;
                            btnCmd.textureID = 5;
                            btnCmd.sortKey = Graphics::BuildSortKey(LAYER_UI, 1, SHADER_UI, 5, 898);
                            renderQueue->Submit(btnCmd);
                        }
                    }
                }
            }
        }

        // Render button hints at bottom of screen
        if (uiAtlas) {
            uint32_t backIdx = uiAtlas->GetIndex("button_back");
            uint32_t startIdx = uiAtlas->GetIndex("button_start");
            if (backIdx != 0xFFFFFFFF) {
                const SpriteRegion* backReg = uiAtlas->GetRegion(backIdx);
                if (backReg) {
                    Graphics::RenderCommand cmd = {};
                    cmd.x = 20.0f; cmd.y = 660.0f;
                    cmd.width = 28.0f; cmd.height = 28.0f;
                    cmd.u0 = backReg->u0; cmd.v0 = backReg->v0;
                    cmd.u1 = backReg->u1; cmd.v1 = backReg->v1;
                    cmd.color = 0xFFFFFFFF;
                    cmd.shaderID = SHADER_UI;
                    cmd.blendMode = 1;
                    cmd.depth = 898;
                    cmd.layer = LAYER_UI;
                    cmd.textureID = 5;
                    cmd.sortKey = Graphics::BuildSortKey(LAYER_UI, 1, SHADER_UI, 5, 898);
                    renderQueue->Submit(cmd);
                }
            }
            m_textManager->DrawString("Back", 52.0f, 664.0f, 0xFF888888, 0.22f);
            if (startIdx != 0xFFFFFFFF) {
                const SpriteRegion* startReg = uiAtlas->GetRegion(startIdx);
                if (startReg) {
                    Graphics::RenderCommand cmd = {};
                    cmd.x = 120.0f; cmd.y = 660.0f;
                    cmd.width = 40.0f; cmd.height = 28.0f;
                    cmd.u0 = startReg->u0; cmd.v0 = startReg->v0;
                    cmd.u1 = startReg->u1; cmd.v1 = startReg->v1;
                    cmd.color = 0xFFFFFFFF;
                    cmd.shaderID = SHADER_UI;
                    cmd.blendMode = 1;
                    cmd.depth = 898;
                    cmd.layer = LAYER_UI;
                    cmd.textureID = 5;
                    cmd.sortKey = Graphics::BuildSortKey(LAYER_UI, 1, SHADER_UI, 5, 898);
                    renderQueue->Submit(cmd);
                }
            }
            m_textManager->DrawString("Select", 165.0f, 664.0f, 0xFF888888, 0.22f);
        }
    }
}

void MenuScene::SetBackground(const std::string& path) {
  m_backgroundPath = path;
}

void MenuScene::SetText(const std::string& text) {
  m_text = text;
}
