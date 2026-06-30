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
#include "../UI/MenuModel.h"
#include "../UI/LocalizationService.h"
#include "../UI/ICommandDispatcher.h"

using Graphics::SpriteRenderer;

static const int MAP_SIZE_COUNT = 4;

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
    void SetDispatcher(UI::ICommandDispatcher* dispatcher) { m_dispatcher = dispatcher; }

    virtual void Initialize(LPDIRECT3DDEVICE9 device, SpriteRenderer* spriteRenderer);
    void Initialize(LPDIRECT3DDEVICE9 device, SpriteRenderer* spriteRenderer, Renderer* renderer, Input::Gamepad* gamepad, TextureLoader* textureLoader);

    void SetGamepad(Input::Gamepad* gamepad) { m_gamepad = gamepad; }
    void SetTextManager(TextManager* textManager) { m_textManager = textManager; }
    void SetBinFileManager(BinFileManager* binFileManager) { m_binFileManager = binFileManager; }
    void SetRenderer(SpriteRenderer* spriteRenderer, Renderer* renderer);
    void SetTextureLoader(TextureLoader* textureLoader) { m_textureLoader = textureLoader; }

private:
    MenuState m_menuState;
    UI::MenuModel m_menuModel;
    UI::LocalizationService m_loc;
    UI::ICommandDispatcher* m_dispatcher;
    float m_stickTimer;
    std::string m_backgroundPath;
    int m_prevMainSelection;

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
    void ShowMainMenu();
    void ShowSizeSelect();
    void ResetTextureState();
    void LoadTextures();
};

#endif // SETTLERS2_SETTLERS2_SCENE_MENU_SCENE_H
