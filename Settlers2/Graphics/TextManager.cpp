#include "stdafx.h"
#include "TextManager.h"
#include "Renderer.h"
#include "ShaderManager.h"
#include <d3dx9.h>

#define DEBUG_TEXT_SKIP_RENDER 0

// Half-pixel UV padding for Xbox 360 to prevent texture bleeding
static const float UV_PADDING = 0.5f / 512.0f; // Assumes 512x512 atlas, adjust as needed

TextManager::TextManager(BitmapFont* font, float screenWidth, float screenHeight)
: m_font(font), m_renderer(nullptr), m_shaderManager(nullptr), 
  m_screenWidth(screenWidth), m_screenHeight(screenHeight),
  m_screenVB(nullptr), m_screenVBCapacity(0)
{
}

TextManager::~TextManager()
{
    // Release font atlas references and glyph data
    for (std::map<FontID, FontData>::iterator it = m_fontData.begin(); it != m_fontData.end(); ++it) {
        if (it->second.texture) {
            it->second.texture->Release();
        }
        if (it->second.glyphs) {
            delete[] it->second.glyphs;
        }
    }
    m_fontData.clear();
    
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
    
    // Release old texture and glyph data if exists
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
    
    // Create or update font data
    FontData& data = m_fontData[fontID];
    data.texture = texture;
    data.glyphs = nullptr;
    data.glyphCount = 0;
    if (texture) {
        texture->AddRef();
    }
    
    // Copy font metrics from BitmapFont if available
    if (m_font) {
        data.lineHeight = m_font->GetLineHeight();
        data.baseLine = m_font->GetBaseLine();
        
        // Build glyph data from font characters
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
}

void TextManager::PushLetterCommand(const Glyph& glyph, LPDIRECT3DTEXTURE9 texture, float x, float y, float w, float h, D3DCOLOR color, float depth, bool isUI, int shaderID)
{
    if (!m_renderer || !m_shaderManager) return;
    
    static int letterCount = 0;
    char debugBuf[256];
    sprintf(debugBuf, "[TextManager::PushLetterCommand] Letter %d: pos=(%.1f,%.1f) size=(%.1f,%.1f) shaderID=%d\n",
            ++letterCount, x, y, w, h, shaderID);
    OutputDebugStringA(debugBuf);
    
    ShaderManager* shaderManager = m_renderer->GetShaderManager();
    if (!shaderManager) return;
    
    // Apply UV padding to prevent bleeding on Xbox 360
    float u0 = glyph.u0 + UV_PADDING;
    float v0 = glyph.v0 + UV_PADDING;
    float u1 = glyph.u1 - UV_PADDING;
    float v1 = glyph.v1 - UV_PADDING;
    
    // Clamp UVs to valid range
    u0 = max(0.0f, min(1.0f, u0));
    v0 = max(0.0f, min(1.0f, v0));
    u1 = max(0.0f, min(1.0f, u1));
    v1 = max(0.0f, min(1.0f, v1));
    
    ShaderManager::RenderCommand cmd;
    cmd.pTexture = texture;
    cmd.shaderID = shaderID;
    cmd.vertexStart = 0;
    cmd.vertexCount = 4;
    cmd.primitiveCount = 2;
    cmd.batchType = 0;
    cmd.depth = depth; // CRITICAL: All letters in a string must have identical depth for batching
    cmd.layer = isUI ? 2 : 1; // UI layer or Objects layer
    cmd.isUI = isUI;
    cmd.u0 = u0;
    cmd.v0 = v0;
    cmd.u1 = u1;
    cmd.v1 = v1;
    cmd.screenX = x;
    cmd.screenY = y;
    cmd.screenW = w;
    cmd.screenH = h;
    cmd.worldX = x;
    cmd.worldY = y;
    cmd.color = color;
    cmd.customDraw = NULL;
    cmd.customUserData = NULL;
    
    shaderManager->Submit(cmd);
}

void TextManager::DrawString(const std::string& text, float x, float y, D3DCOLOR color, float scale, FontID fontID, bool isUI, FontStyle style, float depth)
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
    
    // Get font data
    std::map<FontID, FontData>::iterator it = m_fontData.find(fontID);
    LPDIRECT3DTEXTURE9 fontTexture = nullptr;
    const Glyph* glyphs = nullptr;
    int glyphCount = 0;
    float lineHeight = m_font->GetLineHeight();
    
    if (it != m_fontData.end()) {
        fontTexture = it->second.texture;
        glyphs = it->second.glyphs;
        glyphCount = it->second.glyphCount;
        lineHeight = it->second.lineHeight;
    }
    
    // Fallback to default font texture if available
    if (!fontTexture && m_font) {
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
    
    // Shadow offset
    const float SHADOW_OFFSET = 1.0f;
    D3DCOLOR shadowColor = D3DCOLOR_ARGB(255, 0, 0, 0); // Black shadow
    
    int shaderID = isUI ? SHADER_FONT : SHADER_WORLD;
    
    for (size_t i = 0; i < text.size(); ++i)
    {
        unsigned char c = (unsigned char)text[i];
        
        // New line handling
        if (c == '\n') { penX = x; penY += lineHeight * scale; continue; }
        if (c == '\r') continue;
        
        // Get glyph data
        Glyph glyph;
        bool hasGlyph = false;
        
        if (glyphs && c < (size_t)glyphCount) {
            glyph = glyphs[c];
            hasGlyph = true;
        } else if (m_font) {
            const std::vector<FontChar>& chars = m_font->GetChars();
            if (c < chars.size()) {
                const FontChar& ch = chars[c];
                glyph.u0 = ch.u0;
                glyph.v0 = ch.v0;
                glyph.u1 = ch.u1;
                glyph.v1 = ch.v1;
                glyph.width = ch.width;
                glyph.height = ch.height;
                glyph.xOffset = ch.xOffset;
                glyph.yOffset = ch.yOffset;
                glyph.xAdvance = ch.xAdvance;
                hasGlyph = true;
            }
        }
        
        if (!hasGlyph || glyph.width <= 0.0f || glyph.height <= 0.0f) { 
            penX += glyph.xAdvance * scale; 
            continue; 
        }
        
        float charX = penX + glyph.xOffset * scale;
        float charY = penY + glyph.yOffset * scale;
        float charW = glyph.width * scale;
        float charH = glyph.height * scale;
        
        // Shadow rendering (draw shadow first at slightly higher depth)
        if (style == FONT_STYLE_SHADOW) {
            PushLetterCommand(glyph, fontTexture, charX + SHADOW_OFFSET, charY + SHADOW_OFFSET, charW, charH, shadowColor, depth + 0.0001f, isUI, shaderID);
        }
        
        // Main character rendering (all letters use IDENTICAL depth for batching optimization)
        PushLetterCommand(glyph, fontTexture, charX, charY, charW, charH, color, depth, isUI, shaderID);
        
        // Advance pen position
        penX += glyph.xAdvance * scale;
    }
}

void TextManager::DrawTextToScreen(const std::string& text, float x, float y, D3DCOLOR color, float scale, FontID fontID, FontStyle style)
{
    char debugBuf[256];
    sprintf(debugBuf, "[TextManager::DrawTextToScreen] Drawing '%s' at (%.1f,%.1f) with color=0x%08X\n", 
            text.c_str(), x, y, color);
    OutputDebugStringA(debugBuf);
    
    DrawString(text, x, y, color, scale, fontID, true, style, 0.05f);
    
    sprintf(debugBuf, "[TextManager::DrawTextToScreen] Completed drawing '%s'\n", text.c_str());
    OutputDebugStringA(debugBuf);
}

void TextManager::DrawTextToWorld(const std::string& text, float worldX, float worldY, D3DCOLOR color, float scale, FontID fontID, FontStyle style)
{
    DrawString(text, worldX, worldY, color, scale, fontID, false, style, 0.5f);
}

void TextManager::Begin()
{
    m_screenVertices.clear();
    
    if (m_renderer && m_renderer->GetShaderManager()) {
        m_renderer->GetShaderManager()->ClearQueue();
    }
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

        // Пример корректного заполнения одной вершины буквы
        SpriteVertex v0, v1, v2, v3;
        v0.x = x0; v0.y = y0; v0.z = pos.z; // Используйте глубину (Z) для сортировки
        v0.color = color;                          // Передавайте DWORD цвет
        v0.u = ch.u0; v0.v = ch.v0;                // Координаты из атласа шрифта
        
        v1.x = x1; v1.y = y0; v1.z = pos.z;
        v1.color = color;
        v1.u = ch.u1; v1.v = ch.v0;
        
        v2.x = x0; v2.y = y1; v2.z = pos.z;
        v2.color = color;
        v2.u = ch.u0; v2.v = ch.v1;
        
        v3.x = x1; v3.y = y1; v3.z = pos.z;
        v3.color = color;
        v3.u = ch.u1; v3.v = ch.v1;

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
    // XBOX 360 CRITICAL: Copy text vertices to shared buffer like SpriteRenderer
    if (!m_screenVertices.empty() && m_renderer && m_renderer->GetShaderManager()) {
        char debugBuf[256];
        sprintf(debugBuf, "[TextManager::RenderScreen] Copying %zu text vertices to buffer\n", m_screenVertices.size());
        OutputDebugStringA(debugBuf);
        
        // Get the shared vertex buffer from SpriteRenderer
        ShaderManager* shaderManager = m_renderer->GetShaderManager();
        LPDIRECT3DVERTEXBUFFER9 pVB = shaderManager->GetSharedVertexBuffer();
        
        if (pVB) {
            void* pData = NULL;
            HRESULT hr = pVB->Lock(0, m_screenVertices.size() * sizeof(TextVertex), &pData, 0);
            if (SUCCEEDED(hr)) {
                memcpy(pData, m_screenVertices.data(), m_screenVertices.size() * sizeof(TextVertex));
                pVB->Unlock();
                OutputDebugStringA("[TextManager::RenderScreen] Text vertices copied to shared buffer\n");
            } else {
                OutputDebugStringA("[TextManager::RenderScreen] ERROR: Failed to lock shared vertex buffer\n");
            }
        }
    }
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
