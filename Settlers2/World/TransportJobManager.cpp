#include "stdafx.h"
#include <cassert>
#include "TransportJobManager.h"
#include "FlagManager.h"
#include "RoadManager.h"
#include "CarrierManager.h"
#include "Flag.h"
#include "Road.h"
#include "Carrier.h"
#include "Warehouse.h"

namespace World {

    TransportJobManager::TransportJobManager()
        : m_flagManager(NULL), m_roadManager(NULL), m_carrierManager(NULL), m_warehouse(NULL), m_nextJobId(1),
          m_routesDirty(false), m_recalculatingRoutes(false)
    {
    }

    TransportJobManager::~TransportJobManager()
    {
        for (size_t i = 0; i < m_jobs.size(); ++i)
            delete m_jobs[i];
        m_jobs.clear();
    }

    TransportJob* TransportJobManager::CreateJob(ResourceType resource, uint32_t amount,
                                                  Flag* source, Flag* destination)
    {
        if (!source || !destination || !m_roadManager) return NULL;
        if (source == destination) return NULL;

        std::vector<Flag*> route = m_roadManager->FindFlagPath(source, destination);
        if (route.size() < 2) {
            char buf[256];
            _snprintf(buf, sizeof(buf),
                "[Cargo] Job FAILED: %s src=%u->%u no_route\n",
                ResourceTypeToString(resource), source->id, destination ? destination->id : 0);
            OutputDebugStringA(buf);
            return NULL;
        }

        TransportJob* job = new TransportJob();
        job->id = m_nextJobId++;
        job->resource = resource;
        job->amount = amount;
        job->cargoId = 0;
        job->sourceFlag = m_flagManager ? m_flagManager->GetFlagHandle(source) : FlagHandle();
        job->destinationFlag = m_flagManager ? m_flagManager->GetFlagHandle(destination) : FlagHandle();
        for (size_t i = 0; i < route.size(); ++i)
            job->route.push_back(m_flagManager ? m_flagManager->GetFlagHandle(route[i]) : FlagHandle());
        job->currentLeg = 0;
        job->state = TransportJob::Waiting;

        m_jobs.push_back(job);

        EnsureInTransitSize(source->id, destination->id);
        m_inTransitCount[source->id][destination->id].count[resource] += amount;

        char buf[256];
        _snprintf(buf, sizeof(buf),
            "[Cargo] Job#%u CREATED: %s src=Flag%u(%d,%d) dst=Flag%u(%d,%d) route=%u flags\n",
            job->id, ResourceTypeToString(resource),
            source->id, source->pos.x, source->pos.y,
            destination->id, destination->pos.x, destination->pos.y,
            (unsigned)route.size());
        OutputDebugStringA(buf);

        return job;
    }

    void TransportJobManager::CancelJob(TransportJob* job)
    {
        if (!job || job->state == TransportJob::Delivered || job->state == TransportJob::Cancelled)
            return;
        job->state = TransportJob::Cancelled;
        if (job->assignedCarrier.IsValid()) {
            Carrier* ac = m_carrierManager ? m_carrierManager->ResolveCarrier(job->assignedCarrier) : NULL;
            if (ac) ac->ClearJob();
            job->assignedCarrier = Handle<Carrier>();
        }
        Flag* srcFlag = m_flagManager ? m_flagManager->ResolveFlag(job->sourceFlag) : NULL;
        Flag* dstFlag = m_flagManager ? m_flagManager->ResolveFlag(job->destinationFlag) : NULL;
        if (srcFlag && dstFlag) {
            EnsureInTransitSize(srcFlag->id, dstFlag->id);
            m_inTransitCount[srcFlag->id][dstFlag->id].count[job->resource] -= job->amount;
        }
    }

    void TransportJobManager::CancelJobsForFlag(Flag* flag)
    {
        if (!flag) return;
        for (size_t i = 0; i < m_jobs.size(); ++i) {
            TransportJob* job = m_jobs[i];
            if (job->state == TransportJob::Delivered || job->state == TransportJob::Cancelled)
                continue;

            bool touchesFlag = false;
            Flag* srcFlag = m_flagManager ? m_flagManager->ResolveFlag(job->sourceFlag) : NULL;
            Flag* dstFlag = m_flagManager ? m_flagManager->ResolveFlag(job->destinationFlag) : NULL;
            touchesFlag = (srcFlag == flag || dstFlag == flag);
            if (!touchesFlag) {
                for (size_t ri = 0; ri < job->route.size(); ++ri) {
                    Flag* rf = m_flagManager ? m_flagManager->ResolveFlag(job->route[ri]) : NULL;
                    if (rf == flag) { touchesFlag = true; break; }
                }
            }
            if (touchesFlag)
                CancelJob(job);
        }
    }

    void TransportJobManager::Update()
    {
        // Invariant checks
        for (size_t ci = 0; ci < m_jobs.size(); ++ci) {
            TransportJob* j = m_jobs[ci];
            if (j->state == TransportJob::Assigned)
                assert(j->assignedCarrier.IsValid());
            if (j->state == TransportJob::Waiting || j->state == TransportJob::Delivered)
                assert(!j->assignedCarrier.IsValid());
        }

        for (size_t i = 0; i < m_jobs.size(); ++i) {
            TransportJob* job = m_jobs[i];
            if (job->state != TransportJob::Waiting) continue;

            uint32_t leg = job->currentLeg;
            if (leg + 1 >= job->route.size()) {
                job->state = TransportJob::Delivered;
                continue;
            }

            Flag* from = m_flagManager ? m_flagManager->ResolveFlag(job->route[leg]) : NULL;
            Flag* to = m_flagManager ? m_flagManager->ResolveFlag(job->route[leg + 1]) : NULL;
            if (!from || !to) {
                CancelJob(job);
                continue;
            }
            Road* road = m_roadManager ? m_roadManager->GetRoadBetween(from, to) : NULL;
            if (!road) continue;

            Carrier* carrier = m_carrierManager ? m_carrierManager->GetCarrierForRoad(road) : NULL;
            if (!carrier) {
                m_carrierManager->CreateCarrier(road);
                carrier = m_carrierManager->GetCarrierForRoad(road);
            }
            if (!carrier || carrier->HasJob()) continue;

            carrier->m_resolvedSourceFlag = from;
            carrier->m_resolvedDestFlag = m_flagManager ? m_flagManager->ResolveFlag(job->destinationFlag) : NULL;
            if (carrier->AssignJob(job, from)) {
                job->assignedCarrier = m_carrierManager ? m_carrierManager->GetCarrierHandle(carrier) : Handle<Carrier>();
                job->state = TransportJob::Assigned;
                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[Cargo] Job#%u ASSIGNED: from=Flag%u(%d,%d) to=Flag%u(%d,%d) leg=%u carrier=%p\n",
                    job->id, from->id, from->pos.x, from->pos.y,
                    to->id, to->pos.x, to->pos.y, leg, (void*)carrier);
                OutputDebugStringA(buf);
            }
        }
    }

    void TransportJobManager::OnLegDelivered(TransportJob* job)
    {
        if (!job) return;

        job->assignedCarrier = Handle<Carrier>();
        uint32_t prevLeg = job->currentLeg;
        job->currentLeg++;

        bool isFinal = (job->currentLeg >= job->route.size() - 1);
        if (isFinal) {
            job->state = TransportJob::Delivered;
            Flag* srcFlag = m_flagManager ? m_flagManager->ResolveFlag(job->sourceFlag) : NULL;
            Flag* dstFlag = m_flagManager ? m_flagManager->ResolveFlag(job->destinationFlag) : NULL;
            if (srcFlag && dstFlag) {
                EnsureInTransitSize(srcFlag->id, dstFlag->id);
                m_inTransitCount[srcFlag->id][dstFlag->id].count[job->resource] -= job->amount;
            }
        } else {
            job->state = TransportJob::Waiting;
        }

        Flag* destFlag = m_flagManager ? m_flagManager->ResolveFlag(job->route[prevLeg + 1]) : NULL;
        char buf[256];
        _snprintf(buf, sizeof(buf),
            "[Cargo] Job#%u LEG_%s: leg=%u/%u at=Flag%u(%d,%d) %s\n",
            job->id, isFinal ? "FINAL" : "DONE",
            prevLeg, (unsigned)(job->route.size() - 2),
            destFlag ? destFlag->id : 0,
            destFlag ? destFlag->pos.x : 0,
            destFlag ? destFlag->pos.y : 0,
            isFinal ? "DELIVERED" : "waiting for next leg");
        OutputDebugStringA(buf);
    }

    TransportJob* TransportJobManager::FindJobForRoad(Road* road) const
    {
        for (size_t i = 0; i < m_jobs.size(); ++i) {
            TransportJob* job = m_jobs[i];
            if (job->state != TransportJob::Assigned && job->state != TransportJob::InTransit)
                continue;
            uint32_t leg = job->currentLeg;
            if (leg + 1 >= job->route.size()) continue;
            Flag* from = m_flagManager ? m_flagManager->ResolveFlag(job->route[leg]) : NULL;
            Flag* to = m_flagManager ? m_flagManager->ResolveFlag(job->route[leg + 1]) : NULL;
            if (!from || !to) continue;
            if (!m_roadManager) continue;
            Road* r = m_roadManager->GetRoadBetween(from, to);
            if (r == road) return job;
        }
        return NULL;
    }

    TransportJob* TransportJobManager::GetJob(uint32_t id) const
    {
        for (size_t i = 0; i < m_jobs.size(); ++i) {
            if (m_jobs[i]->id == id) return m_jobs[i];
        }
        return NULL;
    }

    bool TransportJobManager::IsRoadInUse(Road* road) const {
        if (!road) return false;
        Flag* ra = m_flagManager ? m_flagManager->ResolveFlag(road->a) : NULL;
        Flag* rb = m_flagManager ? m_flagManager->ResolveFlag(road->b) : NULL;
        if (!ra || !rb) return false;
        for (size_t i = 0; i < m_jobs.size(); ++i) {
            TransportJob* job = m_jobs[i];
            if (job->state == TransportJob::Delivered || job->state == TransportJob::Cancelled)
                continue;
            for (size_t j = 0; j + 1 < job->route.size(); ++j) {
                Flag* f1 = m_flagManager ? m_flagManager->ResolveFlag(job->route[j]) : NULL;
                Flag* f2 = m_flagManager ? m_flagManager->ResolveFlag(job->route[j + 1]) : NULL;
                if (!f1 || !f2) continue;
                if ((f1 == ra && f2 == rb) || (f1 == rb && f2 == ra))
                    return true;
            }
        }
        return false;
    }

    bool TransportJobManager::IsFlagInUse(Flag* flag) const {
        if (!flag) return false;
        for (size_t i = 0; i < m_jobs.size(); ++i) {
            TransportJob* job = m_jobs[i];
            if (job->state == TransportJob::Delivered || job->state == TransportJob::Cancelled)
                continue;
            for (size_t j = 0; j < job->route.size(); ++j) {
                Flag* rf = m_flagManager ? m_flagManager->ResolveFlag(job->route[j]) : NULL;
                if (rf == flag)
                    return true;
            }
        }
        return false;
    }

    void TransportJobManager::EnsureInTransitSize(uint32_t srcFlagId, uint32_t destFlagId)
    {
        if (srcFlagId >= (uint32_t)m_inTransitCount.size())
            m_inTransitCount.resize(srcFlagId + 1);
        if (destFlagId >= (uint32_t)m_inTransitCount[srcFlagId].size())
            m_inTransitCount[srcFlagId].resize(destFlagId + 1);
    }

    void TransportJobManager::FlushRecalculate()
    {
        if (m_routesDirty && !m_recalculatingRoutes) {
            RecalculateRoutes();
            m_routesDirty = false;
        }
    }

    void TransportJobManager::RecalculateRoutes()
    {
        if (m_recalculatingRoutes) return;
        if (!m_roadManager) return;
        m_recalculatingRoutes = true;
        for (size_t i = 0; i < m_jobs.size(); ++i) {
            TransportJob* job = m_jobs[i];
            if (job->state == TransportJob::Delivered || job->state == TransportJob::Cancelled)
                continue;

            Flag* currentFlag = (job->currentLeg < (uint32_t)job->route.size())
                ? (m_flagManager ? m_flagManager->ResolveFlag(job->route[job->currentLeg]) : NULL)
                : (m_flagManager ? m_flagManager->ResolveFlag(job->sourceFlag) : NULL);

            Flag* srcFlag = m_flagManager ? m_flagManager->ResolveFlag(job->sourceFlag) : NULL;
            Flag* dstFlag = m_flagManager ? m_flagManager->ResolveFlag(job->destinationFlag) : NULL;
            if (!srcFlag || !dstFlag) {
                CancelJob(job);
                continue;
            }

            std::vector<Flag*> newRoute = m_roadManager->FindFlagPath(srcFlag, dstFlag);
            if (newRoute.size() < 2) {
                CancelJob(job);
                continue;
            }

            job->route.clear();
            for (size_t ri = 0; ri < newRoute.size(); ++ri)
                job->route.push_back(m_flagManager ? m_flagManager->GetFlagHandle(newRoute[ri]) : FlagHandle());

            uint32_t newLeg = 0;
            for (uint32_t ri = 0; ri < newRoute.size(); ++ri) {
                if (newRoute[ri] == currentFlag) {
                    newLeg = ri;
                    break;
                }
            }

            job->currentLeg = newLeg;
            if (job->currentLeg >= job->route.size() - 1) {
                job->state = TransportJob::Delivered;
            }

            char buf[256];
            _snprintf(buf, sizeof(buf),
                "[Cargo] Job#%u ROUTE_RECALC: legs=%u currentLeg=%u\n",
                job->id, (unsigned)newRoute.size(), job->currentLeg);
            OutputDebugStringA(buf);
        }
        m_recalculatingRoutes = false;
    }

    void TransportJobManager::ScanFlagsForCargo(FlagManager* flagManager)
    {
        if (!flagManager) return;
        for (size_t fi = 0; fi < flagManager->GetCount(); ++fi) {
            Flag* flag = flagManager->GetFlag(fi);
            if (!flag) continue;
            for (int si = 0; si < 8; ++si) {
                ResourceSlot& slot = flag->slots[si];
                if (slot.type == ResourceType_None || slot.amount <= 0) continue;

                char dbg[256];
                _snprintf(dbg, sizeof(dbg),
                    "[Scan] flag=%u slot=%d type=%d amount=%d destFlagId=%u reserved=%d\n",
                    flag->id, si, slot.type, slot.amount, slot.destFlagId, slot.reserved);
                OutputDebugStringA(dbg);

                if (slot.destFlagId == 0 || slot.destFlagId == World::INVALID_FLAG_ID) {
                    // Resources with destFlagId == 0 or INVALID_FLAG_ID are meant for warehouse pickup
                    // Create transport job to warehouse if warehouse exists
                    if (m_warehouse && m_warehouse->connectedFlag) {
                        Flag* whFlag = m_warehouse->connectedFlag;
                        if (whFlag && whFlag != flag && whFlag->id != 0) {
                            if (flag->Reserve(slot.type, 1, whFlag->id)) {
                                TransportJob* job = CreateJob(slot.type, 1, flag, whFlag);
                                if (job) {
                                    job->cargoId = (uint32_t)(fi * 8 + si);
                                } else {
                                    flag->Unreserve(slot.type, 1, whFlag->id);
                                }
                            }
                        }
                    }
                    char dbg[256];
                    _snprintf(dbg, sizeof(dbg),
                        "[Cargo] SKIP flag=%u slot=%d %s amount=%d: no destination (destFlagId=%u)\n",
                        flag->id, si, ResourceTypeToString(slot.type), slot.amount, slot.destFlagId);
                    OutputDebugStringA(dbg);
                    continue;
                }
                if (slot.destFlagId == flag->id) {
                    char dbg[256];
                    _snprintf(dbg, sizeof(dbg),
                        "[Cargo] SKIP flag=%u slot=%d %s amount=%d: already at destination\n",
                        flag->id, si, ResourceTypeToString(slot.type), slot.amount);
                    OutputDebugStringA(dbg);
                    continue;
                }
                if (slot.amount - slot.reserved <= 0) continue;

                // Count units already tracked by jobs from this source flag to this dest (O(1) cache)
                EnsureInTransitSize(flag->id, slot.destFlagId);
                uint32_t trackedForSource = m_inTransitCount[flag->id][slot.destFlagId].count[slot.type];

                // Skip cargo that's on an intermediate flag of an existing job
                bool inTransit = false;
                TransportJob* transitJob = NULL;
                for (size_t ji = 0; ji < m_jobs.size(); ++ji) {
                    TransportJob* j = m_jobs[ji];
                    if (j->state == TransportJob::Delivered || j->state == TransportJob::Cancelled)
                        continue;
                    Flag* jDstFlag = m_flagManager ? m_flagManager->ResolveFlag(j->destinationFlag) : NULL;
                    if (!jDstFlag) continue;
                    if (j->resource != slot.type || jDstFlag->id != slot.destFlagId)
                        continue;
                    Flag* jSrcFlag = m_flagManager ? m_flagManager->ResolveFlag(j->sourceFlag) : NULL;
                    for (uint32_t ri = j->currentLeg; ri < j->route.size(); ++ri) {
                        Flag* rf = m_flagManager ? m_flagManager->ResolveFlag(j->route[ri]) : NULL;
                        if (rf == flag && rf != jSrcFlag) {
                            inTransit = true;
                            transitJob = j;
                            break;
                        }
                    }
                    if (inTransit) break;
                }
                if (inTransit) {
                    char buf[256];
                    _snprintf(buf, sizeof(buf),
                        "[Cargo] SKIP flag=%u %s inTransit job#%u leg=%u/%u route=[",
                        flag->id, ResourceTypeToString(slot.type),
                        transitJob->id, transitJob->currentLeg, (unsigned)(transitJob->route.size() - 1));
                    for (size_t ri = 0; ri < transitJob->route.size(); ++ri) {
                        Flag* rf = m_flagManager ? m_flagManager->ResolveFlag(transitJob->route[ri]) : NULL;
                        size_t pos = strlen(buf);
                        _snprintf(buf + pos, sizeof(buf) - pos, "%s%u", ri > 0 ? "," : "", rf ? rf->id : 0);
                    }
                    size_t pos2 = strlen(buf);
                    _snprintf(buf + pos2, sizeof(buf) - pos2, "]\n");
                    OutputDebugStringA(buf);
                }
                if (inTransit) continue;

                if (trackedForSource >= (uint32_t)slot.amount) continue;

                Flag* destFlag = flagManager->GetFlagById(slot.destFlagId);
                if (!destFlag) continue;

                if (flag->Reserve(slot.type, 1, slot.destFlagId)) {
                    TransportJob* job = CreateJob(slot.type, 1, flag, destFlag);
                    if (job) {
                        job->cargoId = (uint32_t)(fi * 8 + si);
                    } else {
                        flag->Unreserve(slot.type, 1, slot.destFlagId);
                    }
                }
            }
        }
    }
}
