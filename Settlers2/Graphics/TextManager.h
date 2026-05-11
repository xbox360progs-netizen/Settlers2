// Graphics/TextManager.h
#ifndef TEXTMANAGER_H  
#define TEXTMANAGER_H  

#pragma once
#include "BitmapFont.h"
#include <vector>
#include <string>

class Renderer;
class ShaderManager;

// Font identifiers for atlas management
enum FontID {
    FONT_MENU = 0,
    FONT_CHAT,
    FONT_DEBUG,
    FONT_COUNT
};

// Glyph structure for character UV coordinates in atlas
struct Glyph {
    float u0, v0, u1, v1;  // UV coordinates in atlas
    float width, height;   // Character dimensions
    float xOffset, yOffset; // Offset from cursor position
    float xAdvance;       // Advance to next character
};

class TextManager
{
public:
    TextManager(BitmapFont* font, float screenWidth, float screenHeight);
    ~TextManager();
    
    // Initialize with renderer and shader manager for queue-based rendering
    void Init(Renderer* renderer, ShaderManager* shaderManager);
    
    // Set font atlas texture for a specific font ID
    void SetFontAtlas(FontID fontID, LPDIRECT3DTEXTURE9 texture);
    
    // Draw text to screen space (submits RenderCommand to queue)
    void DrawTextToScreen(const std::string& text, float x, float y, D3DCOLOR color = 0xFFFFFFFF, float scale = 0.10f, FontID fontID = FONT_MENU);
    
    // Draw text to world space (submits RenderCommand to queue)
    void DrawTextToWorld(const std::string& text, float worldX, float worldY, D3DCOLOR color = 0xFFFFFFFF, float scale = 0.1f, FontID fontID = FONT_MENU);
    
    // Legacy methods for backward compatibility (will be deprecated)
    void Begin();
    void RenderScreen();
    void RenderWorld(class Camera* camera);
    void AddTextScreen(const std::string& text, const D3DXVECTOR3& pos, D3DCOLOR color = 0xFFFFFFFF, float scale = 0.10f);

private:
    BitmapFont* m_font;
    Renderer* m_renderer;
    ShaderManager* m_shaderManager;
    
    float m_screenWidth;
    float m_screenHeight;
    
    // Font atlas textures
    LPDIRECT3DTEXTURE9 m_fontAtlases[FONT_COUNT];
    
    // Legacy vertex buffer (for backward compatibility)
    std::vector<TextVertex> m_screenVertices;
    std::vector<TextVertex> m_worldVertices;
    LPDIRECT3DVERTEXBUFFER9 m_screenVB;
    UINT m_screenVBCapacity;
    
    void EnsureScreenVB(size_t vertexCount);
};

#endif // TEXTMANAGER_H
