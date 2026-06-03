#pragma once

#include "Scene.h"
#include "../Core/JobManager.h"
#include "../Graphics/RenderQueue.h"
#include "../Graphics/SpriteRenderer.h"
#include "../World/Map.h"
#include "../World/WildlifeSystem.h"
#include "../Logic/EconomyManager.h"
#include "../World/CarrierManager.h"
#include "../Logic/AISystem.h"
#include "../World/Components/Building.h"
#include <xtl.h>

namespace Scene {

struct EconomyJobData
{
    Logic::EconomyManager* economy;
    World::CarrierManager* carriers;
};

struct WildlifeSectorData
{
    World::WildlifeSystem* wildlife;
    int startSpawner;
    int endSpawner;
    std::vector<World::Animal> newAnimals;
};

struct CarrierRangeData
{
    World::CarrierManager* mgr;
    int startCarrier;
    int endCarrier;
    float dt;
};

static const int MAX_REQUESTS_PER_CHUNK = 8;

struct AIChunkData
{
    Logic::AISystem* ai;
    World::BuildingType types[4];
    int numTypes;
    Logic::BuildRequest requests[MAX_REQUESTS_PER_CHUNK];
    int numRequests;
};

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
    JobManager* m_jobManager;

    // Systems
    World::Map* m_map;
    World::WildlifeSystem* m_wildlife;
    Logic::EconomyManager* m_economyManager;
    World::CarrierManager* m_carrierManager;
    Logic::AISystem* m_aiSystem;

    // Job data (reused each frame)
    EconomyJobData m_economyJobData;
    WildlifeSectorData m_wildlifeSectors[4];
    CarrierRangeData m_carrierRanges[4];
    AIChunkData m_aiChunks[4];
};

} // namespace Scene
