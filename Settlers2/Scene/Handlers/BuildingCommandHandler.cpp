 #include "stdafx.h"
 #include "BuildingCommandHandler.h"

 namespace Handlers {
     BuildingCommandHandler::BuildingCommandHandler(
         World::Map* map,
         World::FlagManager* flagManager,
         World::ConstructionManager* constructionManager
     ) : m_map(map)
       , m_flagManager(flagManager)
       , m_constructionManager(constructionManager)
     {
         // Cmd_DeleteFlag handled by ObjectLifecycleManager
         // Cmd_RemoveConstructionSite handled by ConstructionSystem
     }

     void BuildingCommandHandler::OnCommand(Core::CommandType type, void* data)
     {
         // Reserved for future building-specific command handling
     }
 }
