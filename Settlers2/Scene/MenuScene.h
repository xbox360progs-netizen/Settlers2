#ifndef SETTLERS2_SETTLERS2_SCENE_MENU_SCENE_H
#define SETTLERS2_SETTLERS2_SCENE_MENU_SCENE_H

#include "Scene.h"
#include <string>
#include "../Graphics/Renderer.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/TextureLoader.h"
#include "../Graphics/TextManager.h"
#include "../Graphics/BinFileManager.h"
#include "../Input/Gamepad.h"
#include "../Graphics/Texture.h"
#include "../Graphics/RenderQueue.h"
#include "../Graphics/ShaderManager.h"

using Graphics::SpriteRenderer;

// Menu constants
static const int MAX_MENU_ITEMS = 4;
static const int MAP_SIZE_COUNT = 4;
static const int MAP_SIZES[MAP_SIZE_COUNT] = { 20, 40, 60, 100 };
static const float MENU_START_X = 80.0f;
static const float MENU_START_Y = 200.0f;
static const float MENU_ITEM_HEIGHT = 40.0f;
static const DWORD COLOR_SELECTED = 0xFFFFFF00; // Yellow
static const DWORD COLOR_NORMAL = 0xFFFFFFFF;   // White

enum MenuState {
    MENU_STATE_MAIN,
    MENU_STATE_SIZE_SELECT
};

class MenuScene : public Scene::Scene {
public:
  MenuScene();
  virtual ~MenuScene();

  virtual void Load();
  virtual void Unload();
  virtual void Update(float deltaTime);
  virtual void Render(Graphics::RenderQueue* renderQueue) override;
  virtual void OnEnter();
  virtual void OnExit();

  void SetBackground(const std::string& path);
  void SetText(const std::string& text);

  virtual void Initialize(LPDIRECT3DDEVICE9 device, SpriteRenderer* spriteRenderer);
  void Initialize(LPDIRECT3DDEVICE9 device, SpriteRenderer* spriteRenderer, Renderer* renderer, Input::Gamepad* gamepad, TextureLoader* textureLoader);

  void SetGamepad(Input::Gamepad* gamepad) { m_gamepad = gamepad; }
  void SetTextManager(TextManager* textManager) { m_textManager = textManager; }
  void SetBinFileManager(BinFileManager* binFileManager) { m_binFileManager = binFileManager; }
  void SetRenderer(SpriteRenderer* spriteRenderer, Renderer* renderer) {
    char buf[256];
    sprintf(buf, "[MenuScene::SetRenderer] old sprite=%p, new sprite=%p, renderer=%p\n", 
            m_spriteRenderer, spriteRenderer, renderer);
    OutputDebugStringA(buf);
    m_renderer = renderer;
    m_spriteRenderer = spriteRenderer;
  }
  void SetTextureLoader(TextureLoader* textureLoader) { m_textureLoader = textureLoader; }

private:
  MenuState m_menuState;
  std::string m_menuItems[MAX_MENU_ITEMS];
  int m_menuCount;
  float m_stickTimer;
  std::string m_backgroundPath;
  std::string m_text;
  int m_selectedIndex;

  LPDIRECT3DDEVICE9 m_device;
  SpriteRenderer* m_spriteRenderer;
  Renderer* m_renderer;
  Texture m_backgroundTexture;
  TextureLoader* m_textureLoader;
  Input::Gamepad* m_gamepad;
  TextManager* m_textManager;
  BinFileManager* m_binFileManager;

  void ProcessInput(float deltaTime);
  void ExecuteMenuItem();
  void LaunchEditorWithSize(int gridW, int gridH);
  void ResetTextureState();
  void LoadTextures();
};

#endif // SETTLERS2_SETTLERS2_SCENE_MENU_SCENE_H
