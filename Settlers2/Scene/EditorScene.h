#ifndef SETTLERS2_SCENE_EDITOR_SCENE_H
#define SETTLERS2_SCENE_EDITOR_SCENE_H

#include <vector>
#include "Scene.h"
#include "../Logic/MapConstants.h"
#include "../Graphics/Camera.h"
#include "../UI/RadialMenu.h"
#include "../UI/GridMenu.h"
#include "../UI/WeightMenu.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/Texture.h"
#include "../Graphics/TextManager.h"
#include "../Editor/MapEditor.h"
#include "../Game/Unit.h"
#include "../World/Map.h"
#include "../World/ResourceNode.h"
#include "../World/WildlifeSystem.h"
#include "../Graphics/Renderer.h"
#include "../Input/InputManager.h"
#include "../Input/InputController.h"

using Graphics::SpriteRenderer;

namespace Scene {

// FSM states for resource placement
enum EditorState
{
    STATE_IDLE = 0,
    STATE_SELECTING,
    STATE_PLACING,
    STATE_INPUT_AMOUNT,
    STATE_DEPOSIT_PREVIEW
};

// Editor modes for different editing operations
enum EditorMode
{
    MODE_TERRAIN = 0,
    MODE_WEIGHTS,
    MODE_RESOURCES,
    MODE_PLACEMENT
};

class EditorScene : public Scene {
private:
    // Weight menu related
    bool m_weightMenuVisible;
    UI::WeightMenu* m_weightMenu;
    bool m_weightMenuPlacementMode;
    BYTE m_activeWeight;

    // Private methods
    void ResetShaderState();
    void UpdateWeightMenu(Input::Gamepad* gamepad, float deltaTime);
    void UpdateLBButton(Input::Gamepad* gamepad);
    void UpdateRBButton(Input::Gamepad* gamepad, bool anyMenuActive);
    void UpdateGridMenu(Input::Gamepad* gamepad, float deltaTime);
    void UpdateRadialMenu(Input::Gamepad* gamepad);
    void HandleRBButtonAction();
    void HandleWeightMenuToggle();
    void HandleResourceMenuToggle();
    void HandleRoadMenuToggle();
    void HandleDefaultMenuToggle();
    void CreateResourceGridMenu();
    void CreateRoadGridMenu();
    void CreateDefaultGridMenu();
    void HandleGridMenuInput(Input::Gamepad* gamepad);
    void HandleGridMenuAButton();
    void HandleGridMenuBButton();
    void HandleGridMenuYButton();
    void HandleRoadSelection(int selectedIndex);
    void HandleRadialMenuSelection();

public:
    EditorScene();
    virtual ~EditorScene();
    virtual void Load();
    virtual void Unload();
    virtual void Update(float deltaTime);
    virtual void Render(Graphics::RenderQueue* renderQueue) override;
    virtual void RenderOverlay() override;
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
    void LoadGridMenuGroup(const char* groupName);
    void LoadUIAtlasGroup(const char* groupName, const std::vector<std::string>& filterNames);
	void LoadUIAtlasGroup(const char* groupName);
    void LoadResourceIcons();
    void LoadResourceGroupIcons();
    void LoadResourceGroupResources(int groupIndex);

    // Returns true if a selection was made (GridMenu hidden, state changed)
    bool HandleGridMenuResourceSelection(Input::Gamepad* gamepad);

    // Update methods
    void UpdateFPS();
    void UpdateMenus(Input::Gamepad* gamepad, float deltaTime);
    void UpdateInputController(float deltaTime);
    void UpdateCursorAndTiles();
    void UpdateResourcePlacementFSM();
    void UpdateCamera(Input::Gamepad* gamepad, float deltaTime);
    void UpdateMapEditor(float deltaTime, Input::Gamepad* gamepad);
    void CycleObjectGroup();
    void UpdateSaveLoadMenu(Input::Gamepad* gamepad, float deltaTime);
    void RenderSaveLoadMenu(Graphics::RenderQueue* renderQueue);

    // Public member variables
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
    Texture m_bgEditorTexture;
    Editor::MapEditor* m_mapEditor;
    Game::UnitManager* m_unitManager;
    World::AnimalManager* m_animalManager;
    World::WildlifeSystem* m_wildlife;
    World::LayerType m_currentLayer;

    // Object group cycling (maptiles groups)
    static const char* kObjectGroupNames[];
    static const int kObjectGroupCount;
    int m_objectGroupIndex;

    // Texture slot constants (mirrors GameScene.h for consistency)
    enum {
        SLOT_UNITS = 29,
    };
    bool m_yButtonWasPressed;
    bool m_blockCameraUntilStickNeutral;

    // Resource group for GridMenu
    static const char* kResourceGroupName;
    static const int kResourceTypeCount;
    static const int kResourceMenuGroupCount;
    int m_resourceMenuGroupIndex;
    bool m_resourceMenuShowingGroups;
    int m_resourceAmount;

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

    // Deposit preview (building → confirm → resource icon)
    int m_depositBuildingSpriteIdx;
    bool m_depositConfirmPending;

    // Editor mode (Terrain, Weights, Resources)
    EditorMode m_editorMode;
    bool m_resourcesInitialized;

    // Button hint textures for GridMenu
    GridMenu::TileUV m_buttonAUV;
    GridMenu::TileUV m_buttonBUV;

    // Map size (set by MenuScene before transition)
    static int s_mapGridWidth;
    static int s_mapGridHeight;

    // Save/Load menu
    bool m_saveLoadMenuActive;
    int m_saveLoadMenuSection;   // 0=main, 1=save, 2=load, 3=confirm
    int m_saveLoadMenuSelection;
    int m_saveLoadMenuPendingSlot; // slot index for confirm overwrite
    float m_saveLoadMenuInputTimer;
    static const int SAVE_SLOT_COUNT = 10;
};

} // namespace Scene

#endif // SETTLERS2_SCENE_EDITOR_SCENE_H
