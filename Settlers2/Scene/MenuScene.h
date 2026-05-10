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

// Menu constants
static const int MAX_MENU_ITEMS = 4;
static const float MENU_START_X = 80.0f;
static const float MENU_START_Y = 200.0f;
static const float MENU_ITEM_HEIGHT = 40.0f;
static const DWORD COLOR_SELECTED = 0xFFFFFF00; // Yellow
static const DWORD COLOR_NORMAL = 0xFFFFFFFF;   // White

class MenuScene : public Scene::Scene {
public:
  MenuScene();
  virtual ~MenuScene();

  // Реализация жизненного цикла
  virtual void Load();
  virtual void Unload();
  virtual void Update(float deltaTime);
  virtual void Render();
  virtual void OnEnter();
  virtual void OnExit();

  void SetBackground(const std::string& path);
  void SetText(const std::string& text);

  // Переопределение инициализации из базового класса
  virtual void Initialize(LPDIRECT3DDEVICE9 device, SpriteRenderer* spriteRenderer);
  
  // Extended initialization with all dependencies
  void Initialize(LPDIRECT3DDEVICE9 device, SpriteRenderer* spriteRenderer, Renderer* renderer, Input::Gamepad* gamepad, TextureLoader* textureLoader);

  // Input handling
  void SetGamepad(Input::Gamepad* gamepad) { m_gamepad = gamepad; }
  void SetTextManager(TextManager* textManager) { m_textManager = textManager; }
  void SetBinFileManager(BinFileManager* binFileManager) { m_binFileManager = binFileManager; }
  void SetRenderer(Renderer* renderer) { m_renderer = renderer; }
  void SetTextureLoader(TextureLoader* textureLoader) { m_textureLoader = textureLoader; }

private:
  // Menu items
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

  // Input handling
  void ProcessInput(float deltaTime);
  void ExecuteMenuItem();

  // Загрузка текстур из INI файла
  void LoadTextures();
};

#endif // SETTLERS2_SETTLERS2_SCENE_MENU_SCENE_H
