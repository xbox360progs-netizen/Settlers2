#pragma once
#include <map>
#include "../World/ResourceNode.h"

namespace Logic {

    enum ResourceRouting {
        ROUTE_WAREHOUSE, // surplus → warehouse (used for construction materials)
        ROUTE_DIRECT     // surplus stays at flag for direct delivery to consumers
    };

    struct ResourceRouteConfig {
        ResourceRouting routing;
        uint32_t transferPriority;

        ResourceRouteConfig() : routing(ROUTE_WAREHOUSE), transferPriority(10) {}
        ResourceRouteConfig(ResourceRouting r, uint32_t p) : routing(r), transferPriority(p) {}
    };

    struct EconomySettings {
        ResourceRouteConfig routeConfig[World::ResourceType_Count];

        EconomySettings() {
            for (int i = 0; i < World::ResourceType_Count; ++i) {
                routeConfig[i] = ResourceRouteConfig(ROUTE_WAREHOUSE, 10);
            }

            // Raw materials: keep at flag for direct delivery to processing buildings
            routeConfig[World::ResourceType_Wood]  = ResourceRouteConfig(ROUTE_DIRECT, 30);
            routeConfig[World::ResourceType_Wheat] = ResourceRouteConfig(ROUTE_DIRECT, 30);
            routeConfig[World::ResourceType_Meat]  = ResourceRouteConfig(ROUTE_WAREHOUSE, 25);

            // Processed goods (produced by processing buildings, consumed by advanced buildings)
            routeConfig[World::ResourceType_Planks]  = ResourceRouteConfig(ROUTE_WAREHOUSE, 20);
            routeConfig[World::ResourceType_Flour]   = ResourceRouteConfig(ROUTE_DIRECT, 20);
            routeConfig[World::ResourceType_IronBar] = ResourceRouteConfig(ROUTE_DIRECT, 35);
            routeConfig[World::ResourceType_GoldBar] = ResourceRouteConfig(ROUTE_WAREHOUSE, 15);

            // Finished goods: to warehouse
            routeConfig[World::ResourceType_Bread] = ResourceRouteConfig(ROUTE_WAREHOUSE, 50);
            routeConfig[World::ResourceType_Trap]  = ResourceRouteConfig(ROUTE_WAREHOUSE, 40);
            routeConfig[World::ResourceType_Stone] = ResourceRouteConfig(ROUTE_WAREHOUSE, 10);

            // Raw resources: to warehouse
            routeConfig[World::ResourceType_Coal]   = ResourceRouteConfig(ROUTE_WAREHOUSE, 45);
            routeConfig[World::ResourceType_IronOre] = ResourceRouteConfig(ROUTE_WAREHOUSE, 35);
            routeConfig[World::ResourceType_GoldOre] = ResourceRouteConfig(ROUTE_WAREHOUSE, 15);
            routeConfig[World::ResourceType_Fish]    = ResourceRouteConfig(ROUTE_WAREHOUSE, 25);

            // Priorities for transport (higher = more urgent, carrier picks first)
            routeConfig[World::ResourceType_Coal]    .transferPriority = 50;
            routeConfig[World::ResourceType_IronOre] .transferPriority = 40;
            routeConfig[World::ResourceType_Bread]   .transferPriority = 60;

            // Bronze: route to warehouse (no downstream consumer yet)
            routeConfig[World::ResourceType_BronzeBar] = ResourceRouteConfig(ROUTE_WAREHOUSE, 30);
        }
    };
}
