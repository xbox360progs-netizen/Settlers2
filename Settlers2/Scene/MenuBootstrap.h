#pragma once

#include <string>
#include "../UI/GridMenu.h"
#include "../UI/UIMenu.h"
#include "../UI/UiMessageId.h"
#include "../World/Components/Building.h"

class Renderer;

namespace MenuBootstrap {

    bool SetupBuildMenu(GridMenu* buildMenu, Renderer* renderer);
    bool SetupRoadMenu(GridMenu* roadMenu, Renderer* renderer);
    bool SetupFlagMenu(UIMenu* flagMenu, UIMenu::ItemData* itemData, int maxItems, int& outItemCount);
    bool SetupGeologistMenu(UIMenu* geologistMenu);

    // Shared helper: map sprite/icon name to BuildingType
    World::BuildingType GetBuildingTypeFromSpriteName(const std::string& name);

    // UI5b: map sprite/icon name to UiMessageId for menu labels
    UI::UiMessageId GetBuildingMessageIdFromSpriteName(const std::string& name);

} // namespace MenuBootstrap
