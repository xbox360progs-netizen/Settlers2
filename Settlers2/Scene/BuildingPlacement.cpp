#include "stdafx.h"
#include "BuildingPlacement.h"
#include "../World/Map.h"
#include "../World/FlagManager.h"
#include "../World/RoadManager.h"
#include "../World/DemandManager.h"
#include "../Graphics/SpriteAtlas.h"
#include "../Graphics/TextureRegistry.h"
#include "../Logic/CoordinateSystem.h"
#include "../World/ConstructionSite.h"

namespace Scene {

    BuildingPlacementManager::BuildingPlacementManager(
        World::Map* map,
        World::FlagManager* flagManager,
        World::RoadManager* roadManager,
        World::CarrierManager* carrierManager,
        Logic::EconomyManager* economyManager,
        World::DemandManager* demandManager
    )   : m_map(map), m_flagManager(flagManager)
        , m_roadManager(roadManager), m_carrierManager(carrierManager)
        , m_economyManager(economyManager), m_demandManager(demandManager)
        , m_selectedType(World::Building_None), m_isSelected(false)
        , m_entranceX(0), m_entranceY(0)
        , m_footOffX(0), m_footOffY(0)
        , m_footW(1), m_footH(1)
        , m_spriteIdx(-1), m_constrIdx(-1) {}

    void BuildingPlacementManager::SelectBuilding(World::BuildingType type) {
        m_selectedType = type;
        m_isSelected = true;
        LoadBuildingMetadata();
    }

    void BuildingPlacementManager::Deselect() {
        m_selectedType = World::Building_None;
        m_isSelected = false;
    }

    bool BuildingPlacementManager::IsSelected() const { return m_isSelected; }
    World::BuildingType BuildingPlacementManager::GetSelectedType() const { return m_selectedType; }

    int BuildingPlacementManager::GetSpriteIdx() const { return m_spriteIdx; }
    int BuildingPlacementManager::GetConstrIdx() const { return m_constrIdx; }
    int BuildingPlacementManager::GetEntranceX() const { return m_entranceX; }
    int BuildingPlacementManager::GetEntranceY() const { return m_entranceY; }
    int BuildingPlacementManager::GetFootOffX() const { return m_footOffX; }
    int BuildingPlacementManager::GetFootOffY() const { return m_footOffY; }
    int BuildingPlacementManager::GetFootW() const { return m_footW; }
    int BuildingPlacementManager::GetFootH() const { return m_footH; }
    const std::vector<std::pair<int,int>>& BuildingPlacementManager::GetFootMask() const { return m_footMask; }
    const std::string& BuildingPlacementManager::GetSpriteName() const { return m_spriteName; }

    void BuildingPlacementManager::LoadBuildingMetadata() {
        m_entranceX = 0; m_entranceY = 0;
        m_footOffX = 0; m_footOffY = 0;
        m_footW = 1; m_footH = 1;
        m_footMask.clear();
        m_spriteIdx = -1;
        m_constrIdx = -1;
        m_spriteName.clear();

        const char* namePtr = GetBuildingSpriteName(m_selectedType);
        if (namePtr && namePtr[0]) m_spriteName = namePtr;

        TextureRegistry& reg = TextureRegistry::instance();
        reg.getTextureOrLoad("Buildings");
        std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
        if (!buildingsAtlas) return;

        m_constrIdx = (int)buildingsAtlas->GetIndex("construction");
        if (m_constrIdx < 0) m_constrIdx = (int)buildingsAtlas->GetIndex("Construction");
        if (m_constrIdx < 0) m_constrIdx = (int)buildingsAtlas->GetIndex("ConstructionSite");

        m_spriteIdx = FindBuildingSpriteIdx(m_spriteName);
        if (m_spriteIdx < 0) {
            const char* fallbacks[] = { "b_warehouse", "b_residence", "b_well", "b_mason" };
            for (int fi = 0; fi < 4 && m_spriteIdx < 0; ++fi)
                m_spriteIdx = (int)buildingsAtlas->GetIndex(fallbacks[fi]);
        }

        if (m_spriteIdx >= 0) {
            const SpriteRegion* r = buildingsAtlas->GetRegion((uint32_t)m_spriteIdx);
            if (r) {
                m_footOffX = r->collOffX;
                m_footOffY = r->collOffY;
                m_footW = (int)r->collWidth;
                m_footH = (int)r->collHeight;
                m_footMask = r->collMask;
                if (!IsMineType(m_selectedType)) {
                    m_entranceX = r->entranceX;
                    m_entranceY = r->entranceY;
                }
            }
        }

        bool is2x2 = (m_selectedType == World::Stonemason || m_selectedType == World::Sawmill
            || m_selectedType == World::Farm || m_selectedType == World::Mill);
        if (is2x2 && (m_footW != 2 || m_footH != 2)) { m_footW = 2; m_footH = 2; }
    }

    int BuildingPlacementManager::FindBuildingSpriteIdx(const std::string& spriteName) {
        if (spriteName.empty()) return -1;
        TextureRegistry& reg = TextureRegistry::instance();
        reg.getTextureOrLoad("Buildings");
        std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
        if (!buildingsAtlas) return -1;
        uint32_t idx = buildingsAtlas->GetIndex(spriteName.c_str());
        if (idx == 0xFFFFFFFF) {
            std::string lower = spriteName;
            for (size_t ci = 0; ci < lower.size(); ++ci)
                if (lower[ci] >= 'A' && lower[ci] <= 'Z') lower[ci] = lower[ci] - 'A' + 'a';
            idx = buildingsAtlas->GetIndex(lower.c_str());
        }
        if (idx == 0xFFFFFFFF) {
            std::string prefixed = std::string("b_") + spriteName;
            idx = buildingsAtlas->GetIndex(prefixed.c_str());
        }
        return (idx != 0xFFFFFFFF) ? (int)idx : -1;
    }

    PlacementData BuildingPlacementManager::GetPlacementData(int cursorX, int cursorY) {
        PlacementData data;
        if (!m_isSelected || !m_map) { data.errorMsg = "no building selected"; return data; }

        int entranceX = m_entranceX, entranceY = m_entranceY;
        int buildY = cursorY - entranceY;
        bool buildingEvenY = (buildY % 2 == 0);
        if (!buildingEvenY && entranceY != 0 && entranceX > 0) entranceX--;
        int buildX = cursorX - entranceX;

        data.buildX = buildX; data.buildY = buildY;
        data.flagX = cursorX; data.flagY = cursorY;
        data.entranceX = entranceX; data.entranceY = entranceY;
        data.footOffX = m_footOffX; data.footOffY = m_footOffY;
        data.footW = m_footW; data.footH = m_footH;
        data.footMask = m_footMask;
        data.spriteIdx = m_spriteIdx;
        data.constrIdx = m_constrIdx;

        if (m_spriteIdx >= 0) {
            TextureRegistry& reg = TextureRegistry::instance();
            std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
            if (buildingsAtlas) data.spriteRegion = buildingsAtlas->GetRegion((uint32_t)m_spriteIdx);
        }

        int anchorX = buildX + m_footOffX, anchorY = buildY + m_footOffY;
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        int nodesW = coords.GetNodesWidth(), nodesH = coords.GetNodesHeight();

        World::ResourceType requiredRes = GetResourceTypeForMine(m_selectedType);

        data.footprintTiles.clear();
        auto checkTile = [&](int tx, int ty) -> bool {
            if (tx < 0 || tx >= nodesW || ty < 0 || ty >= nodesH) return false;
            World::TileLayer* bl = m_map->GetLayer(World::Buildings);
            if (bl && bl->GetTile(tx, ty).type != World::Tile_None) return false;
            // Mines replace resource mountain tiles — skip Objects layer check
            if (requiredRes == World::ResourceType_None) {
                World::TileLayer* ol = m_map->GetLayer(World::Objects);
                if (ol) { const World::Tile& ot = ol->GetTile(tx, ty); if (ot.u1 > ot.u0 && ot.v1 > ot.v0) return false; }
            }
            if (m_flagManager && m_flagManager->GetFlagAt(tx, ty)) return false;
            return true;
        };

        if (!m_footMask.empty()) {
            for (size_t i = 0; i < m_footMask.size(); ++i) {
                int tx = anchorX + m_footMask[i].first, ty = anchorY + m_footMask[i].second;
                data.footprintTiles.push_back(std::make_pair(tx, ty));
                if (!checkTile(tx, ty)) { data.errorMsg = "occupied"; return data; }
            }
        } else {
            for (int dy = 0; dy < m_footH; ++dy)
                for (int dx = 0; dx < m_footW; ++dx) {
                    int tx = anchorX + dx, ty = anchorY + dy;
                    data.footprintTiles.push_back(std::make_pair(tx, ty));
                    if (!checkTile(tx, ty)) { data.errorMsg = "occupied"; return data; }
                }
        }

        if (requiredRes != World::ResourceType_None) {
            if (anchorX < 0 || anchorX >= nodesW || anchorY < 0 || anchorY >= nodesH) { data.errorMsg = "out of bounds"; return data; }
            const World::ResourceNode& node = m_map->GetResourceNode(anchorX, anchorY);
            if (node.type != requiredRes) { data.errorMsg = "no resource node"; return data; }
        }

        BYTE flagWeight = m_map->GetNodeWeight(cursorX, cursorY);
        if (flagWeight == World::Weight_Deep) { data.errorMsg = "deep water"; return data; }

        if (!m_spriteName.empty()) {
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[Placement] type=%d sprite='%s' offset=(%d,%d) build=(%d,%d) flag=(%d,%d) valid=%d err=%s\n",
                m_selectedType, m_spriteName.c_str(), entranceX, entranceY, buildX, buildY, cursorX, cursorY, 1, "ok");
            OutputDebugStringA(dbg);
        }

        data.valid = true;
        return data;
    }

    // ─── Static helpers ───────────────────────────────────────────────

    const char* BuildingPlacementManager::GetBuildingSpriteName(World::BuildingType type) {
        switch (type) {
            case World::Woodcutter:   return "b_woodcutter";
            case World::Forester:     return "b_forester";
            case World::Sawmill:      return "b_sawmill";
            case World::Stonemason:   return "b_mason";
            case World::CoalMine:
            case World::BronzeMine:   return "b_mine";
            case World::IronMine:     return "b_mine";
            case World::GoldMine:     return "b_mine";
            case World::IronSmelter:  return "b_ironsmelter";
            case World::GoldSmelter:  return "b_goldsmelter";
            case World::BronzeSmelter: return "b_bronzesmelter";
            case World::Farm:         return "b_farm";
            case World::Mill:         return "b_mill";
            case World::Bakery:       return "b_bakery";
            case World::Fisher:       return "b_fisher";
            case World::Hunter:       return "b_hunter";
            case World::ToolWorkshop: return "b_toolworkshop";
            case World::Storehouse:   return "b_warehouse";
            case World::Well:         return "b_well";
            case World::Barracks:     return "b_barracks";
            default:                  return "";
        }
    }

    World::ResourceType BuildingPlacementManager::GetResourceTypeForMine(World::BuildingType type) {
        switch (type) {
            case World::CoalMine:   return World::ResourceType_Coal;
            case World::BronzeMine: return World::ResourceType_BronzeOre;
            case World::IronMine:   return World::ResourceType_IronOre;
            case World::GoldMine:   return World::ResourceType_GoldOre;
            default:                return World::ResourceType_None;
        }
    }

    bool BuildingPlacementManager::IsMineType(World::BuildingType type) {
        return GetResourceTypeForMine(type) != World::ResourceType_None;
    }

    void BuildingPlacementManager::GetEntranceOffset(const std::string& buildingName, int& outX, int& outY) {
        outX = 0; outY = 0;
        if (buildingName.empty()) return;
        TextureRegistry& reg = TextureRegistry::instance();
        reg.getTextureOrLoad("Buildings");
        std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
        if (!buildingsAtlas) return;
        uint32_t idx = buildingsAtlas->GetIndex(buildingName.c_str());
        if (idx == 0xFFFFFFFF) {
            std::string lower = buildingName;
            for (size_t ci = 0; ci < lower.size(); ++ci)
                if (lower[ci] >= 'A' && lower[ci] <= 'Z') lower[ci] = lower[ci] - 'A' + 'a';
            idx = buildingsAtlas->GetIndex(lower.c_str());
        }
        if (idx == 0xFFFFFFFF) {
            std::string prefixed = std::string("b_") + buildingName;
            idx = buildingsAtlas->GetIndex(prefixed.c_str());
        }
        if (idx == 0xFFFFFFFF) {
            std::string pl = std::string("b_") + buildingName;
            for (size_t ci = 0; ci < pl.size(); ++ci)
                if (pl[ci] >= 'A' && pl[ci] <= 'Z') pl[ci] = pl[ci] - 'A' + 'a';
            idx = buildingsAtlas->GetIndex(pl.c_str());
        }
        if (idx != 0xFFFFFFFF) {
            const SpriteRegion* r = buildingsAtlas->GetRegion(idx);
            if (r) { outX = r->entranceX; outY = r->entranceY; }
        }
    }

} // namespace Scene
