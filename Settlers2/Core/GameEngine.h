#pragma once

#include <xtl.h>
#include <d3d9.h>

// Forward declarations
namespace Input {
    class InputManager;
}
class Renderer;
class SpriteRenderer;
class ShaderManager;
class BitmapFont;
class TextManager;
#include "../Graphics/BinFileManager.h"
#include "../Graphics/TextureLoader.h"
class BinFileManager;
class TextureLoader;

namespace Scene {
    class SceneManager;
}

//-------------------------------------------------------------------------------------
// Game Engine - Main game logic and scene management
//-------------------------------------------------------------------------------------
class GameEngine
{
public:
    GameEngine();
    ~GameEngine();

    // Инициализация/Shutdown
    bool Initialize();
    void Shutdown();

    // Main loop
    void Run();

    // State checks
    bool IsRunning() const { return m_running; }

private:
    // Scene creation
    void CreateScenes();

    // Game loop helpers
    void Update(float deltaTime);
    void Render();
    void ProcessSceneRequests();

private:
    bool m_running;
    bool m_initialized;

    // Graphics systems
    Renderer* m_renderer;
    SpriteRenderer* m_spriteRenderer;
    BitmapFont* m_bitmapFont;
    TextManager* m_textManager;
    BinFileManager* m_binFileManager;
    TextureLoader* m_textureLoader;

    // Input system
    Input::InputManager* m_inputManager;

    // Scene management
    Scene::SceneManager* m_sceneManager;
};
