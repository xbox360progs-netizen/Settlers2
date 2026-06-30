#ifndef COMMAND_HANDLERS_H
  #define COMMAND_HANDLERS_H

  namespace Handlers {
      class BuildingCommandHandler;
      class ResourceCommandHandler;

      // Структура для хранения всех обработчиков
      struct CommandHandlers {
          BuildingCommandHandler* buildingHandler;
          ResourceCommandHandler* resourceHandler;
      };
  }

  #endif // COMMAND_HANDLERS_H