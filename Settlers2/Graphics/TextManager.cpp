#include "stdafx.h"
#include "TextManager.h"
#include "Renderer.h"
#include "ShaderManager.h"
#include <d3dx9.h>

#define DEBUG_TEXT_SKIP_RENDER 0

TextManager::TextManager(BitmapFont* font, float screenWidth, float screenHeight)
: m_font(font), m_renderer(nullptr), m_shaderManager(nullptr), 
  m_screenWidth(screenWidth), m_screenHeight(screenHeight),
  m_screenVB(nullptr), m_screenVBCapacity(0)
{
    // Initialize font atlases to nullptr
    for (int i = 0; i < FONT_COUNT; ++i) {
        m_fontAtlases[i] = nullptr;
    }
}

TextManager::~TextManager()
{
    // Release font atlas references
    for (int i = 0; i < FONT_COUNT; ++i) {
        if (m_fontAtlases[i]) {
            m_fontAtlases[i]->Release();
            m_fontAtlases[i] = nullptr;
        }
    }
    
    // Release legacy vertex buffer
    if (m_screenVB) {
        m_screenVB->Release();
        m_screenVB = nullptr;
    }
}

void TextManager::Init(Renderer* renderer, ShaderManager* shaderManager)
{
    m_renderer = renderer;
    m_shaderManager = shaderManager;
}

void TextManager::SetFontAtlas(FontID fontID, LPDIRECT3DTEXTURE9 texture)
{
    if (fontID < 0 || fontID >= FONT_COUNT) return;
    
    // Release old texture reference
    if (m_fontAtlases[fontID]) {
        m_fontAtlases[fontID]->Release();
    }
    
    // Set new texture and add reference
    m_fontAtlases[fontID] = texture;
    if (texture) {
        texture->AddRef();
    }
}

void TextManager::DrawTextToScreen(const std::string& text, float x, float y, D3DCOLOR color, float scale, FontID fontID)
{
    if (!m_renderer || !m_shaderManager) {
        OutputDebugStringA("[TextManager] ERROR: Renderer or ShaderManager not set. Call Init() first.\n");
        return;
    }
    
    if (!m_font || text.empty() || scale <= 0) {
        return;
    }
    
    // Limit text length to prevent overflow
    if (text.length() > 1000) {
        return;
    }
    
    // Get font atlas texture
    LPDIRECT3DTEXTURE9 fontTexture = (fontID >= 0 && fontID < FONT_COUNT) ? m_fontAtlases[fontID] : nullptr;
    if (!fontTexture && m_font) {
        // Fallback to default font texture if available
        fontTexture = m_font->GetTexture();
    }
    
    if (!fontTexture) {
        OutputDebugStringA("[TextManager] WARNING: No font texture available\n");
        return;
    }
    
    ShaderManager* shaderManager = m_renderer->GetShaderManager();
    if (!shaderManager) return;
    
    float penX = x;
    float penY = y;
    
    const std::vector<FontChar>& chars = m_font->GetChars();
    if (chars.empty()) return;
    
    for (size_t i = 0; i < text.size(); ++i)
    {
        unsigned char c = (unsigned char)text[i];
        
        // New line handling
        if (c == '\n') { penX = x; penY += m_font->GetLineHeight() * scale; continue; }
        if (c == '\r') continue;
        if (c >= chars.size()) {
            continue;
        }
        
        const FontChar& ch = chars[c];
        if (ch.width <= 0.0f || ch.height <= 0.0f) { penX += ch.xAdvance * scale; continue; }
        
        float charX = penX + ch.xOffset * scale;
        float charY = penY + ch.yOffset * scale;
        float charW = ch.width * scale;
        float charH = ch.height * scale;
        
        // Create RenderCommand for this character
        ShaderManager::RenderCommand cmd;
        cmd.pTexture = fontTexture;
        cmd.shaderID = SHADER_UI;
        cmd.vertexStart = 0;
        cmd.vertexCount = 4;
        cmd.primitiveCount = 2;
        cmd.batchType = 0;
        cmd.depth = 0.05f; // Text layer (above UI background)
        cmd.layer = 2; // UI layer
        cmd.isUI = true; // Screen-space rendering
        cmd.u0 = ch.u0;
        cmd.v0 = ch.v0;
        cmd.u1 = ch.u1;
        cmd.v1 = ch.v1;
        cmd.screenX = charX;
        cmd.screenY = charY;
        cmd.screenW = charW;
        cmd.screenH = charH;
        cmd.color = color;
        cmd.customDraw = NULL;
        cmd.customUserData = NULL;
        
        shaderManager->Submit(cmd);
        
        // Advance pen position
        penX += ch.xAdvance * scale;
    }
}

void TextManager::DrawTextToWorld(const std::string& text, float worldX, float worldY, D3DCOLOR color, float scale, FontID fontID)
{
    if (!m_renderer || !m_shaderManager) {
        OutputDebugStringA("[TextManager] ERROR: Renderer or ShaderManager not set. Call Init() first.\n");
        return;
    }
    
    if (!m_font || text.empty() || scale <= 0) {
        return;
    }
    
    // Limit text length to prevent overflow
    if (text.length() > 1000) {
        return;
    }
    
    // Get font atlas texture
    LPDIRECT3DTEXTURE9 fontTexture = (fontID >= 0 && fontID < FONT_COUNT) ? m_fontAtlases[fontID] : nullptr;
    if (!fontTexture && m_font) {
        // Fallback to default font texture if available
        fontTexture = m_font->GetTexture();
    }
    
    if (!fontTexture) {
        OutputDebugStringA("[TextManager] WARNING: No font texture available\n");
        return;
    }
    
    ShaderManager* shaderManager = m_renderer->GetShaderManager();
    if (!shaderManager) return;
    
    float penX = worldX;
    float penY = worldY;
    
    const std::vector<FontChar>& chars = m_font->GetChars();
    if (chars.empty()) return;
    
    for (size_t i = 0; i < text.size(); ++i)
    {
        unsigned char c = (unsigned char)text[i];
        
        // New line handling
        if (c == '\n') { penX = worldX; penY += m_font->GetLineHeight() * scale; continue; }
        if (c == '\r') continue;
        if (c >= chars.size()) {
            continue;
        }
        
        const FontChar& ch = chars[c];
        if (ch.width <= 0.0f || ch.height <= 0.0f) { penX += ch.xAdvance * scale; continue; }
        
        float charX = penX + ch.xOffset * scale;
        float charY = penY + ch.yOffset * scale;
        float charW = ch.width * scale;
        float charH = ch.height * scale;
        
        // Create RenderCommand for this character (world-space)
        ShaderManager::RenderCommand cmd;
        cmd.pTexture = fontTexture;
        cmd.shaderID = SHADER_SPRITE; // Use sprite shader for world-space text
        cmd.vertexStart = 0;
        cmd.vertexCount = 4;
        cmd.primitiveCount = 2;
        cmd.batchType = 0;
        cmd.depth = 0.5f; // Mid-layer for world text
        cmd.layer = 1; // Objects layer
        cmd.isUI = false; // World-space rendering (uses camera matrix)
        cmd.u0 = ch.u0;
        cmd.v0 = ch.v0;
        cmd.u1 = ch.u1;
        cmd.v1 = ch.v1;
        cmd.worldX = charX;
        cmd.worldY = charY;
        cmd.color = color;
        cmd.customDraw = NULL;
        cmd.customUserData = NULL;
        
        shaderManager->Submit(cmd);
        
        // Advance pen position
        penX += ch.xAdvance * scale;
    }
}

// Legacy methods for backward compatibility (will be deprecated)
void TextManager::Begin()
{
    m_screenVertices.clear();
    m_worldVertices.clear();
}

void TextManager::AddTextScreen(const std::string& text, const D3DXVECTOR3& pos, D3DCOLOR color, float scale)
{
    if (!m_font || text.empty() || scale <= 0) {
        return;
    }

    if (text.length() > 1000) {
        return;
    }

    float penX = pos.x;
    float penY = pos.y;

    const std::vector<FontChar>& chars = m_font->GetChars();
    if (chars.empty()) return;
    
    for (size_t i = 0; i < text.size(); ++i)
    {
        unsigned char c = (unsigned char)text[i];
        if (c == '\n') { penX = pos.x; penY += m_font->GetLineHeight() * scale; continue; }
        if (c == '\r') continue;
        if (c >= chars.size()) {
            continue;
        }
        
        if (m_screenVertices.size() + 6 > 49990) {
            break;
        }
        
        const FontChar& ch = chars[c];
        if (ch.width <= 0.0f || ch.height <= 0.0f) { penX += ch.xAdvance * scale; continue; }

        float x0 = penX + ch.xOffset * scale;
        float y0 = penY + ch.yOffset * scale;
        float x1 = penX + (ch.xOffset + ch.width) * scale;
        float y1 = penY + (ch.yOffset + ch.height) * scale;

        TextVertex v0(D3DXVECTOR3(x0, y0, pos.z), color, D3DXVECTOR2(ch.u0, ch.v0));
        TextVertex v1(D3DXVECTOR3(x1, y0, pos.z), color, D3DXVECTOR2(ch.u1, ch.v0));
        TextVertex v2(D3DXVECTOR3(x0, y1, pos.z), color, D3DXVECTOR2(ch.u0, ch.v1));
        TextVertex v3(D3DXVECTOR3(x1, y1, pos.z), color, D3DXVECTOR2(ch.u1, ch.v1));

        m_screenVertices.push_back(v0);
        m_screenVertices.push_back(v1);
        m_screenVertices.push_back(v2);
        m_screenVertices.push_back(v1);
        m_screenVertices.push_back(v3);
        m_screenVertices.push_back(v2);

        penX += ch.xAdvance * scale;
    }
}

void TextManager::RenderScreen()
{
    // Legacy method - no-op with queue-based rendering
    // Text is now rendered via DrawTextToScreen which submits to the queue
}

void TextManager::RenderWorld(Camera* camera)
{
    // Legacy method - no-op with queue-based rendering
    // Text is now rendered via DrawTextToWorld which submits to the queue
}

void TextManager::EnsureScreenVB(size_t vertexCount)
{
    // Legacy method - no-op with queue-based rendering
}
