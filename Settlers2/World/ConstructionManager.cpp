#include "stdafx.h"
#include "ConstructionManager.h"
#include "../Logic/EconomyManager.h"

namespace World {

    ConstructionManager::ConstructionManager()
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
        }
    }

    void ConstructionManager::RemoveSite(ConstructionSite* site)
    {
        for (size_t i = 0; i < m_sites.size(); ++i) {
            if (m_sites[i] == site) {
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
                delete m_sites[i];
                m_sites.erase(m_sites.begin() + i);
                return;
            }
        }
    }

    void ConstructionManager::Update(float dt)
    {
        for (size_t i = 0; i < m_sites.size(); ++i) {
            ConstructionSite* site = m_sites[i];
            if (!site->flag) continue;

            // Transfer resources from flag to construction site
            if (site->NeedsWood()) {
                int available = site->flag->GetAvailable(ResourceType_Wood);
                if (available > 0) {
                    int take = site->woodNeeded - site->woodDelivered;
                    if (take > available) take = available;
                    site->flag->RemoveResource(ResourceType_Wood, take);
                    site->woodDelivered += take;
                    if (site->woodRequested > take)
                        site->woodRequested -= take;
                    else
                        site->woodRequested = 0;
                }
            }

            if (site->NeedsStone()) {
                int available = site->flag->GetAvailable(ResourceType_Stone);
                if (available > 0) {
                    int take = site->stoneNeeded - site->stoneDelivered;
                    if (take > available) take = available;
                    site->flag->RemoveResource(ResourceType_Stone, take);
                    site->stoneDelivered += take;
                    if (site->stoneRequested > take)
                        site->stoneRequested -= take;
                    else
                        site->stoneRequested = 0;
                }
            }

            // If all resources delivered, advance progress
            if (site->CanBuild() && !site->IsComplete()) {
                site->progress += dt * 10.0f; // ~10 seconds to build
                if (site->progress > 100.0f) {
                    site->progress = 100.0f;
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

    void ConstructionManager::GenerateRequests(Logic::EconomyManager* economy)
    {
        if (!economy) return;

        for (size_t i = 0; i < m_sites.size(); ++i) {
            ConstructionSite* site = m_sites[i];
            if (!site->flag) continue;
            if (site->IsComplete()) continue;

            int missing;
            if (site->NeedsWood() && (missing = site->WoodMissing()) > 0) {
                economy->RequestConstructionResource(site->flag, ResourceType_Wood, missing, 50);
                site->woodRequested += missing;
            }

            if (site->NeedsStone() && (missing = site->StoneMissing()) > 0) {
                economy->RequestConstructionResource(site->flag, ResourceType_Stone, missing, 50);
                site->stoneRequested += missing;
            }
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
