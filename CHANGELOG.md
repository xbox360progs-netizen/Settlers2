# Changelog

## 2026-06-30 — Logistics Architecture Migration (PR 1+2+3)

### Architecture
- **Transport pipeline refactored**: Carrier is now execution-only; route planning is centralized in DemandManager. Three invariants documented as the Transport Contract (Planning, Ticket, Ownership).
- **DemandTicket allocation**: migrated from heap to a fixed-size pool (256 slots), matching CargoManager/TransportJobManager memory model.
- **ResourceSlot::destFlagId semantics clarified**: proven to be an ownership tag for stationary resources, not a routing mechanism. It protects ResourceSlots from cross-subsystem consumption before a Carrier converts them to Cargo.
- **Architectural boundary formalized**: `ResourceSlot (destFlagId ownership)` → `TakeCargoForRoad()` → `Cargo (DemandTicket routing)`. These responsibilities must never overlap.

### Removed
- `Building::state` — replaced by invariant: Building always represents a completed, functional building.
- `ConstructionResourceDelivered` dead registration — event never posted.
- `ResourceDeliveredData::destFlagId` — dead field.
- `DemandManager::GetDemandTarget()` — 0 callers after Carrier migration.
- Dead confirm system (~100 lines in GameRenderer, IUiInputHost interface).

### UI
- MenuScene refactored to MenuModel + UiAction + ICommandDispatcher; zero string literals.
- GridMenu (BuildMenu) migrated to MenuModel/UiAction contract.
- All 6 UiEventSystem handlers use NotificationManager instead of legacy pool.
