#pragma once

#pragma once

#include "Scene.h"
#include "../Graphics/RenderQueue.h"
#include "../Graphics/SpriteRenderer.h"
#include "../World/Map.h"
#include "../Logic/EconomyManager.h"
#include "../World/CarrierManager.h"
#include "../Logic/AISystem.h"
#include <xtl.h>

namespace Scene {

class GameScene : public Scene
{
public:
    GameScene();
    virtual ~GameScene();

    virtual void Initialize(IDirect3DDevice9* device, Graphics::SpriteRenderer* spriteRenderer);
    virtual void Load();
    virtual void Unload();
    virtual void Update(float deltaTime);
    virtual void Render(Graphics::RenderQueue* renderQueue);

private:
    HANDLE m_logicThread;
    HANDLE m_aiThread;
    static bool g_sceneRunning;

    static DWORD WINAPI LogicThreadFunction(LPVOID lpParam);
    static DWORD WINAPI AIThreadFunction(LPVOID lpParam);

    // Systems
    World::Map* m_map;
    Logic::EconomyManager* m_economyManager;
    World::CarrierManager* m_carrierManager;
    Logic::AISystem* m_aiSystem;
};

} // namespace Scene

