# TransportNode — Domain Contract

## Definition

**TransportNode** is the attachment point between Production and Transport. It owns a local resource buffer and knows which buildings are attached. It is the single interface through which Production speaks to Transport and vice versa.

## Responsibilities

| Owns | Does NOT own |
|------|-------------|
| Local resource storage (buffer) | Roads |
| Building attachment list | Carriers |
| Atomic buffer operations | Pathfinding |
| Incoming cargo receipt | Production logic |
| | Buildings themselves |
| | **Demand lifecycle** — DemandManager owns that |
| | **Allocation decisions** — LocalTransferSystem decides |
| | **Deficit evaluation** — LocalTransferSystem evaluates (has access to building state) |

## Demand Ownership

```
Building detects need  →  LocalTransferSystem evaluates remaining deficit
                               ↓
                    LocalTransferSystem writes pendingDemand[]
                               ↓
                     DemandManager OWNS the demand
                               ↓
                     TransportController executes
                               ↓
                          Carrier delivers
```

**Rule:** TransportController never creates demand. DemandManager is the single owner of all published demand. LocalTransferSystem evaluates deficits (with visibility into both node buffer and building input state). TransportNode stores pendingDemand but does not evaluate it.

## ResourceBuffer — Passive Storage Contract

`ResourceBuffer` is a dumb container. It has no logic:

```
Add(resource, amount)       — increment
Remove(resource, amount)    — decrement (returns actual removed)
Has(resource, amount)       — check availability
Count(resource)             — query amount
FindEmptySlot()             — query free slot index
```

**Does NOT:**
- search buildings
- reserve resources
- publish demand
- choose recipients
- know about attachments

All decisions live in `LocalTransferSystem`.

## HasCapacity Contract

`HasCapacity(resource, amount)` returns true only when the buffer can accept `amount` units of `resource`:

- **Resource already exists in buffer** → `Count(resource) + amount <= kNodeBufferSlots` (per-type limit)
- **Resource is new to this buffer** → at least one empty slot exists AND `amount <= kNodeBufferSlots`

This prevents both per-resource overflow and total slot exhaustion.

## BuildingAttachment — with Role

```cpp
enum AttachmentRole {
    Consumer,          // only consumes from buffer (e.g. weapon smelter)
    Producer,          // only produces into buffer (e.g. woodcutter)
    ProducerConsumer   // both (e.g. sawmill: consumes wood, produces planks)
};

struct BuildingAttachment {
    BuildingId id;
    AttachmentRole role;
    ResourceType inputs[];
    int inputCount;
};
```

Demand evaluation considers only `Consumer` and `ProducerConsumer` attachments. Pure `Producer` attachments never trigger demand.

## Node Does Not Decide

Node provides atomic operations only:

```
TakeForBuilding(id, resource, amount)    ← LocalTransferSystem decides WHO gets it
ReceiveExport(resource, amount)          ← LocalTransferSystem decides WHAT to export
```

**Node never answers:**
- "who should get this wood?"     → LocalTransferSystem decides
- "is this the best recipient?"    → LocalTransferSystem decides
- "should we wait for more?"       → LocalTransferSystem decides
- "is the deficit resolved?"       → LocalTransferSystem decides (has building.inputDelivered)

Node only:
- holds buffer
- tracks attachments
- provides atomic read/write operations
- stores pendingDemand[] (written by LocalTransferSystem)

## API

```cpp
class TransportNode {
    // Lifecycle
    void AttachBuilding(BuildingId id, const ResourceType inputs[], int inputCount, AttachmentRole role);
    void DetachBuilding(BuildingId id);

    // Incoming — called by external systems
    void ReceiveCargo(ResourceType resource, int amount);   // from Carrier
    void ReceiveExport(ResourceType resource, int amount);   // from LocalTransfer

    // Outgoing — called by LocalTransferSystem (Node does not decide who)
    bool TakeForBuilding(BuildingId id, ResourceType resource, int amount);

    // Query — no side effects
    int  GetBufferAmount(ResourceType resource) const;
    bool HasCapacity(ResourceType resource, int amount) const;
    bool HasDemandFor(ResourceType resource) const;
    int  FindDemand(ResourceType resource) const;

    // Heartbeat — evaluates deficits from buffer alone (standalone use without LocalTransferSystem)
    void Tick();
};
```

All methods are commands except the Query group. `TakeForBuilding` is a conditional command (atomically claims resource or returns false) — it is not a query. `FindDemand` is a public query for use by LocalTransferSystem.

## Structure

```cpp
struct TransportNode {
    NodeId id;

    ResourceBuffer buffer;

    BuildingAttachment attachments[];

    DemandSlot pendingDemand[];   // written by LocalTransferSystem
    int outgoingCount;            // written by LocalTransferSystem
};
```

## Tick Order (deterministic, integrated pipeline)

```
LocalTransferSystem::Tick():
  1. Export: building output → node buffer (ReceiveExport)
  2. Supply: node buffer → building input (TakeForBuilding + inputDelivered++)
  3. Evaluate remaining deficit per Consumer/ProducerConsumer attachment
     → writes pendingDemand[] for resources building STILL needs
  4. Calculate outgoingCount = buffer surplus not in active deficit

DemandManager reads pendingDemand[] in its own Tick.
```

Deficit evaluation uses `building.inputDelivered < building.inputRequired` to determine whether a resource is genuinely needed. This prevents false demand when local transfer already satisfied the building.

TransportNode::Tick() exists for standalone use (evaluates deficits from buffer level alone, without building state visibility). In the integrated pipeline, LocalTransferSystem replaces Tick() for deficit evaluation.

## Events

### Incoming

| Event | Origin | Mutates |
|-------|--------|---------|
| Carrier delivered cargo | `ReceiveCargo(resource, amount)` | buffer += amount |
| Building exported output | `ReceiveExport(resource, amount)` | buffer += amount |
| Building attached | `AttachBuilding(id, inputs, count, role)` | attachments[] += entry |
| Building detached | `DetachBuilding(id)` | attachments[] -= entry |

### Outgoing (emitted by LocalTransferSystem deficit evaluation)

| Event | Destination | When |
|-------|-------------|------|
| `pendingDemand[resource]` → DemandManager | DemandManager | buffer deficit for Consumer/ProducerConsumer, confirmed via building.inputDelivered |
| `outgoingCount` | Transport | buffer surplus, no local consumer needs it |

## Resource Ownership

| Stage | Owner | Transition |
|-------|-------|-----------|
| Building output buffer | Building | Building owns until exported |
| Building → Node (local) | LocalTransferSystem (logical) | `ReceiveExport` — no physical carrier, ownership moves atomically |
| Node buffer | TransportNode | `ReceiveExport` or `ReceiveCargo` → Node owns |
| Node → Node (transport) | Carrier (TransportTask) | Node decrements on pickup, Carrier owns during transit |
| Node buffer (after delivery) | TransportNode | `ReceiveCargo` → Node owns |
| Node → Building (local) | LocalTransferSystem (logical) | `TakeForBuilding` + building.inputDelivered++ — ownership moves atomically |
| Building input buffer | Building | LocalTransferSystem increments → Building owns |

**Rule:** At any Tick boundary, every resource unit is owned by exactly one entity. No resource exists in two places simultaneously. Local transfer is a logical operation — no physical carrier exists for local exchange.

## Demand Lifecycle

```
┌──────────────────────────────────────────────────────────┐
│  1. LocalTransferSystem detects need                      │
│     — building.inputDelivered < building.inputRequired    │
│     AND buffer.Count(resource) < quantity needed          │
│                                                          │
│  2. Demand Published  LocalTransferSystem writes          │
│                        pendingDemand[] on TransportNode   │
│                                                          │
│  3. Task Created      DemandManager reads pendingDemand,  │
│                        creates TransportTask               │
│                                                          │
│  4. Cargo Departs     TransportController dispatches       │
│                        Carrier, source node buffer         │
│                        decremented on pickup               │
│                                                          │
│  5. Cargo Arrives     ReceiveCargo() called,               │
│                        destination buffer incremented      │
│                                                          │
│  6. Need Satisfied    At next Tick, LocalTransferSystem    │
│                        supplies building from buffer.      │
│                        If inputDelivered >= inputRequired, │
│                        no demand published.               │
└──────────────────────────────────────────────────────────┘
```

**Guard:** Demand for resource R is published at most once per Tick per node. LocalTransferSystem deduplicates by `FindDemand(resource)` before adding.

## Invariants

1. **Buffer is single source of truth.** No external system writes to buffer directly — only `ReceiveCargo` or `ReceiveExport`.

2. **Demand is derived from buffer + building input state.** If no attached Consumer/ProducerConsumer building still needs resource R (inputDelivered >= inputRequired), no demand for R is published, regardless of buffer level.

3. **Attachment is exclusive.** A building is attached to exactly one TransportNode. A Node may have zero or more attached buildings.

4. **Local transfer before deficit evaluation.** LocalTransferSystem exports and supplies before evaluating remaining deficits. A deficit is published only after local transfer has exhausted all locally available resources against building needs.

5. **Tick order is deterministic.** Export → Supply → Evaluate → outgoingCount. Each stage completes before the next begins.

6. **Node never owns the building.** `DetachBuilding` severs the link; the building continues to exist independently.

7. **TakeForBuilding is atomic.** It either succeeds (buffer decremented, resource claimed) or fails (buffer unchanged). No partial state.

8. **No resource duplication.** A resource unit leaves one owner before entering the next. The handshake is always: source decrements → target increments.

9. **ResourceBuffer is passive.** It stores, adds, removes, and queries. No logic, no decisions.

10. **TransportController never creates demand.** DemandManager is the single owner of the demand lifecycle.

11. **Node does not allocate.** `TakeForBuilding` and `ReceiveExport` are operations, not decisions. Allocation and deficit evaluation are owned by `LocalTransferSystem`.

12. **ResourceBuffer is the canonical storage.** A resource type exists in exactly one place inside a Node — the buffer. No parallel inventory on the Node or its attachments. Building inventory is independent; Node never mirrors it.

13. **HasCapacity reflects actual buffer capacity.** For existing resources: per-type slot limit. For new resources: empty slot required.

## Relationship to Flag

`TransportNode` is the domain concept. `Flag` is its visual representation on the map. The current `Flag` object will eventually reference a `TransportNodeId`. After migration, no domain code references `Flag` directly.

## Future Node Types

`TransportNode` is not sealed. All types share the same contract:

| Type | Visual | Specialisation |
|------|--------|----------------|
| RoadFlag | Flag | Standard capacity, standard transfer |
| Warehouse | Building | Large capacity, accepts all resource types |
| Harbor | Building | Water transport routing |
| Market | Building | Cross-settlement trade interface |

All share: buffer, attachment, pendingDemand.
All differ: capacity limits, visual representation, additional routing rules.

## Boundaries

```
ProductionSystem
      │
      ▼
Building buffers (outputBuffer / inputDelivered)
      │
      ▼
LocalTransferSystem          ← owns allocation + deficit evaluation
      │
      ├── TransportNode.ReceiveExport()    (building → node)
      ├── TransportNode.TakeForBuilding()  (node → building)
      ├── Evaluates remaining deficit → pendingDemand[]
      │
      ▼
 TransportNode
      │
      ├── pendingDemand[]  →  DemandManager
      │                         │
      │                   TransportController
      │                         │
      │                      Carrier
      │                         │
      └── ReceiveCargo()  ←─────┘
```

## System Responsibility Summary

| System | Responsibility |
|--------|---------------|
| ResourceBuffer | Passive storage, no logic |
| TransportNode | Holds buffer + attachments, atomic ops, owns pendingDemand[] storage |
| LocalTransferSystem | Export, supply, deficit evaluation, surplus calculation |
| DemandManager | Published demand lifecycle — deduplication, task creation |
| TransportController | Task execution, carrier dispatch |
