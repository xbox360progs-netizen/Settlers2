#include "stdafx.h"
#include "MenuScene.h"
#include <iostream>
#include "SceneManager.h"
#include "LoadingScene.h"
#include "../Graphics/TextureLoader.h"
#include "../Graphics/Texture.h"
#include "../Graphics/TextureRegistry.h"

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
    m_textManager(NULL), m_binFileManager(NULL), m_textureLoader(NULL), m_menuCount(4), m_stickTimer(0.0f) {
  m_menuItems[0] = "New Game";
  m_menuItems[1] = "Map Editor";
  m_menuItems[2] = "Settings";
  m_menuItems[3] = "Exit";
}

MenuScene::~MenuScene() {
}

void MenuScene::Load() {
  m_loaded = true;
  std::cout << "[MenuScene] Load() called" << std::endl;
}

void MenuScene::LoadTextures() {
    TextureRegistry& registry = TextureRegistry::instance();
    
    // 1. Инициализируем список путей из конфига (если еще не сделано)
    registry.initializeFromManifest("game:\\Media\\Config\\textures.ini", "Menu");
    
    // 2. Просто просим текстуру по имени
    LPDIRECT3DTEXTURE9 tex = registry.getTextureOrLoad("menu_background");
    
    if (tex) {
        m_backgroundTexture.SetTexture(tex);
    } else {
        // Здесь мы окажемся только если даже заглушка не создалась
        OutputDebugStringA("[MenuScene] Serious error: background texture is NULL\n");
    }
}

void MenuScene::Initialize(LPDIRECT3DDEVICE9 device, SpriteRenderer* spriteRenderer) {
  m_device = device;
  m_spriteRenderer = spriteRenderer;
  
  std::cout << "[MenuScene] Initialize called" << std::endl;
  
  LoadTextures();
}

void MenuScene::Initialize(LPDIRECT3DDEVICE9 device, SpriteRenderer* spriteRenderer, Renderer* renderer, Input::Gamepad* gamepad, TextureLoader* textureLoader) {
  m_device = device;
  m_spriteRenderer = spriteRenderer;
  m_renderer = renderer;
  m_gamepad = gamepad;
  m_textureLoader = textureLoader;
  
  std::cout << "[MenuScene] Initialize called" << std::endl;
  
  LoadTextures();
}

void MenuScene::Unload() {
  m_loaded = false;
}

void MenuScene::OnEnter() {
  ClearExitRequest();
}

void MenuScene::OnExit() {
}

void MenuScene::ProcessInput(float deltaTime) {
  if (!m_gamepad) return;

  if (m_gamepad->IsButtonPressed(Input::GP_DPadUp)) {
    m_selectedIndex--;
    if (m_selectedIndex < 0) m_selectedIndex = m_menuCount - 1;
  }
  if (m_gamepad->IsButtonPressed(Input::GP_DPadDown)) {
    m_selectedIndex++;
    if (m_selectedIndex >= m_menuCount) m_selectedIndex = 0;
  }

  float lx, ly;
  m_gamepad->GetLeftStick(lx, ly);
  if (fabsf(ly) > 0.5f) {
    m_stickTimer -= deltaTime;
    if (m_stickTimer <= 0.0f) {
      if (ly > 0) {
        m_selectedIndex++;
        if (m_selectedIndex >= m_menuCount) m_selectedIndex = 0;
      } else {
        m_selectedIndex--;
        if (m_selectedIndex < 0) m_selectedIndex = m_menuCount - 1;
      }
      m_stickTimer = 0.2f; 
    }
  } else {
    m_stickTimer = 0.0f;
  }

  if (m_gamepad->IsButtonPressed(Input::GP_A)) {
    ExecuteMenuItem();
  }
}

void MenuScene::ExecuteMenuItem() {
  std::cout << "[MenuScene] ExecuteMenuItem called, selectedIndex = " << m_selectedIndex << std::endl;
  
  SceneManager* sceneMgr = GetSceneManager();
  std::cout << "[MenuScene] sceneMgr = " << (sceneMgr ? "VALID" : "NULL") << std::endl;
  
  switch (m_selectedIndex) {
    case 0: // New Game
      std::cout << "[MenuScene] New Game selected" << std::endl;
      // Set target scene in LoadingScene before switching
      if (sceneMgr) {
        std::cout << "[MenuScene] Calling GetScene(\"Loading\")..." << std::endl;
        Scene* rawScene = sceneMgr->GetScene("Loading");
        std::cout << "[MenuScene] rawScene = " << (rawScene ? "VALID" : "NULL") << std::endl;
        if (rawScene) {
          std::cout << "[MenuScene] Attempting static_cast to LoadingScene..." << std::endl;
          LoadingScene* loadingScene = static_cast<LoadingScene*>(rawScene);
          std::cout << "[MenuScene] loadingScene = " << (loadingScene ? "VALID" : "NULL") << std::endl;
          if (loadingScene) {
            loadingScene->SetTargetScene("Game");
            std::cout << "[MenuScene] Set target scene to 'Game'" << std::endl;
          } else {
            std::cout << "[MenuScene] ERROR: static_cast failed!" << std::endl;
          }
        } else {
          std::cout << "[MenuScene] ERROR: GetScene returned NULL!" << std::endl;
        }
      }
      std::cout << "[MenuScene] Calling RequestSceneSwitch(\"Loading\")..." << std::endl;
      RequestSceneSwitch("Loading");
      break;
    case 1: // Map Editor
      std::cout << "[MenuScene] Map Editor selected" << std::endl;
      // Set target scene in LoadingScene before switching
      if (sceneMgr) {
        std::cout << "[MenuScene] Calling GetScene(\"Loading\")..." << std::endl;
        Scene* rawScene = sceneMgr->GetScene("Loading");
        std::cout << "[MenuScene] rawScene = " << (rawScene ? "VALID" : "NULL") << std::endl;
        if (rawScene) {
          std::cout << "[MenuScene] Attempting static_cast to LoadingScene..." << std::endl;
          LoadingScene* loadingScene = static_cast<LoadingScene*>(rawScene);
          std::cout << "[MenuScene] loadingScene = " << (loadingScene ? "VALID" : "NULL") << std::endl;
          if (loadingScene) {
            loadingScene->SetTargetScene("Editor");
            std::cout << "[MenuScene] Set target scene to 'Editor'" << std::endl;
          } else {
            std::cout << "[MenuScene] ERROR: static_cast failed!" << std::endl;
          }
        } else {
          std::cout << "[MenuScene] ERROR: GetScene returned NULL!" << std::endl;
        }
      }
      std::cout << "[MenuScene] Calling RequestSceneSwitch(\"Loading\")..." << std::endl;
      RequestSceneSwitch("Loading");
      break;
    case 2: // Settings
      break;
    case 3: // Exit
      RequestExit();
      break;
  }
}

void MenuScene::Update(float deltaTime) {
  ProcessInput(deltaTime);
}

void MenuScene::Render() {
  if (!m_device) {
    OutputDebugStringA("[MenuScene] No device available for rendering\n");
    return;
  }

  // Use DrawSingleSprite for background (bypasses complex batching for testing)
  LPDIRECT3DTEXTURE9 bgTex = m_backgroundTexture.GetTexture();

  // Debug logging to diagnose empty screen
  if (!bgTex) {
    OutputDebugStringA("[MenuScene] ERROR: Background texture is NULL!\n");
  }
  if (!m_renderer) {
    OutputDebugStringA("[MenuScene] ERROR: Renderer is NULL!\n");
  }

  if (bgTex && m_renderer) {
    float screenWidth = 1280.0f;
    float screenHeight = 720.0f;

    OutputDebugStringA("[MenuScene] Drawing background sprite...\n");
    m_renderer->DrawSingleSprite(&m_backgroundTexture, 0.0f, 0.0f, screenWidth, screenHeight);
  } else if (bgTex && m_spriteRenderer) {
    // Try using SpriteRenderer instead

    // SpriteRenderer might have different interface - just output for now
    OutputDebugStringA("[MenuScene] WARNING: Using SpriteRenderer not implemented yet\n");
  } else {
    if (!bgTex) {
      OutputDebugStringA("[MenuScene] WARNING: Background texture is NULL!\n");
    }
    if (!m_renderer) {
      OutputDebugStringA("[MenuScene] WARNING: Renderer is NULL!\n");
    }
    if (!m_spriteRenderer) {
      OutputDebugStringA("[MenuScene] WARNING: SpriteRenderer is NULL!\n");
    }
  }

  if (m_textManager) {
    m_textManager->Begin();
    for (int i = 0; i < m_menuCount; i++) {
      DWORD color = (i == m_selectedIndex) ? COLOR_SELECTED : COLOR_NORMAL;
      float y = MENU_START_Y + i * MENU_ITEM_HEIGHT;
      m_textManager->DrawTextToScreen(m_menuItems[i], MENU_START_X, y, color, 0.25f);
    }
    m_textManager->RenderScreen();
  }
}

void MenuScene::SetBackground(const std::string& path) {
  m_backgroundPath = path;
}

void MenuScene::SetText(const std::string& text) {
  m_text = text;
}
