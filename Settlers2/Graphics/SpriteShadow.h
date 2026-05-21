#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>

namespace Graphics {

struct SpriteShadowData {
    float x;
    float y;
    float width;
    float height;
    float offsetX;
    float offsetY;
    DWORD color;

    SpriteShadowData()
        : x(0), y(0), width(0), height(0),
          offsetX(0), offsetY(0), color(0x40000000) {}
};

class SpriteShadowSystem {
public:
    SpriteShadowSystem();
    ~SpriteShadowSystem();

    void Initialize(IDirect3DDevice9* pDevice);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void AddSpriteShadow(const SpriteShadowData& shadow);
    void ClearShadowList();

    void Render();

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void SetDefaultColor(DWORD color) { m_defaultColor = color; }
    DWORD GetDefaultColor() const { return m_defaultColor; }

private:
    IDirect3DDevice9* m_pDevice;
    bool m_enabled;
    DWORD m_defaultColor;

    std::vector<SpriteShadowData> m_shadows;
};

}
