#include "stdafx.h"
#include "BuildingDefinition.h"

namespace World {

const BuildingDefinition g_buildingDefinitions[BUILDING_TYPE_COUNT] = {
    // 0:  Building_None
    { NULL, NULL, 0, 0, 0, 0, UI::MSG_NONE, 0 },

    // 1:  Hut
    { NULL, NULL, 0, 0, 0, 0, UI::MSG_NONE, 0 },

    // 2:  Tower
    { NULL, NULL, 0, 0, 0, 0, UI::MSG_NONE, 0 },

    // 3:  Fortress
    { NULL, NULL, 0, 0, 0, 0, UI::MSG_NONE, 0 },

    // 4:  Castle
    { NULL, NULL, 0, 0, 0, 0, UI::MSG_NONE, 0 },

    // 5:  Forester
    { "b_forester", "ib_forester", 2, 0, 1, 1, UI::MSG_BUILDING_FORESTER, 1 },

    // 6:  Woodcutter
    { "b_woodcutter", "ib_woodcutter", 3, 0, 1, 1, UI::MSG_BUILDING_WOODCUTTER, 1 },

    // 7:  Sawmill
    { "b_sawmill", "ib_sawmill", 6, 0, 2, 2, UI::MSG_BUILDING_SAWMILL, 2 },

    // 8:  Stonemason
    { "b_mason", "ib_stonemason", 2, 0, 2, 2, UI::MSG_BUILDING_STONEMASON, 1 },

    // 9:  CoalMine
    { "b_mine", "ib_coalmine", 4, 0, 1, 1, UI::MSG_BUILDING_COALMINE, 3 },

    // 10: IronMine
    { "b_mine", "ib_ironmine", 4, 0, 1, 1, UI::MSG_BUILDING_IRONMINE, 3 },

    // 11: GoldMine
    { "b_mine", "ib_goldmine", 4, 0, 1, 1, UI::MSG_BUILDING_GOLDMINE, 3 },

    // 12: IronSmelter
    { "b_ironsmelter", "ib_ironsmelter", 3, 3, 1, 1, UI::MSG_BUILDING_IRONSMELTER, 2 },

    // 13: GoldSmelter
    { "b_goldsmelter", "ib_goldsmelter", 3, 3, 1, 1, UI::MSG_BUILDING_GOLDSMELTER, 2 },

    // 14: Farm
    { "b_farm", "ib_farm", 4, 0, 2, 2, UI::MSG_BUILDING_FARM, 1 },

    // 15: Mill
    { "b_mill", "ib_mill", 4, 2, 2, 2, UI::MSG_BUILDING_MILL, 2 },

    // 16: Bakery
    { "b_bakery", "ib_bakery", 3, 2, 1, 1, UI::MSG_BUILDING_BAKERY, 2 },

    // 17: Fisher
    { "b_fisher", "ib_fisher", 3, 0, 1, 1, UI::MSG_BUILDING_FISHER, 1 },

    // 18: Hunter
    { "b_hunter", "ib_hunter", 3, 0, 1, 1, UI::MSG_BUILDING_HUNTER, 1 },

    // 19: Baker
    { NULL, NULL, 0, 0, 0, 0, UI::MSG_NONE, 0 },

    // 20: Brewer
    { NULL, NULL, 0, 0, 0, 0, UI::MSG_NONE, 0 },

    // 21: ToolWorkshop
    { "b_toolworkshop", "ib_toolworkshop", 4, 3, 1, 1, UI::MSG_BUILDING_TOOLWORKSHOP, 2 },

    // 22: Storehouse
    { "b_warehouse", "ib_warehouse", 2, 0, 1, 1, UI::MSG_BUILDING_STOREHOUSE, 0 },

    // 23: Residence
    { NULL, NULL, 0, 0, 0, 0, UI::MSG_NONE, 0 },

    // 24: Stronghold
    { NULL, NULL, 0, 0, 0, 0, UI::MSG_NONE, 0 },

    // 25: Well
    { "b_well", "ib_well", 2, 0, 1, 1, UI::MSG_BUILDING_WELL, 0 },

    // 26: BronzeMine
    { "b_mine", "ib_bronzemine", 2, 1, 1, 1, UI::MSG_BUILDING_BRONZEMINE, 3 },

    // 27: ToolMaker
    { NULL, NULL, 0, 0, 0, 0, UI::MSG_NONE, 0 },

    // 28: Barracks
    { "b_barracks", "ib_barracks", 2, 0, 1, 1, UI::MSG_BUILDING_BARRACKS, 0 },

    // 29: BronzeSmelter
    { "b_bronzesmelter", "ib_bronzesmelter", 2, 2, 1, 1, UI::MSG_BUILDING_BRONZESMELTER, 2 },
};

} // namespace World
