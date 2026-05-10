// Graphics/BitmapText.cpp
#include "stdafx.h"
#include "BitmapText.h"
#include "BitmapFont.h"
#include "TextVertex.h"
#include <d3dx9math.h>
#include <sstream>
#include <iomanip>
void BitmapText::UpdateVertices(std::vector<TextVertex>& outVertices) const
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

        TextVertex v0; v0.pos = D3DXVECTOR3(x0, y0, 0.0f); v0.col = color; v0.uv = D3DXVECTOR2(ch.u0, ch.v0);
        TextVertex v1; v1.pos = D3DXVECTOR3(x1, y0, 0.0f); v1.col = color; v1.uv = D3DXVECTOR2(ch.u1, ch.v0);
        TextVertex v2; v2.pos = D3DXVECTOR3(x0, y1, 0.0f); v2.col = color; v2.uv = D3DXVECTOR2(ch.u0, ch.v1);
        TextVertex v3; v3.pos = D3DXVECTOR3(x1, y1, 0.0f); v3.col = color; v3.uv = D3DXVECTOR2(ch.u1, ch.v1);

        outVertices.push_back(v0);
        outVertices.push_back(v1);
        outVertices.push_back(v2);
        outVertices.push_back(v1);
        outVertices.push_back(v2);
        outVertices.push_back(v3);

        penX += ch.xAdvance * m_scale;
    }
}
