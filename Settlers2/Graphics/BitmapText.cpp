// Graphics/BitmapText.cpp
#include "stdafx.h"
#include "BitmapText.h"
#include "BitmapFont.h"
#include "Renderer.h" // For SpriteVertex
#include <d3dx9math.h>
#include <sstream>
#include <iomanip>
void BitmapText::UpdateVertices(std::vector<SpriteVertex>& outVertices) const
{
    if (!m_font) return;

    const auto& chars = m_font->GetChars();
    float penX = m_x;
    float penY = m_y;

    DWORD color = m_color; 

    for (size_t i = 0; i < m_text.size(); ++i)
    {
        unsigned char c = (unsigned char)m_text[i];
        if (c >= chars.size()) continue;

        const FontChar& ch = chars[c];

        float x0 = penX + ch.xOffset * m_scale;
        float y0 = penY + ch.yOffset * m_scale;
        float x1 = x0 + ch.width * m_scale;
        float y1 = y0 + ch.height * m_scale;

        SpriteVertex v0; v0.x = x0; v0.y = y0; v0.z = 0.0f; v0.color = color; v0.u = ch.u0; v0.v = ch.v0;
        SpriteVertex v1; v1.x = x1; v1.y = y0; v1.z = 0.0f; v1.color = color; v1.u = ch.u1; v1.v = ch.v0;
        SpriteVertex v2; v2.x = x0; v2.y = y1; v2.z = 0.0f; v2.color = color; v2.u = ch.u0; v2.v = ch.v1;
        SpriteVertex v3; v3.x = x1; v3.y = y1; v3.z = 0.0f; v3.color = color; v3.u = ch.u1; v3.v = ch.v1;

        outVertices.push_back(v0);
        outVertices.push_back(v1);
        outVertices.push_back(v2);
        outVertices.push_back(v1);
        outVertices.push_back(v2);
        outVertices.push_back(v3);

        penX += ch.xAdvance * m_scale;
    }
}
