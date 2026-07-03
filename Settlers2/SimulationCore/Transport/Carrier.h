#pragma once

#include <vector>
#include <stdint.h>
#include "../Core/Vector2i.h"
#include "../Core/Handle.h"
#include "Cargo.h"
#include "TransportTypes.h"
#include "TransportTask.h"

#define MAX_TRANSIT_TILES 4096

namespace World {

    struct Road;
    class Flag;
    class FlagManager;
    class RoadManager;
    class DemandManager;
    class CargoManager;
    class TransportController;

    enum MovementAuthority {
        Legacy,
        ECS
    };

    enum CarrierState {
        WalkingToPost,
        Working,
        ReturningHome
    };

    inline bool IsTransitState(CarrierState s) {
        return s == WalkingToPost || s == ReturningHome;
    }

    class Carrier {
    public:
        Road* road;
        float ep;
        float walkDir;
        Cargo* m_cargo;

        CarrierState state;
        Vector2i transitTiles[MAX_TRANSIT_TILES];
        uint32_t transitCount;
        float transitProgress;
        bool readyToRemove;
        uint32_t ecsEntity;
        MovementAuthority m_authority;
        uint32_t pathVersion;

        DemandManager* m_demandManager;
        CargoManager* m_cargoManager;
        RoadManager* m_roadManager;
        Flag* m_roadEndpointA;
        Flag* m_roadEndpointB;

        float m_idleCheckTimer;
        bool m_returningToCenter;

        TransportTask* m_phase7Task;
        FlagId m_phase7TargetFlag;
        Cargo* m_phase7Cargo;
        TransportController* m_phase7Controller;

        Carrier(Road* r)
            : road(r), ep(0.0f), walkDir(1.0f), m_cargo(NULL),
              state(Working), transitCount(0), transitProgress(0.0f), readyToRemove(false),
              ecsEntity(0), m_authority(Legacy), pathVersion(0),
              m_demandManager(NULL), m_cargoManager(NULL), m_roadManager(NULL),
              m_roadEndpointA(NULL), m_roadEndpointB(NULL), m_idleCheckTimer(0.0f),
              m_returningToCenter(false),
              m_phase7Task(NULL), m_phase7TargetFlag(0), m_phase7Cargo(NULL), m_phase7Controller(NULL)
        {
        }

        ~Carrier() {}

        float GetPathLen() const;
        float GetCenterEp() const;
        float GetFlagEp(Flag* f) const;

        void SetupWalkingToPost(const std::vector<Vector2i>& tiles) {
            if (tiles.size() < 2) { state = Working; return; }
            transitCount = (tiles.size() < MAX_TRANSIT_TILES) ? (uint32_t)tiles.size() : MAX_TRANSIT_TILES;
            for (uint32_t i = 0; i < transitCount; ++i) transitTiles[i] = tiles[i];
            transitProgress = 0.0f;
            state = WalkingToPost;
            ++pathVersion;
        }

        void SetupReturningHome(const std::vector<Vector2i>& tiles) {
            if (tiles.size() < 2) { readyToRemove = true; return; }
            transitCount = (tiles.size() < MAX_TRANSIT_TILES) ? (uint32_t)tiles.size() : MAX_TRANSIT_TILES;
            for (uint32_t i = 0; i < transitCount; ++i) transitTiles[i] = tiles[i];
            transitProgress = 0.0f;
            state = ReturningHome;
            ++pathVersion;
            m_cargo = NULL;
        }

        void SyncFromECS(float syncEp, float syncWalkDir, CarrierState syncState) {
            ep = syncEp;
            walkDir = syncWalkDir;
            state = syncState;
        }

        void SyncToECS(float& outEp, float& outWalkDir, CarrierState& outState) const {
            outEp = ep;
            outWalkDir = walkDir;
            outState = state;
        }

        void AssignPhase7Task(TransportTask* task, FlagId targetFlag) {
            m_phase7Task = task;
            m_phase7TargetFlag = targetFlag;
        }

        bool IsCarrying() const { return m_cargo != NULL; }

        void Update(float deltaTime);
        void UpdateWalkingToPost(float deltaTime);

        void UpdateReturningHome(float deltaTime) {
            if (transitCount < 2) { readyToRemove = true; return; }
            float pathLen = (float)(transitCount - 1);
            transitProgress += deltaTime * 3.0f;
            if (transitProgress >= pathLen) {
                transitProgress = pathLen;
                readyToRemove = true;
            }
            walkDir = 1.0f;
        }
    };

    typedef Handle<Carrier> CarrierHandle;

} // namespace World
