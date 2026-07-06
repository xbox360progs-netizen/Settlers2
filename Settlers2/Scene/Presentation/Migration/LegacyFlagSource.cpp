#include "stdafx.h"
#include "LegacyFlagSource.h"
#include "../../../World/FlagManager.h"
#include "../../../World/Flag.h"

namespace Scene {

void LegacyFlagSource::SetFlagManager(World::FlagManager* flagManager)
{
    m_flagManager = flagManager;
}

uint32_t LegacyFlagSource::GetFlagCount() const
{
    if (!m_flagManager) return 0;
    return static_cast<uint32_t>(m_flagManager->GetCount());
}

bool LegacyFlagSource::GetFlag(uint32_t index, FlagView& out) const
{
    if (!m_flagManager) return false;

    World::Flag* flag = m_flagManager->GetFlag(index);
    if (!flag) return false;

    out.nodeX = flag->pos.x;
    out.nodeY = flag->pos.y;

    out.slotCount = 0;
    for (int si = 0; si < FlagView::kMaxSlots; ++si) {
        if (flag->slots[si].type != World::ResourceType_None && flag->slots[si].amount > 0) {
            out.slotTypes[out.slotCount] = static_cast<uint8_t>(flag->slots[si].type);
            out.slotAmounts[out.slotCount] = flag->slots[si].amount;
            ++out.slotCount;
        }
    }
    return true;
}

} // namespace Scene
