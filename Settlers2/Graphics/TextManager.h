// Graphics/TextManager.h
#ifndef TEXTMANAGER_H  
#define TEXTMANAGER_H  

#pragma once
#include "BitmapFont.h"
#include "RenderTypes.h"
#include <vector>
#include <string>
#include <map>

namespace Graphics { class RenderQueue; }

enum FontID {
    FONT_MENU = 0,
    FONT_CHAT,
    FONT_DEBUG,
    FONT_COUNT
};

enum FontStyle {
    FONT_STYLE_NORMAL = 0,
    FONT_STYLE_SHADOW,
    FONT_STYLE_OUTLINE
};

struct Glyph {
    float u0, v0, u1, v1;
    float width, height;
    float xOffset, yOffset;
    float xAdvance;
};

struct FontData {
    LPDIRECT3DTEXTURE9 texture;
    float lineHeight;
    float baseLine;
    Glyph* glyphs;
    int glyphCount;
};

class TextManager
{
public:
    TextManager(BitmapFont* font, float screenWidth, float screenHeight, Graphics::RenderQueue* renderQueue);
    ~TextManager();

    void SetRenderQueue(Graphics::RenderQueue* renderQueue) { m_renderQueue = renderQueue; }

    LPDIRECT3DTEXTURE9 SetFontAtlas(FontID fontID, LPDIRECT3DTEXTURE9 texture);
    LPDIRECT3DTEXTURE9 GetFontTexture(FontID fontID);

    void DrawString(const std::string& text, float x, float y, D3DCOLOR color = 0xFFFFFFFF, float scale = 0.10f, FontID fontID = FONT_MENU, bool isUI = true, FontStyle style = FONT_STYLE_NORMAL, float depth = 0.05f);

    void DrawTextToScreen(const std::string& text, float x, float y, D3DCOLOR color = 0xFFFFFFFF, float scale = 0.10f, FontID fontID = FONT_MENU, FontStyle style = FONT_STYLE_NORMAL);
    void DrawTextToWorld(const std::string& text, float worldX, float worldY, D3DCOLOR color = 0xFFFFFFFF, float scale = 0.1f, FontID fontID = FONT_MENU, FontStyle style = FONT_STYLE_NORMAL);

private:
    void PushLetterCommand(LPDIRECT3DTEXTURE9 texture, float x, float y, float w, float h, float u0, float v0, float u1, float v1, D3DCOLOR color, float depth, bool isUI);

    BitmapFont* m_font;
    Graphics::RenderQueue* m_renderQueue;

    float m_screenWidth;
    float m_screenHeight;

    std::map<FontID, FontData> m_fontData;
};

#endif
