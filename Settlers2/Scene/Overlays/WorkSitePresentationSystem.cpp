#include "stdafx.h"
#include "WorkSitePresentationSystem.h"
#include "../../Graphics/TextureRegistry.h"
#include "../../Graphics/SpriteAtlas.h"
#include "../../Logic/CoordinateSystem.h"
#include "../../Logic/EconomyManager.h"
#include "../../World/Components/Building.h"
#include <string>

namespace Scene {

void WorkSitePresentationSystem::SetEconomyManager(Logic::EconomyManager* economy)
{
    m_economy = economy;
}

void WorkSitePresentationSystem::BuildRenderFrame(
    std::vector<RenderWorkSite>& out)
{
    if (!m_economy) return;

    std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas =
        TextureRegistry::instance().getAtlas("Buildings");

    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    for (int i = 0; i < m_economy->GetBuildingCount(); ++i) {
        World::Building* b = m_economy->GetBuilding(i);
        if (!b) continue;

        Vector2i wsPos;
        const char* wsSpriteName = NULL;
        if (!b->GetWorkSiteRenderInfo(wsPos, wsSpriteName)) continue;
        if (!wsSpriteName || !*wsSpriteName) continue;

        uint32_t sprIdx = 0xFFFFFFFF;
        if (buildingsAtlas) {
            sprIdx = buildingsAtlas->GetIndex(wsSpriteName);
            if (sprIdx == 0xFFFFFFFF) {
                std::string lowerName = wsSpriteName;
                for (size_t ci = 0; ci < lowerName.size(); ++ci)
                    if (lowerName[ci] >= 'A' && lowerName[ci] <= 'Z')
                        lowerName[ci] = lowerName[ci] - 'A' + 'a';
                sprIdx = buildingsAtlas->GetIndex(lowerName.c_str());
            }
        }
        if (sprIdx == 0xFFFFFFFF) continue;

        RenderWorkSite ws;
        coords.NodeTileToWorld(wsPos.x, wsPos.y,
            ws.transform.worldX, ws.transform.worldY);
        ws.transform.depthLayer = static_cast<int>(0.97f * 65535.0f);
        ws.spriteIdx = static_cast<int>(sprIdx);
        out.push_back(ws);
    }
}

} // namespace Scene
