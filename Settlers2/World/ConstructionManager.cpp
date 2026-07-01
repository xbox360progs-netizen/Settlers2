#include "stdafx.h"
#include "ConstructionManager.h"
#include "FlagManager.h"
#include "RoadManager.h"
#include "DemandManager.h"
#include "../Logic/EconomyManager.h"

namespace World {

    ConstructionManager::ConstructionManager()
        : m_flagManager(NULL), m_roadManager(NULL), m_warehouseFlag(NULL),
          m_demandManager(NULL), m_builderRoutesDirty(false),
          m_pendingHead(0), m_pendingTail(0), m_pendingCount(0)
    {
    }

    ConstructionManager::~ConstructionManager()
    {
        Clear();
    }

    void ConstructionManager::AddSite(ConstructionSite* site)
    {
        if (site) {
            m_sites.push_back(site);
            PushPendingConstruction((int)m_sites.size() - 1);
        }
    }

    void ConstructionManager::RemoveSite(ConstructionSite* site)
    {
        for (size_t i = 0; i < m_sites.size(); ++i) {
            if (m_sites[i] == site) {
                if (m_demandManager && site->flag) {
                    // Only clear construction demands — preserve any demands the
                    // finished building may have set on the same flag.
                    m_demandManager->ClearDemand(ResourceType_Wood, site->flag->handle);
                    m_demandManager->ClearDemand(ResourceType_Stone, site->flag->handle);
                }
                delete m_sites[i];
                m_sites.erase(m_sites.begin() + i);
                return;
            }
        }
    }

    void ConstructionManager::RemoveSiteAt(int x, int y)
    {
        for (size_t i = 0; i < m_sites.size(); ++i) {
            if (m_sites[i]->x == x && m_sites[i]->y == y) {
                if (m_demandManager && m_sites[i]->flag) {
                    m_demandManager->ClearDemand(ResourceType_Wood, m_sites[i]->flag->handle);
                    m_demandManager->ClearDemand(ResourceType_Stone, m_sites[i]->flag->handle);
                }
                m_sites[i]->flag = NULL;
                m_sites[i]->builderState = Builder_None;
                delete m_sites[i];
                m_sites.erase(m_sites.begin() + i);
                return;
            }
        }
    }

    int ConstructionManager::PopPendingConstruction()
    {
        if (m_pendingCount <= 0) return -1;
        int idx = m_pendingConstruction[m_pendingHead];
        m_pendingHead = (m_pendingHead + 1) % MAX_PENDING_CONSTRUCTION;
        --m_pendingCount;
        return idx;
    }

    void ConstructionManager::PushPendingConstruction(int siteIndex)
    {
        if (m_pendingCount >= MAX_PENDING_CONSTRUCTION) return;
        m_pendingConstruction[m_pendingTail] = siteIndex;
        m_pendingTail = (m_pendingTail + 1) % MAX_PENDING_CONSTRUCTION;
        ++m_pendingCount;
    }

    void ConstructionManager::InitBuilderFirstLeg(ConstructionSite* site)
    {
        if (!site || site->builderRouteCount < 2) return;

        Flag* fromFlag = site->builderRoute[0];
        Flag* toFlag = site->builderRoute[1];
        Road* road = m_roadManager ? m_roadManager->GetRoadBetween(fromFlag, toFlag) : NULL;
        if (road && road->tileCount >= 2) {
            float pathLen = (float)(road->tileCount - 1);
            if (fromFlag->pos.x == road->tiles[0].x && fromFlag->pos.y == road->tiles[0].y) {
                site->builderWalkDir = 1.0f;
                site->builderEp = 0.0f;
            } else {
                site->builderWalkDir = -1.0f;
                site->builderEp = pathLen;
            }
        } else {
            site->builderWalkDir = 1.0f;
            site->builderEp = 0.0f;
        }
    }

    void ConstructionManager::Update(float dt)
    {
        RecalculateBuilderRoutes();

        // Process pending construction queue: dispatch up to 2 builders per frame
        for (int p = 0; p < 2; ++p) {
            int pendingIdx = PopPendingConstruction();
            if (pendingIdx < 0 || pendingIdx >= (int)m_sites.size()) break;
            ConstructionSite* psite = m_sites[pendingIdx];
            if (psite && psite->builderState == Builder_None && !psite->IsComplete() && !psite->CanBuild()) {
                if (m_roadManager && m_warehouseFlag && psite->flag) {
                    std::vector<Flag*> _path = m_roadManager->FindFlagPath(m_warehouseFlag, psite->flag);
                    psite->builderRouteCount = (_path.size() < MAX_BUILDER_FLAGS) ? (uint32_t)_path.size() : MAX_BUILDER_FLAGS;
                    for (uint32_t _ri = 0; _ri < psite->builderRouteCount; ++_ri)
                        psite->builderRoute[_ri] = _path[_ri];
                    if (psite->builderRouteCount >= 2) {
                        psite->builderRouteIndex = 0;
                        InitBuilderFirstLeg(psite);
                        psite->builderState = Builder_Walking;
                    }
                }
            }
            // If site already has a builder or is complete, item is silently consumed
        }

        const float BUILDER_SPEED = 2.5f;

        for (size_t i = 0; i < m_sites.size(); ) {
            ConstructionSite* site = m_sites[i];
            if (!site->flag) { ++i; continue; }

            bool advanced = true;

            // Transfer resources from flag to construction site
            if (site->NeedsWood()) {
                for (int si = 0; si < 8; ++si) {
                    ResourceSlot& slot = site->flag->slots[si];
                    if (slot.type != ResourceType_Wood || slot.amount <= 0) continue;
                    if (slot.destFlagId != World::INVALID_FLAG_ID && slot.destFlagId != site->flag->id) continue;
                    int take = (slot.amount < site->woodNeeded - site->woodDelivered) ? slot.amount : (site->woodNeeded - site->woodDelivered);
                    if (take <= 0) continue;
                    slot.amount -= take;
                    site->woodDelivered += take;
                    { char dbg[256]; _snprintf(dbg, sizeof(dbg), "[Construction] Wood delivered flag=%u take=%d current=%d/%d\n", site->flag->id, take, site->woodDelivered, site->woodNeeded); OutputDebugStringA(dbg); }
                    if (slot.amount <= 0) {
                        slot.type = ResourceType_None;
                        slot.amount = 0;
                        slot.destFlagId = 0;
                    }
                    break;
                }
            }

            if (site->NeedsStone()) {
                for (int si = 0; si < 8; ++si) {
                    ResourceSlot& slot = site->flag->slots[si];
                    if (slot.type != ResourceType_Stone || slot.amount <= 0) continue;
                    if (slot.destFlagId != World::INVALID_FLAG_ID && slot.destFlagId != site->flag->id) continue;
                    int take = (slot.amount < site->stoneNeeded - site->stoneDelivered) ? slot.amount : (site->stoneNeeded - site->stoneDelivered);
                    if (take <= 0) continue;
                    slot.amount -= take;
                    site->stoneDelivered += take;
                    if (slot.amount <= 0) {
                        slot.type = ResourceType_None;
                        slot.amount = 0;
                        slot.destFlagId = 0;
                    }
                    break;
                }
            }

            // ── Builder dispatch ──
            if (site->builderState == Builder_None && !site->IsComplete()) {
                if (site->CanBuild()) {
                    char _dbuf[256];
                    size_t _dpos = _snprintf(_dbuf, sizeof(_dbuf),
                        "[Construction] Dispatch site=%p whF=%p sF=%p rM=%d",
                        (void*)site, (void*)m_warehouseFlag,
                        (void*)site->flag, m_roadManager ? 1 : 0);
                    if (!m_warehouseFlag) _dpos += _snprintf(_dbuf+_dpos, sizeof(_dbuf)-_dpos, " NO_WH");
                    if (!site->flag)      _dpos += _snprintf(_dbuf+_dpos, sizeof(_dbuf)-_dpos, " NO_FLAG");
                    if (!m_roadManager)   _dpos += _snprintf(_dbuf+_dpos, sizeof(_dbuf)-_dpos, " NO_RM");
                    _snprintf(_dbuf+_dpos, sizeof(_dbuf)-_dpos, "\n");
                    OutputDebugStringA(_dbuf);
                }
                if (m_roadManager && m_warehouseFlag && site->flag) {
                    {
                        std::vector<Flag*> _path = m_roadManager->FindFlagPath(m_warehouseFlag, site->flag);
                        site->builderRouteCount = (_path.size() < MAX_BUILDER_FLAGS) ? (uint32_t)_path.size() : MAX_BUILDER_FLAGS;
                        for (uint32_t _ri = 0; _ri < site->builderRouteCount; ++_ri)
                            site->builderRoute[_ri] = _path[_ri];
                    }
                    if (site->builderRouteCount >= 2) {
                        site->builderRouteIndex = 0;
                        InitBuilderFirstLeg(site);
                        site->builderState = Builder_Walking;

                        char _r_[512];
                        size_t _pos_ = _snprintf(_r_, sizeof(_r_),
                            "[Construction] Builder route: %u flags [", (unsigned)site->builderRouteCount);
                        for (uint32_t _i_ = 0; _i_ < site->builderRouteCount; ++_i_) {
                            _pos_ += _snprintf(_r_ + _pos_, sizeof(_r_) - _pos_, "%s#%u(%d,%d)",
                                _i_ > 0 ? " " : "",
                                site->builderRoute[_i_]->id,
                                site->builderRoute[_i_]->pos.x,
                                site->builderRoute[_i_]->pos.y);
                        }
                        _snprintf(_r_ + _pos_, sizeof(_r_) - _pos_, "]\n");
                        OutputDebugStringA(_r_);
                    } else {
                        OutputDebugStringA("[Construction] Builder: no road to site, waiting\n");
                    }
                }
            }

            // ── Builder walking / returning (ep-based movement along road tiles) ──
            if (site->builderState == Builder_Walking || site->builderState == Builder_Returning) {
                if (site->builderRouteIndex >= site->builderRouteCount - 1) {
                    // Arrived at final flag
                    if (site->builderState == Builder_Walking) {
                        site->builderState = Builder_Building;
                        site->buildProgress = 0.0f;
                        OutputDebugStringA("[Construction] Builder arrived at site, starting construction\n");
                    } else {
                        site->builderState = Builder_None;
                        OutputDebugStringA("[Construction] Builder returned to HQ\n");
                    }
                    advanced = false;
                }

                if (advanced) {
                    Flag* fromFlag = site->builderRoute[site->builderRouteIndex];
                    Flag* toFlag = site->builderRoute[site->builderRouteIndex + 1];
                    Road* road = m_roadManager->GetRoadBetween(fromFlag, toFlag);
                    if (!road || road->tileCount < 2) {
                        site->builderRouteIndex++;
                        advanced = false;
                    }

                    if (advanced) {
                        float pathLen = (float)(road->tileCount - 1);

                        if (fromFlag->pos.x == road->tiles[0].x && fromFlag->pos.y == road->tiles[0].y) {
                            site->builderWalkDir = 1.0f;
                        } else {
                            site->builderWalkDir = -1.0f;
                        }

                        site->builderEp += site->builderWalkDir * dt * BUILDER_SPEED;

                        bool arrivedAtFlag = false;
                        if (site->builderWalkDir > 0.0f && site->builderEp >= pathLen) {
                            arrivedAtFlag = true;
                        } else if (site->builderWalkDir < 0.0f && site->builderEp <= 0.0f) {
                            arrivedAtFlag = true;
                        }

                        if (arrivedAtFlag) {
                            site->builderRouteIndex++;
                            if (site->builderRouteIndex >= site->builderRouteCount - 1) {
                                if (site->builderState == Builder_Walking) {
                                    site->builderState = Builder_Building;
                                    site->buildProgress = 0.0f;
                                    OutputDebugStringA("[Construction] Builder arrived at site, starting construction\n");
                                } else {
                                    site->builderState = Builder_None;
                                    OutputDebugStringA("[Construction] Builder returned to HQ\n");
                                }
                            } else {
                                Flag* nextFrom = site->builderRoute[site->builderRouteIndex];
                                Flag* nextTo = site->builderRoute[site->builderRouteIndex + 1];
                                Road* nextRoad = m_roadManager->GetRoadBetween(nextFrom, nextTo);
                                if (nextRoad && nextRoad->tileCount >= 2) {
                                    float nextPathLen = (float)(nextRoad->tileCount - 1);
                                    if (nextFrom->pos.x == nextRoad->tiles[0].x && nextFrom->pos.y == nextRoad->tiles[0].y) {
                                        site->builderWalkDir = 1.0f;
                                        site->builderEp = 0.0f;
                                    } else {
                                        site->builderWalkDir = -1.0f;
                                        site->builderEp = nextPathLen;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ── Builder building (only if builder is at site and resources are available) ──
            if (site->builderState == Builder_Building && site->CanBuild()) {
                site->buildProgress += dt * 10.0f;
                if (site->buildProgress >= 100.0f) {
                    site->buildProgress = 100.0f;
                    OutputDebugStringA("[Construction] Building complete! Builder returning to HQ\n");

                    if (m_roadManager && m_warehouseFlag && site->flag) {
                        {
                            std::vector<Flag*> _path = m_roadManager->FindFlagPath(site->flag, m_warehouseFlag);
                            site->builderRouteCount = (_path.size() < MAX_BUILDER_FLAGS) ? (uint32_t)_path.size() : MAX_BUILDER_FLAGS;
                            for (uint32_t _ri = 0; _ri < site->builderRouteCount; ++_ri)
                                site->builderRoute[_ri] = _path[_ri];
                        }
                        {
                            char _r_[512];
                            size_t _pos_ = _snprintf(_r_, sizeof(_r_),
                                "[Construction] Return route: %u flags [", (unsigned)site->builderRouteCount);
                            for (uint32_t _i_ = 0; _i_ < site->builderRouteCount; ++_i_) {
                                _pos_ += _snprintf(_r_ + _pos_, sizeof(_r_) - _pos_, "%s#%u(%d,%d)",
                                    _i_ > 0 ? " " : "",
                                    site->builderRoute[_i_]->id,
                                    site->builderRoute[_i_]->pos.x,
                                    site->builderRoute[_i_]->pos.y);
                            }
                            _snprintf(_r_ + _pos_, sizeof(_r_) - _pos_, "]\n");
                            OutputDebugStringA(_r_);
                        }
                        if (site->builderRouteCount >= 2) {
                            site->builderRouteIndex = 0;
                            InitBuilderFirstLeg(site);
                            site->builderState = Builder_Returning;
                        } else {
                            site->builderState = Builder_None;
                            OutputDebugStringA("[Construction] No return path — builder vanishes\n");
                        }
                    } else {
                        site->builderState = Builder_None;
                    }
                }
            }

            if (site->IsComplete() && site->builderState == Builder_None) {
                // Defer removal — ConstructionSystem::PostUpdate detects completed sites
                // and fires Event_ConstructionComplete. BuildingSystem handles the event,
                // creates the building + worker, and posts Cmd_RemoveConstructionSite.
                ++i;
            } else {
                ++i;
            }
        }
    }

    void ConstructionManager::OnRoadRemoved(Road* road)
    {
        if (!road) return;
        for (size_t i = 0; i < m_sites.size(); ++i) {
            ConstructionSite* site = m_sites[i];
            if (site->builderState == Builder_None || site->builderState == Builder_Building) continue;
            if (site->builderRouteCount < 2) continue;

            Flag* ra = m_flagManager ? m_flagManager->ResolveFlag(road->a) : NULL;
            Flag* rb = m_flagManager ? m_flagManager->ResolveFlag(road->b) : NULL;
            for (uint32_t ri = 0; ri + 1 < site->builderRouteCount; ++ri) {
                Flag* a = site->builderRoute[ri];
                Flag* b = site->builderRoute[ri + 1];
                if ((a == ra && b == rb) || (a == rb && b == ra)) {
                    char buf[256];
                    _snprintf(buf, sizeof(buf),
                        "[Construction] Road %u in builder route #%u(%d,%d)->#%u(%d,%d) removed — "
                        "reset builder state\n",
                        road->id,
                        a->id, a->pos.x, a->pos.y,
                        b->id, b->pos.x, b->pos.y);
                    OutputDebugStringA(buf);
                    site->builderState = Builder_None;
                    site->builderRouteCount = 0;
                    break;
                }
            }
        }
    }

    ConstructionSite* ConstructionManager::GetSiteAt(int x, int y) const
    {
        for (size_t i = 0; i < m_sites.size(); ++i) {
            if (m_sites[i]->x == x && m_sites[i]->y == y) {
                return m_sites[i];
            }
        }
        return NULL;
    }

    ConstructionSite* ConstructionManager::GetSiteForFlag(const Flag* flag) const
    {
        for (size_t i = 0; i < m_sites.size(); ++i) {
            if (m_sites[i]->flag == flag) {
                return m_sites[i];
            }
        }
        return NULL;
    }

    Flag* ConstructionManager::FindConstructionDemand(Flag* fromFlag, ResourceType type) const
    {
        if (!fromFlag || !m_roadManager) return NULL;
        for (size_t i = 0; i < m_sites.size(); ++i) {
            ConstructionSite* site = m_sites[i];
            if (!site->flag || site->IsComplete()) continue;
            bool needsType = (type == ResourceType_Wood && site->NeedsWood()) ||
                             (type == ResourceType_Stone && site->NeedsStone());
            if (!needsType) continue;
            std::vector<Flag*> path = m_roadManager->FindFlagPath(fromFlag, site->flag);
            if (!path.empty())
                return site->flag;
        }
        return NULL;
    }

    void ConstructionManager::GenerateRequests(Logic::EconomyManager* economy)
    {
        {
            char dbg[256];
            _snprintf(dbg, sizeof(dbg),
                "[CONSTR_GR] sites=%u economy=%p\n",
                (unsigned)m_sites.size(), economy);
            OutputDebugStringA(dbg);
        }
        if (!economy) return;

        for (size_t i = 0; i < m_sites.size(); ++i) {
            ConstructionSite* site = m_sites[i];
            if (!site->flag) continue;
            if (site->IsComplete()) {
                if (m_demandManager) {
                    m_demandManager->ClearDemand(ResourceType_Wood, site->flag->handle);
                    m_demandManager->ClearDemand(ResourceType_Stone, site->flag->handle);
                }
                continue;
            }

            // DEBUG: verify site->flag identity
            {
                char dbg[512];
                _snprintf(dbg, sizeof(dbg),
                    "[CONSTR DEBUG] site=%p flag=%p id=%u handleIdx=%u pos=(%d,%d) buildingType=%d\n",
                    site, site->flag, site->flag->id,
                    site->flag->handle.index,
                    site->flag->pos.x, site->flag->pos.y,
                    (int)site->buildingType);
                OutputDebugStringA(dbg);
            }

            int woodMissing = site->WoodMissing();
            int stoneMissing = site->StoneMissing();

            if (woodMissing > 0) {
                economy->RequestConstructionResource(site->flag, ResourceType_Wood, woodMissing, 50);
                if (m_demandManager && woodMissing != site->woodRequested) {
                    m_demandManager->SetDemand(ResourceType_Wood, (uint32_t)woodMissing,
                        site->flag->handle, 100);
                }
            } else {
                if (m_demandManager && site->woodRequested != 0) {
                    m_demandManager->ClearDemand(ResourceType_Wood, site->flag->handle);
                }
            }
            site->woodRequested = (woodMissing > 0) ? woodMissing : 0;

            if (stoneMissing > 0) {
                economy->RequestConstructionResource(site->flag, ResourceType_Stone, stoneMissing, 50);
                if (m_demandManager && stoneMissing != site->stoneRequested) {
                    m_demandManager->SetDemand(ResourceType_Stone, (uint32_t)stoneMissing,
                        site->flag->handle, 100);
                }
            } else {
                if (m_demandManager && site->stoneRequested != 0) {
                    m_demandManager->ClearDemand(ResourceType_Stone, site->flag->handle);
                }
            }
            site->stoneRequested = (stoneMissing > 0) ? stoneMissing : 0;
        }
    }

    void ConstructionManager::RecalculateBuilderRoutes()
    {
        if (!m_builderRoutesDirty) return;
        if (!m_roadManager || !m_warehouseFlag) { m_builderRoutesDirty = false; return; }
        m_builderRoutesDirty = false;

        for (size_t i = 0; i < m_sites.size(); ++i) {
            ConstructionSite* site = m_sites[i];
            if (site->builderState == Builder_None) continue;
            if (!site->flag) continue;

            // Only recalculate if the current route no longer works
            bool needsRecalc = false;
            for (uint32_t ri = 0; ri + 1 < site->builderRouteCount; ++ri) {
                Flag* a = site->builderRoute[ri];
                Flag* b = site->builderRoute[ri + 1];
                if (!m_roadManager->GetRoadBetween(a, b)) {
                    needsRecalc = true;
                    break;
                }
            }
            if (!needsRecalc) continue;

            // Recalculate route from current position
            std::vector<Flag*> newRoute;
            if (site->builderState == Builder_Walking) {
                newRoute = m_roadManager->FindFlagPath(m_warehouseFlag, site->flag);
            } else if (site->builderState == Builder_Returning) {
                newRoute = m_roadManager->FindFlagPath(site->flag, m_warehouseFlag);
            } else {
                continue;
            }

            if (newRoute.size() < 2) {
                // No route — reset builder to retry later
                site->builderState = Builder_None;
                site->builderRouteCount = 0;
                continue;
            }

            // Preserve current position in new route
            Flag* currentFlag = (site->builderRouteIndex < site->builderRouteCount)
                ? site->builderRoute[site->builderRouteIndex] : NULL;

            site->builderRouteCount = (newRoute.size() < MAX_BUILDER_FLAGS) ? (uint32_t)newRoute.size() : MAX_BUILDER_FLAGS;
            for (uint32_t _ri = 0; _ri < site->builderRouteCount; ++_ri)
                site->builderRoute[_ri] = newRoute[_ri];

            uint32_t newIndex = 0;
            if (currentFlag) {
                for (uint32_t ri = 0; ri < newRoute.size(); ++ri) {
                    if (newRoute[ri] == currentFlag) {
                        newIndex = ri;
                        break;
                    }
                }
            }
            site->builderRouteIndex = newIndex;
            if (site->builderRouteIndex >= site->builderRouteCount - 1) {
                if (site->builderState == Builder_Walking) {
                    site->builderState = Builder_Building;
                    site->buildProgress = 0.0f;
                } else {
                    site->builderState = Builder_None;
                }
            } else {
                InitBuilderFirstLeg(site);
            }

            char buf[256];
            _snprintf(buf, sizeof(buf),
                "[Construction] Builder route recalculated: %u flags, index=%u\n",
                (unsigned)newRoute.size(), newIndex);
            OutputDebugStringA(buf);
        }
    }

    void ConstructionManager::Clear()
    {
        for (size_t i = 0; i < m_sites.size(); ++i) {
            delete m_sites[i];
        }
        m_sites.clear();
    }

}
