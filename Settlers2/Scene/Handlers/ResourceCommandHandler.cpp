 #include "stdafx.h"
 #include "ResourceCommandHandler.h"
 #include "../../Core/CommandBus.h"
 #include "../../World/Map.h"
 #include <stdio.h>

 namespace Handlers {
     ResourceCommandHandler::ResourceCommandHandler(World::Map* map)
         : m_map(map)
     {
     }

     void ResourceCommandHandler::OnCommand(Core::CommandType type, void* data)
     {
         // TODO: handle ground resource commands
     }

     void ResourceCommandHandler::RemoveGroundResource(int index)
     {
         if (m_map) {
             m_map->RemoveGroundResource(index);
         }
     }
 }