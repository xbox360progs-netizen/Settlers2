#include "stdafx.h"
#include "MenuBootstrap.h"
#include "TextureSlots.h"
#include "../Graphics/TextureRegistry.h"
#include "../Graphics/SpriteAtlas.h"
#include "../Graphics/Renderer.h"

namespace MenuBootstrap {

bool SetupBuildMenu(GridMenu* buildMenu, Renderer* renderer)
{
    if (!buildMenu) {
        OutputDebugStringA("[MenuBootstrap] WARNING: buildMenu is null\n");
        return false;
    }

    TextureRegistry& reg = TextureRegistry::instance();
    reg.getTextureOrLoad("ui");
    std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
    if (!uiAtlas) {
        OutputDebugStringA("[MenuBootstrap] WARNING: UI atlas not found\n");
        return false;
    }

    LPDIRECT3DTEXTURE9 uiTex = uiAtlas->GetTexture();
    if (!uiTex) {
        OutputDebugStringA("[MenuBootstrap] WARNING: UI atlas has no texture\n");
        return false;
    }

    reg.getTextureOrLoad("Icon");
    std::tr1::shared_ptr<SpriteAtlas> iconAtlas = reg.getAtlas("Icon");
    if (!iconAtlas) {
        OutputDebugStringA("[MenuBootstrap] WARNING: Icon atlas not found\n");
        return false;
    }

    LPDIRECT3DTEXTURE9 iconTex = iconAtlas->GetTexture();
    if (!iconTex) {
        OutputDebugStringA("[MenuBootstrap] WARNING: Icon atlas has no texture\n");
        return false;
    }

    if (renderer) {
        SpriteRenderer* sr = renderer->GetSpriteRenderer();
        if (sr) {
            sr->SetTextureSlot(SLOT_UI_MENU_BG, uiTex);
            sr->SetTextureSlot(SLOT_UI_MENU_CELL, uiTex);
            sr->SetTextureSlot(SLOT_UI_MENU_ICON, iconTex);
        }
    }

    buildMenu->SetTextureSlots(SLOT_UI_MENU_BG, SLOT_UI_MENU_CELL, SLOT_UI_MENU_ICON);
    buildMenu->SetTextures(uiTex, uiTex, iconTex);

    GridMenu::TileUV bgUV = {0,0,1,1}, cellUV = {0,0,1,1};
    uint32_t bgIdx = uiAtlas->GetIndex("menu_Grid");
    if (bgIdx != 0xFFFFFFFF) {
        const SpriteRegion* r = uiAtlas->GetRegion(bgIdx);
        if (r) { bgUV.u0 = r->u0; bgUV.v0 = r->v0; bgUV.u1 = r->u1; bgUV.v1 = r->v1; }
    } else {
        OutputDebugStringA("[MenuBootstrap] WARNING: 'menu_Grid' NOT FOUND\n");
    }
    uint32_t cellIdx = uiAtlas->GetIndex("menu_cell1");
    if (cellIdx != 0xFFFFFFFF) {
        const SpriteRegion* r = uiAtlas->GetRegion(cellIdx);
        if (r) { cellUV.u0 = r->u0; cellUV.v0 = r->v0; cellUV.u1 = r->u1; cellUV.v1 = r->v1; }
    } else {
        OutputDebugStringA("[MenuBootstrap] WARNING: 'menu_cell1' NOT FOUND\n");
    }
    buildMenu->SetBackgroundUV(bgUV);
    buildMenu->SetCellUV(cellUV);

    std::vector<GridMenu::TileUV> tileUVs;
    std::vector<int> spriteIndices;
    std::vector<std::string> cellLabels;

    const std::vector<uint32_t>* groupIndices = iconAtlas->GetGroup("icon_building");
    if (groupIndices && !groupIndices->empty()) {
        for (size_t gi = 0; gi < groupIndices->size(); ++gi) {
            uint32_t spriteIdx = (*groupIndices)[gi];
            const SpriteRegion* reg = iconAtlas->GetRegion(spriteIdx);
            if (!reg) continue;
            GridMenu::TileUV uv;
            uv.u0 = reg->u0; uv.v0 = reg->v0;
            uv.u1 = reg->u1; uv.v1 = reg->v1;
            tileUVs.push_back(uv);
            spriteIndices.push_back((int)spriteIdx);
            std::string label = reg->name;
            if (label.compare(0, 3, "ib_") == 0) label = label.substr(3);
            cellLabels.push_back(label);
        }
    }

    {
        char dbg[256];
        _snprintf(dbg, sizeof(dbg), "[MenuBootstrap] Found %d sprites in icon_building group\n", (int)tileUVs.size());
        OutputDebugStringA(dbg);
    }

    if (tileUVs.empty()) {
        OutputDebugStringA("[MenuBootstrap] icon_building group empty\n");
        return false;
    }

    buildMenu->SetCellLabels(cellLabels);
    buildMenu->SetCellSpacing(80.0f, 80.0f);
    buildMenu->SetCellPadding(4.0f);
    buildMenu->SetCellVisualSize(64.0f, 64.0f);
    buildMenu->SetTileData(tileUVs, spriteIndices);

    OutputDebugStringA("[MenuBootstrap::SetupBuildMenu] DONE\n");
    return true;
}

bool SetupRoadMenu(GridMenu* roadMenu, Renderer* renderer)
{
    if (!roadMenu) {
        OutputDebugStringA("[MenuBootstrap] WARNING: roadMenu is null\n");
        return false;
    }

    TextureRegistry& reg = TextureRegistry::instance();
    reg.getTextureOrLoad("ui");
    std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
    if (!uiAtlas) {
        OutputDebugStringA("[MenuBootstrap] WARNING: UI atlas not found for road menu\n");
        return false;
    }

    LPDIRECT3DTEXTURE9 uiTex = uiAtlas->GetTexture();
    if (!uiTex) {
        OutputDebugStringA("[MenuBootstrap] WARNING: UI atlas has no texture\n");
        return false;
    }

    if (renderer) {
        SpriteRenderer* sr = renderer->GetSpriteRenderer();
        if (sr) {
            sr->SetTextureSlot(SLOT_UI_ROAD_BG, uiTex);
            sr->SetTextureSlot(SLOT_UI_ROAD_CELL, uiTex);
            sr->SetTextureSlot(SLOT_UI_ROAD_ICON, uiTex);
        }
    }

    roadMenu->SetTextureSlots(SLOT_UI_ROAD_BG, SLOT_UI_ROAD_CELL, SLOT_UI_ROAD_ICON);
    roadMenu->SetBackgroundTexture(uiTex);
    roadMenu->SetAtlasTexture(uiTex);

    GridMenu::TileUV bgUV = {0,0,1,1};
    uint32_t newBgIdx = uiAtlas->GetIndex("menu_creat_flag_road");
    if (newBgIdx != 0xFFFFFFFF) {
        const SpriteRegion* r = uiAtlas->GetRegion(newBgIdx);
        if (r) {
            bgUV.u0 = r->u0; bgUV.v0 = r->v0;
            bgUV.u1 = r->u1; bgUV.v1 = r->v1;
            roadMenu->SetMenuSize((float)r->width, (float)r->height);
        }
    } else {
        uint32_t bgIdx = uiAtlas->GetIndex("menu_Grid");
        if (bgIdx != 0xFFFFFFFF) {
            const SpriteRegion* r = uiAtlas->GetRegion(bgIdx);
            if (r) { bgUV.u0 = r->u0; bgUV.v0 = r->v0; bgUV.u1 = r->u1; bgUV.v1 = r->v1; }
        }
    }
    roadMenu->SetBackgroundUV(bgUV);

    const char* iconNames[] = { "icon_set_flag", "icon_delete_flag", "icon_Streets" };
    const char* iconLabels[] = { "Set Flag", "Delete Flag", "Buildings" };
    std::vector<GridMenu::TileUV> tileUVs;
    std::vector<int> spriteIndices;
    std::vector<std::string> cellLabels;
    int iconH = 32;
    for (int i = 0; i < 3; ++i) {
        uint32_t idx = uiAtlas->GetIndex(iconNames[i]);
        if (idx == 0xFFFFFFFF) {
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[MenuBootstrap] WARNING: '%s' NOT FOUND\n", iconNames[i]);
            OutputDebugStringA(dbg);
            continue;
        }
        const SpriteRegion* r = uiAtlas->GetRegion(idx);
        if (!r) continue;
        GridMenu::TileUV uv;
        uv.u0 = r->u0; uv.v0 = r->v0;
        uv.u1 = r->u1; uv.v1 = r->v1;
        tileUVs.push_back(uv);
        spriteIndices.push_back((int)idx);
        cellLabels.push_back(iconLabels[i]);
        if ((int)r->height > iconH) iconH = (int)r->height;
    }

    roadMenu->SetCellLabels(cellLabels);
    roadMenu->SetCellSpacing(110.0f, 60.0f);
    roadMenu->SetCellPadding(4.0f);
    roadMenu->SetCellVisualSize((float)iconH, (float)iconH);
    roadMenu->SetTileData(tileUVs, spriteIndices);

    OutputDebugStringA("[MenuBootstrap::SetupRoadMenu] DONE\n");
    return true;
}

bool SetupFlagMenu(UIMenu* flagMenu, UIMenu::ItemData* itemData, int maxItems, int& outItemCount)
{
    if (!flagMenu || !itemData || maxItems < 3) {
        OutputDebugStringA("[MenuBootstrap] WARNING: invalid params for SetupFlagMenu\n");
        return false;
    }

    TextureRegistry& reg = TextureRegistry::instance();
    reg.getTextureOrLoad("ui");
    std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
    if (!uiAtlas) {
        OutputDebugStringA("[MenuBootstrap] WARNING: UI atlas not found for flag menu\n");
        return false;
    }

    LPDIRECT3DTEXTURE9 uiTex = uiAtlas->GetTexture();
    if (!uiTex) {
        OutputDebugStringA("[MenuBootstrap] WARNING: UI atlas has no texture\n");
        return false;
    }

    const float scale = 0.5f;

    UIMenu::BackgroundData bg = {0,0,1,1, 0,0,0,0};
    uint32_t bgIdx = uiAtlas->GetIndex("menu_creat_flag_road");
    if (bgIdx != 0xFFFFFFFF) {
        const SpriteRegion* r = uiAtlas->GetRegion(bgIdx);
        if (r) {
            bg.u0 = r->u0; bg.v0 = r->v0;
            bg.u1 = r->u1; bg.v1 = r->v1;
            bg.w = (float)r->width * scale;
            bg.h = (float)r->height * scale;
            bg.x = 640.0f - bg.w * 0.5f;
            bg.y = 360.0f - bg.h * 0.5f;
        }
    }
    flagMenu->SetBackground(bg);
    flagMenu->SetAtlas(uiTex, SLOT_UI_ROAD_BG);

    const char* iconNames[] = { "icon_set_flag", "icon_delete_flag", "icon_Streets" };
    const char* iconLabels[] = { "Set Flag", "Delete Flag", "Build Road" };

    outItemCount = 3;
    float menuCX = 640.0f;
    float menuY = bg.y + bg.h * 0.5f;
    float itemSpacing = bg.w / (float)(outItemCount + 1);
    int iconSize = 32;

    for (int i = 0; i < outItemCount; ++i) {
        uint32_t idx = uiAtlas->GetIndex(iconNames[i]);
        UIMenu::ItemData& item = itemData[i];
        if (idx != 0xFFFFFFFF) {
            const SpriteRegion* r = uiAtlas->GetRegion(idx);
            if (r) {
                item.u0 = r->u0; item.v0 = r->v0;
                item.u1 = r->u1; item.v1 = r->v1;
                item.w = (float)r->width * scale;
                item.h = (float)r->height * scale;
            }
        }
        if (item.w < 1.0f) { item.w = (float)iconSize; item.h = (float)iconSize; }
        item.x = menuCX + (float)(i - 1) * itemSpacing - item.w * 0.5f;
        item.y = menuY - item.h * 0.5f;
        item.label = iconLabels[i];
    }
    flagMenu->SetItems(itemData, outItemCount);

    OutputDebugStringA("[MenuBootstrap::SetupFlagMenu] DONE\n");
    return true;
}

bool SetupGeologistMenu(UIMenu* geologistMenu)
{
    if (!geologistMenu) {
        OutputDebugStringA("[MenuBootstrap] WARNING: geologistMenu is null\n");
        return false;
    }

    TextureRegistry& reg = TextureRegistry::instance();
    reg.getTextureOrLoad("ui");
    std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
    if (!uiAtlas) {
        OutputDebugStringA("[MenuBootstrap] WARNING: UI atlas not found for geologist menu\n");
        return false;
    }

    LPDIRECT3DTEXTURE9 uiTex = uiAtlas->GetTexture();
    if (!uiTex) {
        OutputDebugStringA("[MenuBootstrap] WARNING: UI atlas has no texture\n");
        return false;
    }

    UIMenu::BackgroundData bg = {0,0,1,1, 0,0,0,0};
    bool bgFound = false;
    const char* bgNames[] = { "geologist_menu", "menu_creat_flag_road" };
    for (int bi = 0; bi < 2 && !bgFound; ++bi) {
        uint32_t bgIdx = uiAtlas->GetIndex(bgNames[bi]);
        if (bgIdx != 0xFFFFFFFF) {
            const SpriteRegion* r = uiAtlas->GetRegion(bgIdx);
            if (r) {
                bg.u0 = r->u0; bg.v0 = r->v0;
                bg.u1 = r->u1; bg.v1 = r->v1;
                bg.w = (float)r->width;
                bg.h = (float)r->height;
                bg.x = 640.0f - bg.w * 0.5f;
                bg.y = 360.0f - bg.h * 0.5f;
                bgFound = true;
            }
        }
    }
    if (bgFound) {
        geologistMenu->SetBackground(bg);
    }
    geologistMenu->SetAtlas(uiTex, SLOT_UI_MENU_BG);

    OutputDebugStringA("[MenuBootstrap::SetupGeologistMenu] DONE\n");
    return true;
}

World::BuildingType GetBuildingTypeFromSpriteName(const std::string& name)
{
    std::string key = name;
    if (key.compare(0, 2, "b_") == 0)
        key = key.substr(2);

    struct { const char* name; World::BuildingType type; } entries[] = {
        { "woodcutter",     World::Woodcutter },
        { "sawmill",        World::Sawmill },
        { "coalmine",       World::CoalMine },
        { "ironmine",       World::IronMine },
        { "goldmine",       World::GoldMine },
        { "ironsmelter",    World::IronSmelter },
        { "goldsmelter",    World::GoldSmelter },
        { "farm",           World::Farm },
        { "mill",           World::Mill },
        { "bakery",         World::Bakery },
        { "fisher",         World::Fisher },
        { "hunter",         World::Hunter },
        { "toolworkshop",   World::ToolWorkshop },
        { "warehouse",      World::Storehouse },
        { "townhall",       World::Storehouse },
        { "bronzemine",     World::BronzeMine },
        { "bronzesmelter",  World::BronzeSmelter },
    };

    for (int i = 0; i < sizeof(entries)/sizeof(entries[0]); ++i) {
        if (key == entries[i].name)
            return entries[i].type;
    }
    return World::Building_None;
}

} // namespace MenuBootstrap
