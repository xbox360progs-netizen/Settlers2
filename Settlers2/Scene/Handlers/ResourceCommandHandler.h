#ifndef RESOURCE_COMMAND_HANDLER_H
  #define RESOURCE_COMMAND_HANDLER_H

  #include "../../Core/CommandBus.h"
  #include "../../World/Map.h"

  namespace Handlers {
      class ResourceCommandHandler : public Core::CommandListener {
      public:
          ResourceCommandHandler(World::Map* map);

          virtual void OnCommand(Core::CommandType type, void* data);

          //  ( SimulationSystem)
          void RemoveGroundResource(int index);

      private:
          World::Map* m_map;
      };
  }

  #endif // RESOURCE_COMMAND_HANDLER_H