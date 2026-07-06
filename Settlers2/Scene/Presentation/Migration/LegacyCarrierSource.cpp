#include "stdafx.h"
#include "LegacyCarrierSource.h"
#include "../../../World/CarrierManager.h"
#include "../../../World/Carrier.h"
#include "../../../World/Cargo.h"
#include "../../../World/Road.h"

namespace Scene {

LegacyCarrierSource::LegacyCarrierSource()
    : m_carrierManager(NULL)
{
}

void LegacyCarrierSource::SetCarrierManager(World::CarrierManager* carrierManager)
{
    m_carrierManager = carrierManager;
}

uint32_t LegacyCarrierSource::GetCarrierCount() const
{
    if (!m_carrierManager) return 0;
    return static_cast<uint32_t>(m_carrierManager->GetCarrierCount());
}

bool LegacyCarrierSource::GetCarrier(uint32_t index, CarrierView& out) const
{
    if (!m_carrierManager) return false;

    World::Carrier* carrier = m_carrierManager->GetCarrier(static_cast<int>(index));
    if (!carrier) return false;

    out.state = static_cast<uint8_t>(carrier->state);
    out.transitTiles = carrier->transitTiles;
    out.transitCount = carrier->transitCount;
    out.transitProgress = carrier->transitProgress;
    out.walkDir = carrier->walkDir;

    if (carrier->road) {
        out.roadTiles = carrier->road->tiles;
        out.roadTileCount = carrier->road->tileCount;
    } else {
        out.roadTiles = NULL;
        out.roadTileCount = 0;
    }
    out.roadEp = carrier->ep;

    out.cargoPresent = (carrier->m_cargo != NULL);
    out.cargoType = carrier->m_cargo ? static_cast<uint8_t>(carrier->m_cargo->type) : 0;

    return true;
}

} // namespace Scene
