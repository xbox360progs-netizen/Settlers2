// Graphics/BitmapText.h
#pragma once
#include "Renderer.h" // For SpriteVertex
#include <vector>
#include <string>

class BitmapFont;

class BitmapText
{
public:
    BitmapText(BitmapFont* font, const std::string& text, float x, float y, DWORD color = 0xFFFFFFFF, float scale = 1.0f);
    ~BitmapText();
    
    void SetText(const std::string& text) { m_text = text; }
    void SetPosition(float x, float y) { m_x = x; m_y = y; }
    void SetColor(DWORD color) { m_color = color; }
    void SetScale(float scale) { m_scale = scale; }
    
    void UpdateVertices(std::vector<SpriteVertex>& outVertices) const;
    void SetWorldMatrix(const D3DXMATRIX& world) { m_world = world; }
    const D3DXMATRIX& GetWorldMatrix() const { return m_world; }

private:
    BitmapFont* m_font;
    std::string m_text;
    float m_x, m_y;
    DWORD m_color;
    float m_scale;
    D3DXMATRIX m_world;
};
