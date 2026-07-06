#pragma once

// Forward declarations
namespace Input {
    class InputManager;
}
class Renderer;
namespace Graphics { class ShaderManager; class SpriteRenderer; }
using Graphics::ShaderManager;
using Graphics::SpriteRenderer;
class BitmapFont;
class TextManager;
#include "../Graphics/BinFileManager.h"
#include "../Graphics/TextureLoader.h"
class BinFileManager;
class TextureLoader;

namespace Scene {
    class SceneManager;
}

class GameEngine
{
public:
    GameEngine();
    ~GameEngine();

    bool Initialize();
    void Shutdown();

    void Run();

    bool IsRunning() const { return m_running; }

private:
    void CreateScenes();

    void Update(float deltaTime);
    void Render();
    void ProcessSceneRequests();

private:
    bool m_running;
    bool m_initialized;

    Renderer* m_renderer;
    SpriteRenderer* m_spriteRenderer;
    ShaderManager* m_pShaderManager;
    BitmapFont* m_bitmapFont;
    TextManager* m_textManager;
    BinFileManager* m_binFileManager;
    TextureLoader* m_textureLoader;

    Input::InputManager* m_inputManager;

    Scene::SceneManager* m_sceneManager;
};
