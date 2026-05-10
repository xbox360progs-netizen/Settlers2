#include "stdafx.h"
#include "TilePalette.h"
#include <cstdio>

namespace UI {

struct TileButtonData {
    TilePalette* palette;
    World::TileType tileType;
};

static TileButtonData* s_buttonData[16];
static int s_buttonDataCount = 0;

TilePalette::TilePalette()
    : Widget()
    , m_selectedType(World::None)
    , m_columns(4)
    , m_buttonSize(48.0f)
    , m_callback(0)
    , m_callbackData(0)
{
    SetSize(200.0f, 300.0f);
}

TilePalette::~TilePalette() {
    for (int i = 0; i < s_buttonDataCount; ++i) {
        delete s_buttonData[i];
    }
    s_buttonDataCount = 0;

    for (size_t i = 0; i < m_tileButtons.size(); ++i) {
        delete m_tileButtons[i].button;
    }
    m_tileButtons.clear();
}

void TilePalette::CreateTileButtons(IDirect3DDevice9* device) {
	World::TileType tileTypes[] = {
		World::Tree, World::Mountain, World::MountainOnWater,
		World::Rock, World::Building, World::Decoration
	};

	const char* tileNames[] = {
		"Tree", "Mountain", "Water Mountain",
		"Rock", "Building", "Decoration"
	};

    int count = sizeof(tileTypes) / sizeof(tileTypes[0]);

    for (int i = 0; i < count; ++i) {
        TileButtonInfo info;
        info.button = new Button();
        info.button->SetText(tileNames[i]);
        info.button->SetSize(m_buttonSize, m_buttonSize);
        info.tileType = tileTypes[i];

        TileButtonData* data = new TileButtonData();
        data->palette = this;
        data->tileType = tileTypes[i];
        if (s_buttonDataCount < 16) {
            s_buttonData[s_buttonDataCount++] = data;
        }

        info.button->SetCallback(OnTileButtonClicked, data);
        info.button->Initialize(device);

        m_tileButtons.push_back(info);
    }
}

void TilePalette::LayoutButtons() {
    float baseX = 0.0f, baseY = 0.0f;
    GetPosition(baseX, baseY);

    for (size_t i = 0; i < m_tileButtons.size(); ++i) {
        int row = static_cast<int>(i) / m_columns;
        int col = static_cast<int>(i) % m_columns;

        float posX = baseX + static_cast<float>(col) * (m_buttonSize + 4.0f);
        float posY = baseY + static_cast<float>(row) * (m_buttonSize + 4.0f);

        m_tileButtons[i].button->SetPosition(posX, posY);
    }
}

void TilePalette::Update(float deltaTime) {
    Widget::Update(deltaTime);
    for (size_t i = 0; i < m_tileButtons.size(); ++i) {
        m_tileButtons[i].button->Update(deltaTime);
    }
}

void TilePalette::Render(IDirect3DDevice9* device) {
    Widget::Render(device);
    for (size_t i = 0; i < m_tileButtons.size(); ++i) {
        m_tileButtons[i].button->Render(device);
    }
}

void TilePalette::SetSelectedTileType(World::TileType type) {
    m_selectedType = type;
}

void TilePalette::OnTileButtonClicked(void* userData) {
    TileButtonData* data = static_cast<TileButtonData*>(userData);
    if (data && data->palette) {
        data->palette->m_selectedType = data->tileType;

        char buf[128];
        sprintf_s(buf, "Tile selected: %d\n", static_cast<int>(data->tileType));
        OutputDebugStringA(buf);

        if (data->palette->m_callback) {
            data->palette->m_callback(data->tileType, data->palette->m_callbackData);
        }
    }
}

} // namespace UI
