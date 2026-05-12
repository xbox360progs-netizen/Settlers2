// Graphics/BitmapFont.h
#ifndef BITMAPFONT_H
#define BITMAPFONT_H

#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>
#include "Renderer.h" // For SpriteVertex

// Forward declarations
class Renderer;
class ShaderManager;

// ==========================================================
// Structure for font character data
// ==========================================================
struct FontChar
{
    float u0, v0;    // Top-left UV
    float u1, v1;    // Bottom-right UV
    float width;     // Character width in pixels
    float height;    // Character height in pixels
    float xOffset;   // Character X offset
    float yOffset;   // Character Y offset
    float xAdvance;  // Pen advance for next character
};

// ==========================================================
// Font space (screen vs world)
// ==========================================================
enum FontSpace
{
    FONT_SPACE_SCREEN = 0, // Screen coordinates
    FONT_SPACE_WORLD       // World coordinates
};

// ==========================================================
// BitmapFont class
// ==========================================================
class BitmapFont
{
public:
    explicit BitmapFont(LPDIRECT3DDEVICE9 device);
    ~BitmapFont();
    
    float GetBaseLine() const { return m_baseLine; }
    
    // Initialize with renderer and shader manager for queue-based rendering
    void Init(Renderer* renderer, ShaderManager* shaderManager);
    
    // Legacy Initialize method (deprecated - use Init instead)
    bool Initialize();
    
    // Load font from .fnt and texture
    bool LoadFromFile(const wchar_t* fontDefinitionFile);
    
    // CPU: Build vertices for text
    void Begin(); // Clears m_vertices
    void DrawText(const std::string& text, DWORD color);
    void DrawText(const std::string& text); // Default white color
    
    // GPU: Render text (legacy method - deprecated, use TextManager instead)
    void Render(
        const D3DXVECTOR3& origin,
        const D3DXMATRIX& view,
        const D3DXMATRIX& proj,
        float scale,
        bool mirrorX = false,
        bool mirrorY = false
    );
    
    // Get device and texture
    LPDIRECT3DDEVICE9 GetDevice() const { return m_device; }
    IDirect3DTexture9* GetTexture() const { return m_texture; }
    
    // CPU vertex management (for TextManager)
    void SetVertices(const std::vector<SpriteVertex>& vertices) { m_vertices = vertices; }
    const std::vector<SpriteVertex>& GetVertices() const { return m_vertices; }
    
    // Access to character array
    const std::vector<FontChar>& GetChars() const { return m_chars; }
    
    // Line height from .fnt
    float GetLineHeight() const { return m_lineHeight; }

private:
    // Create/expand VertexBuffer
    void EnsureVB(size_t vertexCount);
    
    // Load texture from memory
    bool LoadTextureFromMemory(const wchar_t* texturePath);

private:
    // ======================================================
    // D3D objects
    // ======================================================
    LPDIRECT3DDEVICE9 m_device;
    LPDIRECT3DTEXTURE9 m_texture;
    LPDIRECT3DVERTEXBUFFER9 m_vb;
    UINT m_vbCapacity;
    
    // ======================================================
    // Rendering system integration
    // ======================================================
    Renderer* m_renderer;
    ShaderManager* m_shaderManager;
    
    // ======================================================
    // CPU vertex buffer
    // ======================================================
    std::vector<SpriteVertex> m_vertices; // Local / world vertices
    
    // ======================================================
    // Font data
    // ======================================================
    std::vector<FontChar> m_chars;
    float m_lineHeight;
    float m_textureWidth;
    float m_textureHeight;
    float m_baseLine;
};

#endif // BITMAPFONT_H
