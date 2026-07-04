#pragma once
#include <stdint.h>
#include "../Core/ResourceTypes.h"
#include "../Transport/TransportTypes.h"
#include "../Construction/ConstructionRequest.h"
#include "../Construction/ConstructionSite.h"
#include "../Core/BuildingTypes.h"
#include "../Core/DemandTypes.h"
#include "../Core/WorkerTypes.h"
#include "../Core/JobTypes.h"

namespace World {

    enum TreeGrowthState {
        TGS_Empty   = 0,
        TGS_Sapling = 1,
        TGS_Young   = 2,
        TGS_Mature  = 3,
        TGS_Stump   = 4,
    };

    static const int kMaxPendingRequests = 128;
    static const int kMaxConstructionRequests = 128;
    static const int kMaxConstructionSites = 64;
    static const int kMaxDeliveryEvents = 64;
    static const int kMaxProductionBuildings = 64;
    static const int kMaxProductionInputs = 4;
    static const int kMaxWorkers = 32;
    static const uint8_t kNoDemand = 0xFF;

    struct TransportRequest {
        ResourceType resource;
        FlagId origin;
        FlagId destination;
        TransportTaskReason reason;
        DemandOwner owner;
        bool fulfilled;
        uint8_t demandIndex;
    };

    enum DeliveryEventType {
        DET_Completed
    };

    struct DeliveryEvent {
        DeliveryEventType type;
        ResourceType resource;
        int amount;
        FlagId destinationFlag;
        TransportTaskReason reason;

        DeliveryEvent()
            : type(DET_Completed)
            , resource(ResourceType_None)
            , amount(0)
            , destinationFlag(0)
            , reason(TTR_Construction)
        {
        }
    };

    struct ProductionBuilding {
        BuildingType type;
        Vector2i position;
        int owner;
        int cycleTimer;
        bool active;
        bool inputsRequested;
        bool fed;
        bool foodRequested;
        int foodStored;
        ResourceType inputResources[kMaxProductionInputs];
        int inputRequired[kMaxProductionInputs];
        int inputDelivered[kMaxProductionInputs];
        ResourceType outputResources[kMaxProductionInputs];
        int outputBuffer[kMaxProductionInputs];
        int totalOutput[kMaxProductionInputs];

        ProductionBuilding()
            : type(BuildingType_None)
            , position(0, 0)
            , owner(0)
            , cycleTimer(0)
            , active(false)
            , inputsRequested(false)
            , fed(false)
            , foodRequested(false)
            , foodStored(0)
        {
            for (int i = 0; i < kMaxProductionInputs; ++i) {
                inputResources[i] = ResourceType_None;
                inputRequired[i] = 0;
                inputDelivered[i] = 0;
                outputResources[i] = ResourceType_None;
                outputBuffer[i] = 0;
                totalOutput[i] = 0;
            }
        }
    };

    struct Worker {
        WorkerId id;
        WorkerState state;
        uint8_t currentJob;
        uint16_t workTicksRemaining;

        Worker()
            : id(0)
            , state(WorkerState_Idle)
            , currentJob(0)
            , workTicksRemaining(0)
        {
        }
    };

    struct WorldModel {
        uint32_t width;
        uint32_t height;

        TransportRequest pendingRequests[kMaxPendingRequests];
        int pendingRequestCount;

        ConstructionRequest pendingConstructionRequests[kMaxConstructionRequests];
        int pendingConstructionCount;

        ConstructionSite activeSites[kMaxConstructionSites];
        int activeSiteCount;

        DeliveryEvent deliveryEvents[kMaxDeliveryEvents];
        int deliveryEventCount;

        JobEvent jobEvents[kMaxJobEvents];
        int jobEventCount;

        ProductionBuilding productionBuildings[kMaxProductionBuildings];
        int productionBuildingCount;

        Worker workers[kMaxWorkers];
        int workerCount;

        int treeMatureCount;
        int treeYoungCount;
        int treeSaplingCount;
        int treeStumpCount;
        int treeEmptySpots;

        int animalCount;
        int maxAnimalCount;

        int fishCount;
        int maxFishCount;

        WorldModel()
            : width(0)
            , height(0)
            , pendingRequestCount(0)
            , pendingConstructionCount(0)
            , activeSiteCount(0)
            , deliveryEventCount(0)
            , jobEventCount(0)
            , productionBuildingCount(0)
            , workerCount(0)
            , treeMatureCount(0)
            , treeYoungCount(0)
            , treeSaplingCount(0)
            , treeStumpCount(0)
            , treeEmptySpots(0)
            , animalCount(0)
            , maxAnimalCount(0)
            , fishCount(0)
            , maxFishCount(0)
        {
        }
    };

} // namespace World
