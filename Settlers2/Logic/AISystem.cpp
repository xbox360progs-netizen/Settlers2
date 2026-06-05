#include "stdafx.h"
#include "AISystem.h"
#include "../World/Components/Woodcutter.h"
#include "../World/Components/CoalMine.h"
#include "../World/Components/IronMine.h"
#include "../World/Components/GoldMine.h"
#include "../World/Components/Farm.h"
#include "../World/Components/Mill.h"
#include "../World/Components/Bakery.h"
#include "../World/Components/Sawmill.h"
#include "../World/Components/Stonemason.h"
#include "../World/Components/Hunter.h"
#include "../World/Components/Fisher.h"
#include "../World/Components/ToolWorkshop.h"
#include "../World/Components/IronSmelter.h"
#include "../World/Components/GoldSmelter.h"

namespace Logic {

    static bool FindBuildableTile(World::Map* map, int startX, int startY, int radius, int& outX, int& outY)
    {
        int layerWidth = map->GetWidth() * 2;
        int layerHeight = map->GetHeight() * 4;

        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                int cx = startX + dx;
                int cy = startY + dy;
                if (cx < 0 || cx >= layerWidth || cy < 0 || cy >= layerHeight) continue;

                World::Tile objTile = map->GetTile(World::Objects, cx, cy);
                World::Tile bldTile = map->GetTile(World::Buildings, cx, cy);
                if (objTile.type == World::Tile_None && bldTile.type == World::Tile_None) {
                    outX = cx;
                    outY = cy;
                    return true;
                }
            }
        }
        return false;
    }

    static bool FindAdjacentBuildable(World::Map* map, int nearX, int nearY, int& outX, int& outY)
    {
        return FindBuildableTile(map, nearX, nearY, 3, outX, outY);
    }

    static int GetResourceSearchRadius(World::BuildingType type)
    {
        switch (type)
        {
            case World::Woodcutter:  return 8;
            case World::CoalMine:    return 6;
            case World::IronMine:    return 6;
            case World::GoldMine:    return 6;
            case World::Farm:        return 8;
            case World::Fisher:      return 8;
            case World::Hunter:      return 8;
            default:                 return 0;
        }
    }

    static bool FindResourceForBuilding(World::Map* map, World::BuildingType type, int startX, int startY, int& resX, int& resY)
    {
        int radius = GetResourceSearchRadius(type);
        if (radius == 0) return false;

        World::ResourceType resType = World::ResourceType_None;
        switch (type)
        {
            case World::Woodcutter: resType = World::ResourceType_Wood; break;
            case World::Farm:       resType = World::ResourceType_Field; break;
            case World::Fisher:     resType = World::ResourceType_Fish; break;
            case World::Hunter:     resType = World::ResourceType_WildlifeSpawner_Deer; break;
            default: break;
        }

        if (resType != World::ResourceType_None) {
            return map->FindResourceInRadius(startX, startY, radius, resType, resX, resY);
        }

        bool isMine = (type == World::CoalMine || type == World::IronMine || type == World::GoldMine);
        if (isMine) {
            return map->FindTileTypeInRadius(startX, startY, radius, World::Objects, World::Mountain, resX, resY);
        }

        return false;
    }

     AISystem::AISystem(uint8_t playerID, World::Map* map, EconomyManager* economy)
         : m_playerID(playerID), m_map(map), m_economy(economy), m_decisionTimer(0.0f)
     {
         int numTiles = (map->GetWidth() * 2) * (map->GetHeight() * 4);
         m_reservedNumWords = (numTiles + 31) / 32;
         m_reservedBits = new LONG[m_reservedNumWords]();

         for (int i = 0; i < MAX_BUILDING_TYPE; ++i)
             m_siteCacheValid[i] = false;
         
         // Initialize decision list with some basic buildings
         UpdateDecisionList();
     }

    AISystem::~AISystem()
    {
        delete[] m_reservedBits;
    }

    void AISystem::ClearReservations()
    {
        for (int i = 0; i < m_reservedNumWords; ++i)
            m_reservedBits[i] = 0;
    }

    bool AISystem::ReserveTile(int x, int y)
    {
        int width = m_map->GetWidth() * 2;
        int idx = y * width + x;
        int word = idx >> 5;
        LONG bit = 1L << (idx & 31);

        LONG oldVal, newVal;
        do {
            oldVal = m_reservedBits[word];
            if (oldVal & bit) return false;
            newVal = oldVal | bit;
        } while (InterlockedCompareExchange(&m_reservedBits[word], newVal, oldVal) != oldVal);
        return true;
    }

     void AISystem::Update(float deltaTime) {
         // Update decision timer for strategic AI (decide what to build every 3 seconds)
         m_decisionTimer += deltaTime;
         if (m_decisionTimer >= 3.0f) {
             m_decisionTimer = 0.0f;
             UpdateDecisionList();
         }

         // Build based on decision list (find location every frame)
         if (!m_decisionList.empty()) {
             // Try to build the first item in the decision list
             World::BuildingType type = m_decisionList.front();
             BuildRequest req;
             if (PlanBuild(type, req)) {
                 ApplyBuild(req);
                 // Remove from list after successful build attempt
                 m_decisionList.erase(m_decisionList.begin());
             }
         }
     }

    bool AISystem::PlanBuild(World::BuildingType type, BuildRequest& outReq) {
        if (!m_map || !m_economy) return false;
        if (HasBuilding(type)) return false;

        int idx = static_cast<int>(type);
        if (idx >= 0 && idx < MAX_BUILDING_TYPE && m_siteCacheValid[idx])
        {
            outReq.type = type;
            outReq.x = m_siteCache[idx].x;
            outReq.y = m_siteCache[idx].y;
            if (ReserveTile(outReq.x, outReq.y))
                return true;
            // Reservation failed (taken by another chunk) — fall through to re-scan
        }

        int originX = m_map->GetWidth();
        int originY = m_map->GetHeight() * 2;

        int resX = 0, resY = 0;
        bool needsResource = FindResourceForBuilding(m_map, type, originX, originY, resX, resY);

        int placeX, placeY;
        bool foundSpot = false;

        if (needsResource) {
            foundSpot = FindAdjacentBuildable(m_map, resX, resY, placeX, placeY);
        }

        if (!foundSpot) {
            foundSpot = FindBuildableTile(m_map, originX, originY, 15, placeX, placeY);
        }

        if (foundSpot) {
            if (!ReserveTile(placeX, placeY))
                return false;

            outReq.type = type;
            outReq.x = placeX;
            outReq.y = placeY;

            if (idx >= 0 && idx < MAX_BUILDING_TYPE)
            {
                m_siteCache[idx].x = static_cast<short>(placeX);
                m_siteCache[idx].y = static_cast<short>(placeY);
                m_siteCacheValid[idx] = true;
            }
            return true;
        }
        return false;
    }

    void AISystem::ApplyBuildRequests(const BuildRequest* requests, int numRequests) {
        if (!m_map) return;

        for (int i = 0; i < numRequests; ++i)
        {
            if (HasBuilding(requests[i].type))
                continue;

            const World::Tile& bldTile = m_map->GetTile(World::Buildings, requests[i].x, requests[i].y);
            if (bldTile.type != World::Tile_None) continue;

            ApplyBuild(requests[i]);
        }
    }

    void AISystem::ApplyBuild(const BuildRequest& req) {
        World::Building* b = CreateBuilding(req.type, req.x, req.y);
        if (b) {
            m_economy->AddBuilding(b);
        }
    }

    World::Building* AISystem::CreateBuilding(World::BuildingType type, int x, int y) {
        switch (type)
        {
            case World::Woodcutter:    return new class World::Woodcutter(x, y, m_playerID, m_map);
            case World::CoalMine:      return new class World::CoalMine(x, y, m_playerID, m_map);
            case World::IronMine:      return new class World::IronMine(x, y, m_playerID, m_map);
            case World::GoldMine:      return new class World::GoldMine(x, y, m_playerID, m_map);
            case World::Sawmill:       return new class World::Sawmill(x, y, m_playerID, m_map);
            case World::Stonemason:    return new class World::Stonemason(x, y, m_playerID, m_map);
            case World::Farm:          return new class World::Farm(x, y, m_playerID, m_map);
            case World::Mill:          return new class World::Mill(x, y, m_playerID, m_map);
            case World::Bakery:        return new class World::Bakery(x, y, m_playerID, m_map);
            case World::Hunter:        return new class World::Hunter(x, y, m_playerID, m_map);
            case World::Fisher:        return new class World::Fisher(x, y, m_playerID, m_map);
            case World::ToolWorkshop:  return new class World::ToolWorkshop(x, y, m_playerID, m_map);
            case World::IronSmelter:   return new class World::IronSmelter(x, y, m_playerID, m_map);
            case World::GoldSmelter:   return new class World::GoldSmelter(x, y, m_playerID, m_map);
            default: return NULL;
        }
    }

    bool AISystem::HasBuilding(World::BuildingType type) {
        return m_economy ? m_economy->HasBuilding(type) : false;
    }

    void AISystem::UpdateDecisionList()
    {
        // Clear current decision list
        m_decisionList.clear();

        // Simple strategy: prioritize based on what we can afford and what we need
        // For now, just cycle through some basic buildings in a reasonable order
        static const World::BuildingType buildingOrder[] = {
            World::Woodcutter,    // Need wood for everything
            World::Sawmill,       // Turn wood into planks
            World::Farm,          // Need food
            World::Mill,          // Turn wheat into flour
            World::Bakery,        // Turn flour into bread
            World::CoalMine,      // Need coal for smelting
            World::IronMine,      // Need iron ore
            World::IronSmelter,   // Turn ore into bars
            World::ToolWorkshop,  // Turn bars into tools
            World::Stonemason,    // Need stone
            World::Hunter,        // Need meat
            World::Fisher,        // Need fish
            World::GoldMine,      // Need gold ore
            World::GoldSmelter    // Turn gold ore into bars
        };

        // Add buildings to decision list if we don't have them yet
        for (int i = 0; i < sizeof(buildingOrder) / sizeof(buildingOrder[0]); ++i) {
            World::BuildingType type = buildingOrder[i];
            if (!HasBuilding(type)) {
                m_decisionList.push_back(type);
            }
        }

        // If we have all basic buildings, add some variety
        if (m_decisionList.empty()) {
            static const World::BuildingType extraBuildings[] = {
                World::Woodcutter, World::Sawmill, World::Farm, 
                World::Mill, World::Bakery, World::Stonemason
            };
            for (int i = 0; i < sizeof(extraBuildings) / sizeof(extraBuildings[0]); ++i) {
                m_decisionList.push_back(extraBuildings[i]);
            }
        }
    }

} // namespace Logic
