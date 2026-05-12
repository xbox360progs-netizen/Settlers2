// Graphics/TextManager.h
#ifndef TEXTMANAGER_H  
#define TEXTMANAGER_H  

#pragma once
#include "BitmapFont.h"
#include "Renderer.h" // For SpriteVertex
#include <vector>
#include <string>
#include <map>

class Renderer;
class ShaderManager;
class SpriteRenderer;

// Font identifiers for atlas management
enum FontID {
    FONT_MENU = 0,
    FONT_CHAT,
    FONT_DEBUG,
    FONT_COUNT
};

// Font style for text rendering
enum FontStyle {
    FONT_STYLE_NORMAL = 0,
    FONT_STYLE_SHADOW,
    FONT_STYLE_OUTLINE
};

// Glyph structure for character UV coordinates in atlas
struct Glyph {
    float u0, v0, u1, v1;  // UV coordinates in atlas
    float width, height;   // Character dimensions
    float xOffset, yOffset; // Offset from cursor position
    float xAdvance;       // Advance to next character
};

// Font data structure for atlas management
struct FontData {
    LPDIRECT3DTEXTURE9 texture;
    float lineHeight;
    float baseLine;
    Glyph* glyphs; // Raw pointer to glyph array
    int glyphCount; // Number of glyphs in array
};

class TextManager
{
public:
    TextManager(BitmapFont* font, float screenWidth, float screenHeight, SpriteRenderer* spriteRenderer);
    ~TextManager();
    
    // Initialize with renderer and shader manager for queue-based rendering
    void Init(Renderer* renderer, ShaderManager* shaderManager);
    
    // Set font atlas texture for a specific font ID
    LPDIRECT3DTEXTURE9 SetFontAtlas(FontID fontID, LPDIRECT3DTEXTURE9 texture);
    
    // Get font atlas texture for a specific font ID
    LPDIRECT3DTEXTURE9 GetFontTexture(FontID fontID);
    
    // Unified draw method with explicit isUI flag
    void DrawString(const std::string& text, float x, float y, D3DCOLOR color = 0xFFFFFFFF, float scale = 0.10f, FontID fontID = FONT_MENU, bool isUI = true, FontStyle style = FONT_STYLE_NORMAL, float depth = 0.05f);
    
    // Convenience methods
    void DrawTextToScreen(const std::string& text, float x, float y, D3DCOLOR color = 0xFFFFFFFF, float scale = 0.10f, FontID fontID = FONT_MENU, FontStyle style = FONT_STYLE_NORMAL);
    void DrawTextToWorld(const std::string& text, float worldX, float worldY, D3DCOLOR color = 0xFFFFFFFF, float scale = 0.1f, FontID fontID = FONT_MENU, FontStyle style = FONT_STYLE_NORMAL);
    
    // Legacy methods for backward compatibility (will be deprecated)
    void Begin();
    void RenderScreen();
    void RenderWorld(class Camera* camera);
    void AddTextScreen(const std::string& text, const D3DXVECTOR3& pos, D3DCOLOR color = 0xFFFFFFFF, float scale = 0.10f);

private:
    BitmapFont* m_font;
    Renderer* m_renderer;
    ShaderManager* m_shaderManager;
    SpriteRenderer* m_spriteRenderer;
    
    float m_screenWidth;
    float m_screenHeight;
    
    // Font data management
    std::map<FontID, FontData> m_fontData;
    
    // Legacy vertex buffer (for backward compatibility)
    std::vector<SpriteVertex> m_screenVertices;
    std::vector<SpriteVertex> m_worldVertices;
    LPDIRECT3DVERTEXBUFFER9 m_screenVB;
    UINT m_screenVBCapacity;
    
    // Helper to push a single character command
    void PushLetterCommand(const Glyph& glyph, LPDIRECT3DTEXTURE9 texture, float x, float y, float w, float h, D3DCOLOR color, float depth, bool isUI, int shaderID, int letterCount);
    
    void EnsureScreenVB(size_t vertexCount);
};

#endif // TEXTMANAGER_H
