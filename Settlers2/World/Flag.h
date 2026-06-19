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

namespace World {
    static const uint32_t INVALID_FLAG_ID = 0xFFFFFFFF;
    static const uint32_t FLAG_MAX_CARGO = 8;

    class Building;
    struct Road;
    class Flag;
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
        int reserved; // committed to pending TransportJobs, not available for new requests
        uint32_t destFlagId; // ID of ultimate destination flag (0 = no routing, safe across flag deletion)

        ResourceSlot() : type(ResourceType_None), amount(0), reserved(0), destFlagId(INVALID_FLAG_ID) {}
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
                    total += slots[i].amount - slots[i].reserved;
            }
            return total;
        }

        bool Reserve(ResourceType type, int amount, uint32_t destFlagId = INVALID_FLAG_ID) {
            int idx = FindSlot(type, destFlagId);
            if (idx < 0) return false;
            if (slots[idx].amount - slots[idx].reserved < amount) return false;
            slots[idx].reserved += amount;
            assert(slots[idx].reserved <= slots[idx].amount);
            char buf[256];
            _snprintf(buf, sizeof(buf),
                "[RESERVE] slot=%d type=%s amount=%d reserved=%d destFlagId=%u flag=%u(%d,%d)\n",
                idx, ResourceTypeToString(type), slots[idx].amount, slots[idx].reserved,
                destFlagId, id, pos.x, pos.y);
            OutputDebugStringA(buf);
            return true;
        }

        void Unreserve(ResourceType type, int amount, uint32_t destFlagId = INVALID_FLAG_ID) {
            int idx = FindSlot(type, destFlagId);
            if (idx >= 0) {
                slots[idx].reserved -= amount;
                if (slots[idx].reserved < 0) slots[idx].reserved = 0;
            }
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
                slots[idx].reserved = 0;
                slots[idx].destFlagId = destFlagId;
                return true;
            }
            return false;
        }

        bool RemoveResource(ResourceType type, int amount, uint32_t destFlagId = INVALID_FLAG_ID) {
            int idx = FindSlot(type, destFlagId);
            if (idx < 0) return false;
            if (slots[idx].amount - slots[idx].reserved < amount) return false;
            slots[idx].amount -= amount;
            if (slots[idx].amount <= 0) {
                slots[idx].type = ResourceType_None;
                slots[idx].amount = 0;
                slots[idx].reserved = 0;
                slots[idx].destFlagId = INVALID_FLAG_ID;
            }
            return true;
        }

        void CommitPickup(ResourceType type, int amount, uint32_t destFlagId = INVALID_FLAG_ID) {
            int idx = FindSlot(type, destFlagId);
            if (idx < 0) return;
            if (amount <= 0) return;
            int reservedBefore = slots[idx].reserved;
            int toRemove = (amount < slots[idx].amount) ? amount : slots[idx].amount;
            if (toRemove <= 0) return;
            if (slots[idx].reserved >= toRemove) {
                slots[idx].reserved -= toRemove;
            } else {
                slots[idx].reserved = 0;
            }
            slots[idx].amount -= toRemove;
            char buf[256];
            _snprintf(buf, sizeof(buf),
                "[PICKUP] slot=%d type=%s amount=%d reserved=%d->%d flag=%u(%d,%d)\n",
                idx, ResourceTypeToString(type), toRemove,
                reservedBefore, slots[idx].reserved, id, pos.x, pos.y);
            OutputDebugStringA(buf);
            if (slots[idx].amount <= 0) {
                slots[idx].type = ResourceType_None;
                slots[idx].amount = 0;
                slots[idx].reserved = 0;
                slots[idx].destFlagId = INVALID_FLAG_ID;
            }
        }

        // New Cargo-based storage (replaces ResourceSlot in migration)
        std::vector<Cargo*> cargo;

        bool AcceptCargo(Cargo* c) {
            if (cargo.size() >= FLAG_MAX_CARGO) return false;
            char buf[256];
            _snprintf(buf, sizeof(buf),
                "[Flag] AcceptCargo id=%u type=%s thisFlag(id=%u handleIdx=%u) cargoCount=%u (currentFlag before=%u)\n",
                c->id, ResourceTypeToString(c->type), id, handle.index, (unsigned)cargo.size(),
                c->currentFlag.index);
            OutputDebugStringA(buf);
            c->state = Cargo_OnFlag;
            c->currentFlag = handle;
            cargo.push_back(c);
            _snprintf(buf, sizeof(buf),
                "[Flag] AcceptCargo id=%u currentFlag(after=%u)\n",
                c->id, c->currentFlag.index);
            OutputDebugStringA(buf);
            return true;
        }

        Cargo* GetCargoForRoad(Road* road, DemandManager* dm, CargoManager* cm) {
            for (size_t i = 0; i < cargo.size(); ++i) {
                Cargo* c = cargo[i];
                if (c->state != Cargo_OnFlag) continue;
                if (!c->ticket) continue;
                if (c->ticket->state == Ticket_Cancelled || !c->ticket->demand) {
                    // Try to reassign on the fly
                    char buf[256];
                    _snprintf(buf, sizeof(buf),
                        "[Cargo] Reassign id=%u type=%s oldTicket=%u (during pickup)\n",
                        c->id, ResourceTypeToString(c->type), c->ticket->id);
                    OutputDebugStringA(buf);
                    if (dm) dm->ReleaseTicket(c->ticket);
                    c->ticket = NULL;
                    if (dm) {
                        DemandTicket* newTicket = dm->Reserve(c->type);
                        if (newTicket) {
                            c->ticket = newTicket;
                            _snprintf(buf, sizeof(buf),
                                "[Cargo] Reassigned id=%u type=%s ticket=%u\n",
                                c->id, ResourceTypeToString(c->type), newTicket->id);
                            OutputDebugStringA(buf);
                        } else {
                            continue; // skip this cargo, no demand available
                        }
                    } else {
                        continue; // no demand manager, skip
                    }
                }
                if (c->ticket->state == Ticket_Active)
                    return c;
            }
            return NULL;
        }

        uint32_t GetCargoCount() const { return (uint32_t)cargo.size(); }

        Cargo* TakeCargoForRoad(Road* road = NULL, DemandManager* dm = NULL, CargoManager* cm = NULL) {
            // Note: existing Cargo pickup is done by Carrier via routing-aware loop before calling this.
            // This function only converts ResourceSlot entries to Cargo.

            // Convert a ResourceSlot entry to Cargo
            if (dm && cm) {
                for (int si = 0; si < 8; ++si) {
                    ResourceSlot& slot = slots[si];
                    if (slot.type == ResourceType_None || slot.amount <= 0) continue;
                    if (slot.amount - slot.reserved <= 0) continue;

                    DemandTicket* ticket = dm->Reserve(slot.type);
                    if (!ticket) continue; // no demand exists for this resource

                    // Don't pick up resources whose demand targets this same flag — they're already at destination
                    if (ticket->demand && ticket->demand->targetFlag == handle) {
                        dm->ReleaseTicket(ticket);
                        continue;
                    }

                    Cargo* c = cm->Allocate(slot.type, 1, handle);
                    c->ticket = ticket;

                    slot.amount -= 1;
                    if (slot.amount <= 0) {
                        slot.type = ResourceType_None;
                        slot.amount = 0;
                        slot.reserved = 0;
                        slot.destFlagId = INVALID_FLAG_ID;
                    }
                    return c;
                }
            }

            return NULL;
        }

        void CheckDeliveries(DemandManager* dm, CargoManager* cm) {
            for (size_t i = 0; i < cargo.size(); ) {
                Cargo* c = cargo[i];
                if (c->state == Cargo_OnFlag && c->ticket) {
                    if (c->ticket->state == Ticket_Cancelled || !c->ticket->demand) {
                        // Ticket was cancelled — reassign cargo to a new demand
                        char buf[256];
                        _snprintf(buf, sizeof(buf),
                            "[Cargo] Reassign id=%u type=%s oldTicket=%u\n",
                            c->id, ResourceTypeToString(c->type), c->ticket->id);
                        OutputDebugStringA(buf);
                        if (dm) dm->ReleaseTicket(c->ticket);
                        c->ticket = NULL;
                        if (dm) {
                            DemandTicket* newTicket = dm->Reserve(c->type);
                            if (newTicket) {
                                c->ticket = newTicket;
                                _snprintf(buf, sizeof(buf),
                                    "[Cargo] Reassigned id=%u type=%s ticket=%u\n",
                                    c->id, ResourceTypeToString(c->type), newTicket->id);
                                OutputDebugStringA(buf);
                            } else {
                                _snprintf(buf, sizeof(buf),
                                    "[Cargo] No demand for id=%u type=%s — keeping on flag\n",
                                    c->id, ResourceTypeToString(c->type));
                                OutputDebugStringA(buf);
                            }
                        }
                        ++i;
                        continue;
                    }

                    if (c->ticket->state == Ticket_Active && c->ticket->demand &&
                        c->ticket->demand->targetFlag.index == handle.index &&
                        c->ticket->demand->targetFlag.generation == handle.generation)
                    {
                        // This cargo has reached its destination flag
                        // Add to ResourceSlot so buildings/warehouse can consume it
                        if (!AddResource(c->type, c->amount)) {
                            // All 8 slots full — retry next frame instead of dropping cargo
                            ++i;
                            continue;
                        }

                        DemandTicket* ticket = c->ticket;
                        uint32_t id = c->id;
                        cargo.erase(cargo.begin() + i);
                        if (dm) dm->Deliver(ticket);
                        if (cm) cm->Release(id);
                        continue;
                    }
                }
                ++i;
            }
        }
    };
}
