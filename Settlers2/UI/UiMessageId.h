#pragma once

namespace UI {

    enum UiMessageId {
        MSG_NONE = 0,

        // Geologist status text
        MSG_GEOLOGIST_CONFIRM,         // "Geologist: A=yes  B=no"
        MSG_GEOLOGIST_WORKING,         // "Geologist working..."
        MSG_GEOLOGIST_WORKING_SEC,     // "Geologist working... %d sec"
        MSG_GEOLOGIST_ALREADY,         // "Geologist already surveying"
        MSG_GEOLOGIST_CANCELLED,       // "Survey cancelled"

        // Geologist notification title
        MSG_GEOLOGIST_REPORT,          // "GEOLOGIST REPORT"

        // Geologist survey results (formatted with units count)
        MSG_GEOLOGIST_COAL_FOUND,      // "Coal vein found! Units: %d"
        MSG_GEOLOGIST_IRON_FOUND,      // "Iron ore found! Units: %d"
        MSG_GEOLOGIST_GOLD_FOUND,      // "Gold vein found! Units: %d"
        MSG_GEOLOGIST_STONE_FOUND,     // "Stone deposit found! Units: %d"
        MSG_GEOLOGIST_MARBLE_FOUND,    // "Marble deposit found! Units: %d"
        MSG_GEOLOGIST_GRANITE_FOUND,   // "Granite deposit found! Units: %d"
        MSG_GEOLOGIST_BRONZE_FOUND,    // "Bronze ore vein found! Units: %d"
        MSG_GEOLOGIST_UNKNOWN_FOUND,   // "Minerals detected"
        MSG_GEOLOGIST_BARREN_FOUND,    // "Barren rock — No minerals found"

        // Resource names (for HUD and general use)
        MSG_RESOURCE_WOOD,
        MSG_RESOURCE_PLANKS,
        MSG_RESOURCE_STONE,
        MSG_RESOURCE_FISH,
        MSG_RESOURCE_MEAT,
        MSG_RESOURCE_BREAD,
        MSG_RESOURCE_COAL,
        MSG_RESOURCE_IRONORE,
        MSG_RESOURCE_GOLDORE,
        MSG_RESOURCE_IRONBAR,
        MSG_RESOURCE_GOLDBAR,
        MSG_RESOURCE_BRONZEORE,
        MSG_RESOURCE_MARBLE,
        MSG_RESOURCE_GRANITE,
        MSG_RESOURCE_WHEAT,
        MSG_RESOURCE_FLOUR,
        MSG_RESOURCE_WATER,
        MSG_RESOURCE_TOOLS,
        MSG_RESOURCE_TRAP,
        MSG_RESOURCE_FIELD,
        MSG_RESOURCE_REALWOOD,
        MSG_RESOURCE_EXOTICWOOD,
        MSG_RESOURCE_TITANIUM,
        MSG_RESOURCE_SALPETER,
        MSG_RESOURCE_BRONZEBAR,

        // UI actions
        MSG_CONFIRM_YES,
        MSG_CONFIRM_NO,
        MSG_BUILD_MENU,
        MSG_DELETE_FLAG,

        // Status text messages
        MSG_BUILDING_AND_FLAG_DELETED, // "Building and flag deleted!"
        MSG_CANNOT_DELETE_TOWN_HALL,   // "Cannot delete town hall flag!"
        MSG_DELETE_FLAG_PROMPT,        // "Delete building and flag? A=Yes B=No"
        MSG_FLAG_REMOVED,              // "Flag removed!"
        MSG_NO_FLAG_NEARBY,            // "No flag found nearby"
        MSG_BUILDING_STARTED,          // "Building construction started!"
        MSG_CANNOT_PLACE_FLAG_OBJECT,  // "Cannot place flag on object"
        MSG_FLAG_WATER_ONLY,           // "Flag can only be placed on water (deep or shallow)"
        MSG_FLAG_PLACED,               // "Flag placed!"
        MSG_ROAD_BUILD_HELP,           // "ROAD: A=add tile  B=cancel"
        MSG_ROAD_AUTO_PATH,            // "ROAD: auto-path built!"
        MSG_CANNOT_BUILD_HERE,         // "Cannot build here!"
        MSG_CANNOT_BUILD_THROUGH_ROAD, // "Cannot build through existing road!"
        MSG_ROAD_BUILT,                // "Road built!"
        MSG_PLACEMENT_CANCELLED,       // "Placement cancelled"
        MSG_ROAD_CANCELLED,            // "Road cancelled"
        MSG_CANCELLED,                 // "Cancelled"
        MSG_LOGISTICS_DEBUG_ON,        // "LOGISTICS DEBUG ON"
        MSG_LOGISTICS_DEBUG_OFF,       // "LOGISTICS DEBUG OFF"

        // ── Notification titles ─────────────────────────────────
        MSG_TITLE_CONSTRUCTION_COMPLETE, // "COMPLETE"
        MSG_TITLE_BUILDING,              // "BUILD"
        MSG_TITLE_FLAG,                  // "FLAG"
        MSG_TITLE_DELIVERY,              // "DELIVERY"
        MSG_TITLE_PRODUCTION,            // "PRODUCTION"

        // ── Notification description strings ────────────────────
        MSG_CONSTRUCTION_COMPLETED,      // "Construction finished!"
        MSG_BUILDING_PLACED_TEXT,        // "Building started" (avoid conflict with MSG_BUILDING_STARTED)
        MSG_FLAG_PLACED_TEXT,            // "Flag placed"
        MSG_FLAG_REMOVED_TEXT,           // "Flag removed"

        // ── Notification formats (name + amount) ────────────────
        MSG_NAME_FORMAT,                // "%s" (just the name, no count)
        MSG_DELIVERY_FORMAT,            // "%s x%d"
        MSG_PRODUCTION_FORMAT,          // "Produced %s x%d"

        // ── Count / dedup format ────────────────────────────────
        MSG_COUNT_X_FORMAT,             // "x%d"
        MSG_COUNT_X99_FORMAT,           // "x99+"

        // ── Building names ──────────────────────────────────────
        MSG_BUILDING_GENERIC,
        MSG_BUILDING_WOODCUTTER,
        MSG_BUILDING_FORESTER,
        MSG_BUILDING_SAWMILL,
        MSG_BUILDING_STONEMASON,
        MSG_BUILDING_BRONZEMINE,
        MSG_BUILDING_IRONMINE,
        MSG_BUILDING_GOLDMINE,
        MSG_BUILDING_COALMINE,
        MSG_BUILDING_IRONSMELTER,
        MSG_BUILDING_GOLDSMELTER,
        MSG_BUILDING_BRONZESMELTER,
        MSG_BUILDING_FARM,
        MSG_BUILDING_MILL,
        MSG_BUILDING_BAKERY,
        MSG_BUILDING_FISHER,
        MSG_BUILDING_HUNTER,
        MSG_BUILDING_TOOLWORKSHOP,
        MSG_BUILDING_STOREHOUSE,
        MSG_BUILDING_WELL,
        MSG_BUILDING_BARRACKS,
        MSG_BUILDING_HQ,

        // ── Menu labels ─────────────────────────────────────────────
        MSG_MENU_NEW_GAME,
        MSG_MENU_MAP_EDITOR,
        MSG_MENU_SETTINGS,
        MSG_MENU_EXIT,
        MSG_MENU_SIZE_SELECT_TITLE,  // "Select Map Size"
        MSG_MENU_HINT_BACK,          // "Back"
        MSG_MENU_HINT_SELECT,        // "Select"
        MSG_MENU_SET_FLAG,           // "Set Flag"
        MSG_MENU_DELETE_FLAG,        // "Delete Flag"
        MSG_MENU_BUILDINGS,          // "Buildings"

        MSG_COUNT
    };

    struct UiFormatArgs {
        int values[4];
        UiFormatArgs() { values[0] = 0; values[1] = 0; values[2] = 0; values[3] = 0; }
        explicit UiFormatArgs(int v0) { values[0] = v0; values[1] = 0; values[2] = 0; values[3] = 0; }
    };

} // namespace UI
