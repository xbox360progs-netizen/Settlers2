#ifndef BUILDING_COMMAND_HANDLER_H
#define BUILDING_COMMAND_HANDLER_H

#include "../../Core/CommandBus.h"
#include "../../World/Components/Building.h"
#include "../../World/Map.h"
#include "../../World/FlagManager.h"
#include "../../World/ConstructionManager.h"

namespace Handlers {
    class BuildingCommandHandler : public Core::CommandListener {
    public:
        BuildingCommandHandler(
            World::Map* map,
            World::FlagManager* flagManager,
            World::ConstructionManager* constructionManager
        );

        virtual void OnCommand(Core::CommandType type, void* data);

    private:
        World::Map* m_map;
        World::FlagManager* m_flagManager;
        World::ConstructionManager* m_constructionManager;
    };
}

#endif // BUILDING_COMMAND_HANDLER_H
