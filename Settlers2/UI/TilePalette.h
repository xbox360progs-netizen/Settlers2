#pragma once
#include "Widget.h"
#include "Button.h"
#include "../World/TileType.h"
#include <vector>

namespace UI {

class TilePalette : public Widget {
public:
    TilePalette();
    virtual ~TilePalette();

    virtual void Update(float deltaTime) override;
    virtual void Render(IDirect3DDevice9* device) override;

    World::TileType GetSelectedTileType() const { return m_selectedType; }
    void SetSelectedTileType(World::TileType type);

    void SetTileSelectedCallback(void (*callback)(World::TileType, void*), void* userData) {
        m_callback = callback;
        m_callbackData = userData;
    }

    void CreateTileButtons(IDirect3DDevice9* device);
    void LayoutButtons();

private:
    static void OnTileButtonClicked(void* userData);

    struct TileButtonInfo {
        Button* button;
        World::TileType tileType;
    };

    std::vector<TileButtonInfo> m_tileButtons;
    World::TileType m_selectedType;
    int m_columns;
    float m_buttonSize;
    void (*m_callback)(World::TileType, void*);
    void* m_callbackData;
};

} // namespace UI
