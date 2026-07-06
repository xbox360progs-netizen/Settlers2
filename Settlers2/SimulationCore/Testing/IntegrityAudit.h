#pragma once
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "../Simulation/Simulation.h"
#include "../World/WorldModel.h"
#include "../Transport/TransportController.h"
#include "../Transport/Cargo.h"
#include "../Transport/TransportTask.h"
#include "../Core/ResourceTypes.h"
#include "../Core/ResourceDebug.h"
#include "../Core/BuildingTypes.h"
#include "../Core/WorkerTypes.h"
#include "../Systems/DemandManager.h"

namespace World {

    static const int kAuditMaxDetails = 32;
    static const int kAuditDetailLen = 128;

    struct AuditReport {
        bool objectLiveness;
        bool ownership;
        bool conservation;
        bool queueConsistency;
        int detailCount;
        char details[kAuditMaxDetails][kAuditDetailLen];

        AuditReport()
            : objectLiveness(true), ownership(true), conservation(true), queueConsistency(true), detailCount(0)
        {
            for (int i = 0; i < kAuditMaxDetails; ++i)
                details[i][0] = '\0';
        }

        bool AllPassed() const {
            return objectLiveness && ownership && conservation && queueConsistency;
        }

        void Fail(const char* section, const char* fmt, ...) {
            if (detailCount >= kAuditMaxDetails) return;

            if (strcmp(section, "objectLiveness") == 0) objectLiveness = false;
            else if (strcmp(section, "ownership") == 0) ownership = false;
            else if (strcmp(section, "conservation") == 0) conservation = false;
            else if (strcmp(section, "queueConsistency") == 0) queueConsistency = false;

            va_list args;
            va_start(args, fmt);
            vsnprintf(details[detailCount], kAuditDetailLen - 1, fmt, args);
            va_end(args);
            detailCount++;
        }
    };

    inline AuditReport RunIntegrityAudit(Simulation& sim) {
        AuditReport r;
        const WorldModel& world = sim.GetWorld();
        const SimulationState& state = sim.GetState();

        // ═══════════════════════════════════════
        // 1. OBJECT LIVENESS
        // ═══════════════════════════════════════

        // Worker bounds
        if (world.workerCount > kMaxWorkers) {
            r.Fail("objectLiveness", "Workers: %d exceeds max %d", world.workerCount, kMaxWorkers);
        }
        for (int i = 0; i < world.workerCount && i < kMaxWorkers; ++i) {
            if (world.workers[i].state < WorkerState_Idle || world.workers[i].state > WorkerState_Working) {
                r.Fail("objectLiveness", "Worker[%d]: invalid state %d", i, (int)world.workers[i].state);
            }
        }

        // Production building bounds
        if (world.productionBuildingCount > kMaxProductionBuildings) {
            r.Fail("objectLiveness", "ProdBuildings: %d exceeds max %d",
                world.productionBuildingCount, kMaxProductionBuildings);
        }
        for (int i = 0; i < world.productionBuildingCount && i < kMaxProductionBuildings; ++i) {
            if (world.productionBuildings[i].type <= BuildingType_None ||
                world.productionBuildings[i].type >= BuildingType_Count) {
                r.Fail("objectLiveness", "ProdBuilding[%d]: invalid type %d",
                    i, (int)world.productionBuildings[i].type);
            }
        }

        // Bounds checks
        if (world.pendingRequestCount > kMaxPendingRequests) {
            r.Fail("objectLiveness", "PendingRequests: %d exceeds max %d",
                world.pendingRequestCount, kMaxPendingRequests);
        }
        if (world.pendingConstructionCount > kMaxConstructionRequests) {
            r.Fail("objectLiveness", "PendingConstruction: %d exceeds max %d",
                world.pendingConstructionCount, kMaxConstructionRequests);
        }
        if (world.activeSiteCount > kMaxConstructionSites) {
            r.Fail("objectLiveness", "ActiveSites: %d exceeds max %d",
                world.activeSiteCount, kMaxConstructionSites);
        }
        if (world.deliveryEventCount > kMaxDeliveryEvents) {
            r.Fail("objectLiveness", "DeliveryEvents: %d exceeds max %d",
                world.deliveryEventCount, kMaxDeliveryEvents);
        }

        // ═══════════════════════════════════════
        // 2. OWNERSHIP AUDIT
        // ═══════════════════════════════════════

        int cargoCount = sim.GetCargoCount();
        const TransportController* tc = sim.GetTransportController();

        // Validate each cargo's ownership link
        for (int ci = 0; ci < cargoCount; ++ci) {
            const Cargo* cargo = sim.GetCargoAt(ci);
            if (cargo == NULL) {
                r.Fail("ownership", "Cargo[%d]: NULL pointer in pool", ci);
                continue;
            }
            if (cargo->type <= ResourceType_None || cargo->type >= ResourceType_Count) {
                r.Fail("ownership", "Cargo %u: invalid ResourceType %d", cargo->id, (int)cargo->type);
            }
            if (cargo->ownerTask == NULL) {
                r.Fail("ownership", "Cargo %u: no ownerTask", cargo->id);
            }
        }

        // Validate task->cargo back-links
        if (tc != NULL) {
            const TransportTask* pool = tc->GetTaskPool();
            int poolSize = tc->GetPoolSize();
            for (int ti = 0; ti < poolSize; ++ti) {
                const TransportTask& task = pool[ti];
                if (task.state == TTS_Delivered || task.state == TTS_Cancelled) continue;

                if (task.cargo != NULL) {
                    bool found = false;
                    for (int ci = 0; ci < cargoCount; ++ci) {
                        const Cargo* c = sim.GetCargoAt(ci);
                        if (c == task.cargo) {
                            found = true;
                            if (c->ownerTask != &task) {
                                r.Fail("ownership", "Task %u -> Cargo %u: broken back-link (cargo.ownerTask=%p)",
                                    task.id, c->id, (void*)c->ownerTask);
                            }
                            break;
                        }
                    }
                    if (!found) {
                        r.Fail("ownership", "Task %u: cargo ptr %p not in alive cargo pool",
                            task.id, (void*)task.cargo);
                    }
                }
            }
        }

        // ═══════════════════════════════════════
        // 3. CONSERVATION
        // ═══════════════════════════════════════

        const EconomySystem* eco = sim.GetEconomySystem();
        const WarehouseSystem* wh = sim.GetWarehouseSystem();

        if (eco != NULL) {
            // Cargo amounts per resource type
            int cargoAmount[ResourceType_Count];
            memset(cargoAmount, 0, sizeof(cargoAmount));
            for (int ci = 0; ci < cargoCount; ++ci) {
                const Cargo* c = sim.GetCargoAt(ci);
                if (c != NULL && c->type > ResourceType_None && c->type < ResourceType_Count) {
                    cargoAmount[(int)c->type] += (int)c->amount;
                }
            }

            static const ResourceType kKey[] = {
                ResourceType_Wood, ResourceType_Planks, ResourceType_Stone, ResourceType_Tools
            };
            static const int kKeyCount = 4;

            for (int ri = 0; ri < kKeyCount; ++ri) {
                ResourceType rt = kKey[ri];
                int t = (int)rt;

                int produced = eco->GetTotalProduced(rt);

                // Stock = warehouse stockpile + cargo in transit.
                // (outputBuffer is NOT current stock — it is a cumulative
                // production counter incremented alongside totalOutput.)
                int stockpile = (wh != NULL) ? wh->GetStockpileAmount(rt) : 0;
                int inStock = stockpile + cargoAmount[t];

                // No phantom resources: stock cannot exceed lifetime production.
                // This catches resource duplication bugs.
                if (inStock > produced) {
                    r.Fail("conservation", "%s: stock %d > produced %d",
                        ResourceTypeToString(rt), inStock, produced);
                }

                // For Tools (no production consumption, zero construction use):
                // produced - inStock should always be >= 0.
                // A negative value means Tools disappeared without consumption.
                if (rt == ResourceType_Tools) {
                    int residual = produced - inStock;
                    if (residual < -5) {
                        r.Fail("conservation", "%s: possible leak: produced %d, stock %d",
                            ResourceTypeToString(rt), produced, inStock);
                    }
                }
            }
        }

        // ═══════════════════════════════════════
        // 4. QUEUE CONSISTENCY
        // ═══════════════════════════════════════

        // Transport task count: state matches controller
        if (tc != NULL) {
            int activeFromCtrl = tc->GetActiveTaskCount();
            int blockedFromCtrl = tc->GetBlockedCount();
            if (activeFromCtrl != state.activeTransportTasks) {
                r.Fail("queueConsistency", "Transport tasks: simState=%d, controller=%d",
                    state.activeTransportTasks, activeFromCtrl);
            }
        }

        // No negative counts in WorldModel
        if (world.pendingRequestCount < 0) {
            r.Fail("queueConsistency", "pendingRequestCount < 0: %d", world.pendingRequestCount);
        }

        // Check DemandManager invariants
        const DemandManager* dm = sim.GetDemandManager();
        if (dm != NULL) {
            int demandCount = dm->GetDemandCount();
            static const int kMaxDemands = 64;
            if (demandCount > kMaxDemands) {
                r.Fail("queueConsistency", "Demands: %d exceeds %d", demandCount, kMaxDemands);
            }
            for (int di = 0; di < demandCount; ++di) {
                ResourceType dt = dm->GetDemandType(di);
                if (dt <= ResourceType_None || dt >= ResourceType_Count) {
                    r.Fail("queueConsistency", "Demand[%d]: invalid type %d", di, (int)dt);
                }
            }
        }

        return r;
    }

    inline void PrintAuditReport(const AuditReport& r, const char* name, uint32_t tick) {
        printf("\n=== Integrity Audit [%s] tick=%u ===\n", name, tick);
        printf("  Object Liveness:    %s\n", r.objectLiveness    ? "PASS" : "FAIL");
        printf("  Ownership:          %s\n", r.ownership          ? "PASS" : "FAIL");
        printf("  Conservation:       %s\n", r.conservation       ? "PASS" : "FAIL");
        printf("  Queue Consistency:  %s\n", r.queueConsistency   ? "PASS" : "FAIL");

        if (r.detailCount > 0) {
            printf("  Details (%d):\n", r.detailCount);
            for (int i = 0; i < r.detailCount; ++i) {
                printf("    %s\n", r.details[i]);
            }
        }
    }

} // namespace World
