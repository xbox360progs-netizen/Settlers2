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

  // Debug logging for m_spriteRenderer initialization
  if (!m_spriteRenderer) {
    std::cout << "[MenuScene] WARNING: m_spriteRenderer is NULL in Load()" << std::endl;
  } else {
    std::cout << "[MenuScene] m_spriteRenderer is valid in Load()" << std::endl;
  }
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
  
  char buf[256];
  sprintf(buf, "[MenuScene::Initialize] EXTENDED this=%p, m_device=%p, m_spriteRenderer=%p, m_renderer=%p\n", this, m_device, m_spriteRenderer, m_renderer);
  OutputDebugStringA(buf);
  std::cout << "[MenuScene] Initialize called" << std::endl;
  
  if (m_spriteRenderer) {
    sprintf(buf, "[MenuScene::Initialize] m_spriteRenderer->GetDevice()=%p\n", m_spriteRenderer->GetDevice());
    OutputDebugStringA(buf);
  }
  
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
  OutputDebugStringA("[MenuScene::Render] ENTRY\n");

  // 1. Базовая проверка
  if (!m_spriteRenderer || !m_backgroundTexture.GetTexture()) return;

  LPDIRECT3DTEXTURE9 bgTex = m_backgroundTexture.GetTexture();
  OutputDebugStringA("[MenuScene::Render] Got texture pointer\n");

  // 2. Подготовка (SR сам выставит шейдер ID 0 и стейты)
  OutputDebugStringA("[MenuScene::Render] Calling Begin...\n");
  m_spriteRenderer->Begin(SHADER_SPRITE, bgTex, 0.0f, 0, false);
  OutputDebugStringA("[MenuScene::Render] Begin() returned\n");

  // 3. Отправка спрайта (Scene просто передает данные)
  OutputDebugStringA("[MenuScene::Render] Calling Draw...\n");
  m_spriteRenderer->Draw(0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF);
  OutputDebugStringA("[MenuScene::Render] Draw() returned\n");

  // 4. Финализация (Здесь произойдет реальный Draw Call)
  OutputDebugStringA("[MenuScene::Render] Calling End...\n");
  m_spriteRenderer->End();
  OutputDebugStringA("[MenuScene::Render] End() returned\n");

  OutputDebugStringA("[MenuScene::Render] About to return from Render()...\n");
/*
  // 5. Текст рисуем ПОСЛЕ (так как у него другой шейдер - ID 7)
  if (m_textManager) {
    m_textManager->Begin();
    for (int i = 0; i < m_menuCount; i++) {
      DWORD color = (i == m_selectedIndex) ? COLOR_SELECTED : COLOR_NORMAL;
      float y = MENU_START_Y + i * MENU_ITEM_HEIGHT;
      m_textManager->DrawTextToScreen(m_menuItems[i], MENU_START_X, y, color, 0.25f);
    }
    m_textManager->RenderScreen();
  }
*/
}

void MenuScene::SetBackground(const std::string& path) {
  m_backgroundPath = path;
}

void MenuScene::SetText(const std::string& text) {
  m_text = text;
}
