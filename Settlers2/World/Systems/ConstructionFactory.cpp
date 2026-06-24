#include "stdafx.h"
#include "ConstructionFactory.h"
#include "../FlagManager.h"

namespace World {

ConstructionFactory::ConstructionFactory()
    : m_flagManager(NULL)
    , m_defaultEntranceDx(1)
    , m_defaultEntranceDy(0)
{
}

ConstructionFactory::ConstructionFactory(FlagManager* flagManager)
    : m_flagManager(flagManager)
    , m_defaultEntranceDx(1)
    , m_defaultEntranceDy(0)
{
}

void ConstructionFactory::SetFlagManager(FlagManager* flagManager)
{
    m_flagManager = flagManager;
}

void ConstructionFactory::SetDefaultEntranceOffset(int dx, int dy)
{
    m_defaultEntranceDx = dx;
    m_defaultEntranceDy = dy;
}

ConstructionSite* ConstructionFactory::Create(const BuildCommand& cmd)
{
    if (cmd.type == Building_None) return NULL;
    if (!m_flagManager) return NULL;

    // Determine the entrance flag
    Flag* flag = cmd.entranceFlag;
    if (!flag) {
        int entranceX = cmd.tileX + m_defaultEntranceDx;
        int entranceY = cmd.tileY + m_defaultEntranceDy;

        flag = m_flagManager->CreateFlag(entranceX, entranceY);
        if (!flag) return NULL;
        flag->type = FLAG_NORMAL;
        flag->pendingBuilding = cmd.type;
        flag->hasBuilding = true;
    }

    return new ConstructionSite(cmd.tileX, cmd.tileY, cmd.type, flag);
}

} // namespace World
