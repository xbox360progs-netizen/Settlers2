#pragma once
#include <vector>
#include <stdint.h>
#include <cassert>
#include "../Core/Vector2i.h"
#include "ResourceNode.h"
#include "Components/Building.h"
#include "ObjectState.h"
#include "Handle.h"
#include "Cargo.h"
#include "Demand.h"
#include "CargoManager.h"
#include "DemandManager.h"
#include "TransportController.h"

namespace World {
    static const uint32_t INVALID_FLAG_ID = 0xFFFFFFFF;
    static const uint32_t FLAG_MAX_CARGO = 8;

    class Building;
    struct Road;
    class Flag;
    class TransportController;
    typedef Handle<Flag> FlagHandle;

    enum FlagType {
        FLAG_NORMAL,
        FLAG_BUILDING,
        FLAG_WAREHOUSE,
        FLAG_MILITARY
    };

    struct FlagData {
        int x, y;
        uint32_t id;
        FlagType type;
        BuildingType pendingBuilding;
        bool hasBuilding;
    };

    struct ResourceSlot {
        ResourceType type;
        int amount;
        uint32_t destFlagId; // Ownership tag: who may claim this slot first (0=free, INVALID=unowned).
                              // NOT a routing field — TransportTask governs moving resources.
                              // Prevents collisions: Warehouse, ConstructionManager, CollectWarehouse
                              // check this before consuming a stationary resource.

        ResourceSlot() : type(ResourceType_None), amount(0), destFlagId(INVALID_FLAG_ID) {}
        void Clear() { type = ResourceType_None; amount = 0; destFlagId = INVALID_FLAG_ID; }
    };

    class Flag {
    public:
        uint32_t id;
        FlagHandle handle;
        Vector2i pos;
        FlagType type;
        Building* building;
        ResourceSlot slots[8];
        std::vector<Road*> roads;
        BuildingType pendingBuilding;
        bool hasBuilding;
        ObjectState state;

        Flag(int x, int y, uint32_t id)
            : id(id), type(FLAG_NORMAL), building(NULL), pendingBuilding(static_cast<BuildingType>(0)), hasBuilding(false), state(Active)
        {
            pos.x = x;
            pos.y = y;
        }

        int FindSlot(ResourceType type, uint32_t destFlagId = INVALID_FLAG_ID) const {
            for (int i = 0; i < 8; ++i) {
                if (slots[i].type == type) {
                    if (destFlagId == INVALID_FLAG_ID || slots[i].destFlagId == destFlagId)
                        return i;
                }
            }
            return -1;
        }

        int FindEmptySlot() const {
            for (int i = 0; i < 8; ++i) {
                if (slots[i].type == ResourceType_None)
                    return i;
            }
            return -1;
        }

        int GetAvailable(ResourceType type) const {
            int total = 0;
            for (int i = 0; i < 8; ++i) {
                if (slots[i].type == type)
                    total += slots[i].amount;
            }
            return total;
        }

        bool AddResource(ResourceType type, int amount, uint32_t destFlagId = INVALID_FLAG_ID) {
            int idx = FindSlot(type, destFlagId);
            if (idx >= 0) {
                slots[idx].amount += amount;
                return true;
            }
            idx = FindEmptySlot();
            if (idx >= 0) {
                slots[idx].type = type;
                slots[idx].amount = amount;
                slots[idx].destFlagId = destFlagId;
                return true;
            }
            return false;
        }

        bool RemoveResource(ResourceType type, int amount, uint32_t destFlagId = INVALID_FLAG_ID) {
            int idx = FindSlot(type, destFlagId);
            if (idx < 0) return false;
            if (slots[idx].amount < amount) return false;
            slots[idx].amount -= amount;
            if (slots[idx].amount <= 0) {
                slots[idx].type = ResourceType_None;
                slots[idx].amount = 0;
                slots[idx].destFlagId = INVALID_FLAG_ID;
            }
            return true;
        }

        bool AcceptCargo(Cargo* c) {
            char buf[256];
            _snprintf(buf, sizeof(buf),
                "[Flag] AcceptCargo id=%u type=%s thisFlag(id=%u handleIdx=%u) (currentFlag before=%u)\n",
                c->id, ResourceTypeToString(c->type), id, handle.index,
                c->currentFlag.index);
            OutputDebugStringA(buf);
            c->state = Cargo_OnFlag;
            c->currentFlag = handle;
            _snprintf(buf, sizeof(buf),
                "[Flag] AcceptCargo id=%u currentFlag(after=%u)\n",
                c->id, c->currentFlag.index);
            OutputDebugStringA(buf);
            return true;
        }

        Cargo* TakeCargoForRoad(Road* road = NULL, DemandManager* dm = NULL, CargoManager* cm = NULL,
                                TransportController* tc = NULL) {
            // This function converts ResourceSlot entries to Cargo.
            if (dm && cm) {
                for (int si = 0; si < 8; ++si) {
                    ResourceSlot& slot = slots[si];
                    if (slot.type == ResourceType_None || slot.amount <= 0) continue;

                    DemandTicket* ticket = dm->Reserve(slot.type, id);
                    if (!ticket) continue; // no demand exists for this resource

                    // Don't pick up resources whose demand targets this same flag — they're already at destination
                    if (ticket->demand && ticket->demand->targetFlag == handle) {
                        dm->ReleaseTicket(ticket);
                        continue;
                    }

                    Cargo* c = cm->Allocate(slot.type, 1, handle);
                    // Phase 8.2 — wire ownerTask from the TransportTask created by Reserve()
                    {
                        TransportController* controller = tc ? tc : dm->GetController();
                        if (controller) {
                            c->ownerTask = controller->FindTask(ticket->transportTaskId);
                        }
                    }

                    slot.amount -= 1;
                    if (slot.amount <= 0) {
                        slot.Clear();
                    }
                    return c;
                }
            }

            return NULL;
        }


    };
}
