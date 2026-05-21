#include "stdafx.h"
#include "TextManager.h"
#include "RenderQueue.h"
#include "RenderLayers.h"

static const float UV_PADDING = 0.5f / 512.0f;

TextManager::TextManager(BitmapFont* font, float screenWidth, float screenHeight, Graphics::RenderQueue* renderQueue)
: m_font(font), m_renderQueue(renderQueue),
  m_screenWidth(screenWidth), m_screenHeight(screenHeight)
{
}

TextManager::~TextManager()
{
    for (std::map<FontID, FontData>::iterator it = m_fontData.begin(); it != m_fontData.end(); ++it) {
        if (it->second.texture) {
            it->second.texture->Release();
        }
        if (it->second.glyphs) {
            delete[] it->second.glyphs;
        }
    }
    m_fontData.clear();
}

LPDIRECT3DTEXTURE9 TextManager::SetFontAtlas(FontID fontID, LPDIRECT3DTEXTURE9 texture)
{
    if (fontID < 0 || fontID >= FONT_COUNT) return nullptr;

    std::map<FontID, FontData>::iterator it = m_fontData.find(fontID);
    if (it != m_fontData.end()) {
        if (it->second.texture) {
            it->second.texture->Release();
        }
        if (it->second.glyphs) {
            delete[] it->second.glyphs;
            it->second.glyphs = nullptr;
        }
    }

    FontData& data = m_fontData[fontID];
    data.texture = texture;
    data.glyphs = nullptr;

    if (m_font) {
        data.lineHeight = m_font->GetLineHeight();
        data.baseLine = m_font->GetBaseLine();

        const std::vector<FontChar>& chars = m_font->GetChars();
        data.glyphCount = (int)chars.size();
        data.glyphs = new Glyph[data.glyphCount];

        for (size_t i = 0; i < chars.size(); ++i) {
            const FontChar& ch = chars[i];
            data.glyphs[i].u0 = ch.u0;
            data.glyphs[i].v0 = ch.v0;
            data.glyphs[i].u1 = ch.u1;
            data.glyphs[i].v1 = ch.v1;
            data.glyphs[i].width = ch.width;
            data.glyphs[i].height = ch.height;
            data.glyphs[i].xOffset = ch.xOffset;
            data.glyphs[i].yOffset = ch.yOffset;
            data.glyphs[i].xAdvance = ch.xAdvance;
        }
    }

    if (texture) {
        texture->AddRef();
    }

    return data.texture;
}

LPDIRECT3DTEXTURE9 TextManager::GetFontTexture(FontID fontID)
{
    if (fontID < 0 || fontID >= FONT_COUNT) return nullptr;

    std::map<FontID, FontData>::iterator it = m_fontData.find(fontID);
    if (it != m_fontData.end()) {
        return it->second.texture;
    }

    if (m_font) {
        return m_font->GetTexture();
    }

    return nullptr;
}

void TextManager::PushLetterCommand(LPDIRECT3DTEXTURE9 texture, float x, float y, float w, float h, float u0, float v0, float u1, float v1, D3DCOLOR color, float depth, bool isUI)
{
    if (!m_renderQueue) return;

    Graphics::RenderCommand cmd = {};
    cmd.x = x;
    cmd.y = y;
    cmd.width = w;
    cmd.height = h;
    cmd.u0 = u0;
    cmd.v0 = v0;
    cmd.u1 = u1;
    cmd.v1 = v1;
    cmd.color = color;
    cmd.shaderID = SHADER_SPRITE;
    cmd.textureID = 0;
    cmd.blendMode = 1;
    cmd.layer = isUI ? LAYER_UI : LAYER_FOREGROUND;
    cmd.depth = (WORD)(depth * 1000.0f);
    m_renderQueue->Submit(cmd);
}

void TextManager::DrawString(const std::string& text, float x, float y, D3DCOLOR color, float scale, FontID fontID, bool isUI, FontStyle style, float depth)
{
    if (!m_font || !m_renderQueue) return;

    LPDIRECT3DTEXTURE9 fontTexture = m_font->GetTexture();
    if (!fontTexture) return;

    float lineHeight = m_font->GetLineHeight() * scale;
    float penX = x;
    float penY = y;

    for (size_t i = 0; i < text.length(); ++i)
    {
        unsigned char c = (unsigned char)text[i];

        if (c == '\n') { penX = x; penY += lineHeight; continue; }
        if (c == '\r') continue;

        const std::vector<FontChar>& chars = m_font->GetChars();
        if (c >= chars.size()) continue;

        const FontChar& ch = chars[c];

        float charW = ch.width * scale;
        float charH = ch.height * scale;
        float charX = penX + ch.xOffset * scale;
        float charY = penY + ch.yOffset * scale;

        float u0 = ch.u0;
        float v0 = ch.v0;
        float u1 = ch.u1;
        float v1 = ch.v1;

        float uvPad = UV_PADDING;
        u0 += uvPad; v0 += uvPad;
        u1 -= uvPad; v1 -= uvPad;

        if (style == FONT_STYLE_SHADOW) {
            PushLetterCommand(fontTexture, charX + 2.0f, charY + 2.0f, charW, charH, u0, v0, u1, v1, 0xFF000000, depth, isUI);
        }

        PushLetterCommand(fontTexture, charX, charY, charW, charH, u0, v0, u1, v1, color, depth, isUI);

        penX += ch.xAdvance * scale;
    }
}

void TextManager::DrawTextToScreen(const std::string& text, float x, float y, D3DCOLOR color, float scale, FontID fontID, FontStyle style)
{
    DrawString(text, x, y, color, scale, fontID, true, style, 0.05f);
}

void TextManager::DrawTextToWorld(const std::string& text, float worldX, float worldY, D3DCOLOR color, float scale, FontID fontID, FontStyle style)
{
    DrawString(text, worldX, worldY, color, scale, fontID, false, style, 0.5f);
}
