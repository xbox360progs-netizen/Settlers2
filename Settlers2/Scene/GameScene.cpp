#include "stdafx.h"
#include "GameScene.h"
#include "../Graphics/RenderQueue.h"
#include "../World/Map.h"
#include "../Logic/EconomyManager.h"
#include "../World/CarrierManager.h"

namespace Scene {

static void EconomyJobFunc(void* data);
static void WildlifeSectorFunc(void* data);
static void CarrierAssignFunc(void* data);
static void CarrierUpdateFunc(void* data);
static void AIChunkJobFunc(void* data);

GameScene::GameScene()
    : Scene("Game")
    , m_jobManager(NULL)
    , m_map(NULL)
    , m_wildlife(NULL)
    , m_economyManager(NULL)
    , m_carrierManager(NULL)
    , m_aiSystem(NULL)
{
}

GameScene::~GameScene()
{
}

void GameScene::Initialize(IDirect3DDevice9* device, Graphics::SpriteRenderer* spriteRenderer) {
    (void)device;
    (void)spriteRenderer;
    m_map = new World::Map(20, 20, 40, 80);
    m_wildlife = new World::WildlifeSystem(m_map);
    m_map->SetWildlifeSystem(m_wildlife);
    m_economyManager = new Logic::EconomyManager();
    m_carrierManager = new World::CarrierManager();
    m_aiSystem = new Logic::AISystem(0, m_map, m_economyManager);
}

void GameScene::Load()
{
    m_jobManager = new JobManager();
    int processors[] = { 1, 2 };
    m_jobManager->Initialize(2, processors);

    m_loaded = true;
}

void GameScene::Unload()
{
    if (m_jobManager) {
        m_jobManager->Shutdown();
        delete m_jobManager;
        m_jobManager = NULL;
    }
    m_loaded = false;
}

void GameScene::Update(float deltaTime)
{
    if (!m_loaded) return;

    // ─── Phase A: Economy ∥ Wildlife sectors ───────────────────────────
    m_economyJobData.economy = m_economyManager;
    m_economyJobData.carriers = m_carrierManager;

    m_jobManager->Submit(EconomyJobFunc, &m_economyJobData);

    bool doWildlifeSpawn = m_wildlife && m_wildlife->ShouldSpawn(deltaTime);
    if (doWildlifeSpawn)
    {
        int totalSpawners = m_wildlife ? m_wildlife->GetSpawnerCount() : 0;
        if (totalSpawners > 0)
        {
            int spawnersPerSector = totalSpawners / 4;
            int sectorStart = 0;
            for (int i = 0; i < 4; ++i)
            {
                m_wildlifeSectors[i].wildlife = m_wildlife;
                m_wildlifeSectors[i].startSpawner = sectorStart;
                m_wildlifeSectors[i].endSpawner = (i == 3) ? totalSpawners : sectorStart + spawnersPerSector;
                m_wildlifeSectors[i].newAnimals.clear();
                m_jobManager->Submit(WildlifeSectorFunc, &m_wildlifeSectors[i]);
                sectorStart = m_wildlifeSectors[i].endSpawner;
            }
        }
    }

    m_jobManager->WaitAll();

    // Merge wildlife spawns from sector buffers
    if (doWildlifeSpawn && m_wildlife)
    {
        for (int i = 0; i < 4; ++i)
            m_wildlife->AddAnimals(m_wildlifeSectors[i].newAnimals);
    }

    // ─── Phase B1: Carrier sort + assign (single-threaded) ──────────────
    m_jobManager->Submit(CarrierAssignFunc, m_carrierManager);
    m_jobManager->WaitAll();

    // ─── Phase B2: Carrier updates ∥ AI analysis ────────────────────────
    int numCarriers = m_carrierManager ? m_carrierManager->GetCarrierCount() : 0;
    if (numCarriers > 0)
    {
        int carriersPerRange = numCarriers / 4;
        if (carriersPerRange < 1) carriersPerRange = 1;
        int rangeStart = 0;
        for (int i = 0; i < 4; ++i)
        {
            m_carrierRanges[i].mgr = m_carrierManager;
            m_carrierRanges[i].startCarrier = rangeStart;
            m_carrierRanges[i].endCarrier = (i == 3) ? numCarriers : rangeStart + carriersPerRange;
            m_carrierRanges[i].dt = deltaTime;
            m_jobManager->Submit(CarrierUpdateFunc, &m_carrierRanges[i]);
            rangeStart = m_carrierRanges[i].endCarrier;
            if (rangeStart >= numCarriers) break;
        }
    }

    // AI chunks — read-only PlanBuild, use reservations + cache
    m_aiSystem->ClearReservations();

    m_aiChunks[0].ai = m_aiSystem;
    m_aiChunks[0].types[0] = World::Woodcutter;
    m_aiChunks[0].types[1] = World::Sawmill;
    m_aiChunks[0].types[2] = World::CoalMine;
    m_aiChunks[0].numTypes = 3;
    m_aiChunks[0].numRequests = 0;

    m_aiChunks[1].ai = m_aiSystem;
    m_aiChunks[1].types[0] = World::IronMine;
    m_aiChunks[1].types[1] = World::IronSmelter;
    m_aiChunks[1].types[2] = World::ToolWorkshop;
    m_aiChunks[1].numTypes = 3;
    m_aiChunks[1].numRequests = 0;

    m_aiChunks[2].ai = m_aiSystem;
    m_aiChunks[2].types[0] = World::Farm;
    m_aiChunks[2].types[1] = World::Mill;
    m_aiChunks[2].types[2] = World::Bakery;
    m_aiChunks[2].numTypes = 3;
    m_aiChunks[2].numRequests = 0;

    m_aiChunks[3].ai = m_aiSystem;
    m_aiChunks[3].types[0] = World::Hunter;
    m_aiChunks[3].types[1] = World::Fisher;
    m_aiChunks[3].types[2] = World::GoldMine;
    m_aiChunks[3].types[3] = World::GoldSmelter;
    m_aiChunks[3].numTypes = 4;
    m_aiChunks[3].numRequests = 0;

    m_jobManager->Submit(AIChunkJobFunc, &m_aiChunks[0]);
    m_jobManager->Submit(AIChunkJobFunc, &m_aiChunks[1]);
    m_jobManager->Submit(AIChunkJobFunc, &m_aiChunks[2]);
    m_jobManager->Submit(AIChunkJobFunc, &m_aiChunks[3]);
    m_jobManager->WaitAll();

    // ─── Phase C: Apply AI Commands (single-threaded) ───────────────────
    for (int c = 0; c < 4; ++c)
        m_aiSystem->ApplyBuildRequests(m_aiChunks[c].requests, m_aiChunks[c].numRequests);
}

void GameScene::Render(Graphics::RenderQueue* renderQueue)
{
    (void)renderQueue;
}

// ─── Job function implementations ───────────────────────────────────────

static void EconomyJobFunc(void* data)
{
    EconomyJobData* d = (EconomyJobData*)data;
    d->economy->Update(d->carriers);
}

static void WildlifeSectorFunc(void* data)
{
    WildlifeSectorData* d = (WildlifeSectorData*)data;
    if (d->wildlife)
        d->wildlife->ProcessSpawnerRange(d->startSpawner, d->endSpawner, d->newAnimals);
}

static void CarrierAssignFunc(void* data)
{
    World::CarrierManager* mgr = (World::CarrierManager*)data;
    mgr->SortAndAssign();
}

static void CarrierUpdateFunc(void* data)
{
    CarrierRangeData* d = (CarrierRangeData*)data;
    if (d->mgr)
        d->mgr->UpdateCarrierRange(d->startCarrier, d->endCarrier, d->dt);
}

static void AIChunkJobFunc(void* data)
{
    AIChunkData* d = (AIChunkData*)data;
    for (int i = 0; i < d->numTypes; ++i)
    {
        if (d->numRequests >= MAX_REQUESTS_PER_CHUNK) break;
        Logic::BuildRequest req;
        if (d->ai->PlanBuild(d->types[i], req))
            d->requests[d->numRequests++] = req;
    }
}

} // namespace Scene
