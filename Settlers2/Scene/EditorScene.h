#ifndef SETTLERS2_SCENE_EDITOR_SCENE_H
#define SETTLERS2_SCENE_EDITOR_SCENE_H

#include "Scene.h"
#include "../Logic/MapConstants.h"
#include "../Graphics/Camera.h"
#include "../Graphics/RadialMenu.h"
#include "../Graphics/GridMenu.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/Texture.h"
#include "../Graphics/TextManager.h"
#include "../Editor/MapEditor.h"
#include "../World/Map.h"
#include "../World/ResourceNode.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/BinFileManager.h"
#include "../Input/InputManager.h"
#include "../Input/InputController.h"
#include "../UI/WeightMenu.h"

namespace Scene {

// FSM states for resource placement
enum EditorState
{
    STATE_IDLE = 0,
    STATE_SELECTING,
    STATE_PLACING,
    STATE_INPUT_AMOUNT
};

// Editor modes for different editing operations
enum EditorMode
{
    MODE_TERRAIN = 0,
    MODE_WEIGHTS,
    MODE_RESOURCES
};

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
    class Camera* m_camera;
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

    // Selection
    int m_selectedTileX;
    int m_selectedTileY;
    bool m_hasSelection;

    // Input controller for world coordinate translation
    Logic::InputController* m_inputController;

    // FSM state for resource placement
    EditorState m_currentState;
    World::ResourceType m_activeResourceType;
    int m_phantomTileX;
    int m_phantomTileY;

    // Editor mode (Terrain, Weights, Resources)
    EditorMode m_editorMode;
    bool m_weightMenuVisible;
    BYTE m_activeWeight;

    // Weight menu for D-pad weight selection
    UI::WeightMenu* m_weightMenu;
};

} // namespace Scene

#endif // SETTLERS2_SCENE_EDITOR_SCENE_H
