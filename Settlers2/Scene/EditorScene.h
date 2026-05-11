#ifndef SETTLERS2_SCENE_EDITOR_SCENE_H
#define SETTLERS2_SCENE_EDITOR_SCENE_H

#include "Scene.h"
#include "../Logic/MapConstants.h"
#include "../World/TileLayer.h"
#include "../Graphics/RadialMenu.h"
#include "../Graphics/TextManager.h"
#include "../Graphics/GridMenu.h"
#include "../Graphics/Texture.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/BinFileManager.h"
#include "../Input/InputManager.h"
#include "../Editor/MapEditor.h"

namespace Scene {

class EditorScene : public Scene {
public:
EditorScene();
    virtual ~EditorScene();
    virtual void Load();
    virtual void Unload();
    virtual void Update(float deltaTime);
    virtual void Render();
    virtual void OnEnter();
    virtual void OnExit();

    void SetRenderer(Renderer* renderer) { m_renderer = renderer; }
    void SetSpriteRenderer(SpriteRenderer* spriteRenderer) { m_spriteRenderer = spriteRenderer; }
    void SetInputManager(Input::InputManager* inputManager) { m_inputManager = inputManager; }
    void SetBinFileManager(BinFileManager* binFileManager) { m_binFileManager = binFileManager; }
    void SetTextManager(class TextManager* textManager) { m_textManager = textManager; }
    void SetShaderManager(class ShaderManager* shaderManager) { m_shaderManager = shaderManager; }
    // Bind three textures to GridMenu externally (no loading here)
    void BindGridMenuTextures(LPDIRECT3DTEXTURE9 bgTexture, LPDIRECT3DTEXTURE9 cellTexture, LPDIRECT3DTEXTURE9 atlasTexture);

    // Load an atlas by name into GridMenu (textures + UVs)
    void LoadGridMenuAtlas(const char* atlasName);

    // Cycle through object atlases (icon_tree -> icon_mountains -> icon_mountains_water -> icon_rocks)
    void CycleObjectAtlas();

    Renderer* m_renderer;
    SpriteRenderer* m_spriteRenderer;
    Input::InputManager* m_inputManager;
    BinFileManager* m_binFileManager;
    class TextManager* m_textManager;
    class ShaderManager* m_shaderManager;
    RadialMenu* m_radialMenu;
    GridMenu* m_gridMenu;
    Texture m_groundTexture;
    Editor::MapEditor* m_mapEditor;
    World::LayerType m_currentLayer;

    // Object atlas cycling
    static const char* kObjectAtlasNames[];
    static const int kObjectAtlasCount;
    int m_objectAtlasIndex;
    bool m_yButtonWasPressed;

    // FPS counter
    int m_fps;
    int m_frameCount;
    DWORD m_lastFpsTime;
};

} // namespace Scene

#endif // SETTLERS2_SCENE_EDITOR_SCENE_H
