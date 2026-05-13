#include "stdafx.h"
#include <d3d9.h>
#include "TextManager.h"
#include "Renderer.h"
#include "ShaderManager.h"
#include "SpriteRenderer.h"
#include <d3dx9.h>

#define DEBUG_TEXT_SKIP_RENDER 0

// Half-pixel UV padding for Xbox 360 to prevent texture bleeding
static const float UV_PADDING = 0.5f / 512.0f; // Assumes 512x512 atlas, adjust as needed

TextManager::TextManager(BitmapFont* font, float screenWidth, float screenHeight, SpriteRenderer* spriteRenderer)
: m_font(font), m_renderer(nullptr), m_shaderManager(nullptr), m_spriteRenderer(spriteRenderer),
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

LPDIRECT3DTEXTURE9 TextManager::SetFontAtlas(FontID fontID, LPDIRECT3DTEXTURE9 texture)
{
    if (fontID < 0 || fontID >= FONT_COUNT) return nullptr;
    
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
    
    // Fallback to default font texture if available
    if (m_font) {
        return m_font->GetTexture();
    }
    
    return nullptr;
}

void TextManager::PushLetterCommand(const Glyph& glyph, LPDIRECT3DTEXTURE9 texture, float x, float y, float w, float h, D3DCOLOR color, float depth, bool isUI, int shaderID, int letterCount)
{
    if (!m_spriteRenderer) return;
    
//    char debugBuf[256];
//    sprintf(debugBuf, "[TextManager::PushLetterCommand] Letter %d: pos=(%.1f,%.1f) size=(%.1f,%.1f) using SpriteRenderer\n",
//            letterCount, x, y, w, h);
//    OutputDebugStringA(debugBuf);
    
    // Apply UV padding to prevent bleeding on Xbox 360
    float u0 = glyph.u0 + UV_PADDING;
    float v0 = glyph.v0 + UV_PADDING;
    float u1 = glyph.u1 - UV_PADDING;
    float v1 = glyph.v1 - UV_PADDING;
    
    // Normalize UV coordinates by texture dimensions
    u0 = u0 / m_font->GetTextureWidth();
    v0 = v0 / m_font->GetTextureHeight();
    u1 = u1 / m_font->GetTextureWidth();
    v1 = v1 / m_font->GetTextureHeight();
    
    // Clamp UVs to valid range
    u0 = max(0.0f, min(1.0f, u0));
    v0 = max(0.0f, min(1.0f, v0));
    u1 = max(0.0f, min(1.0f, u1));
    v1 = max(0.0f, min(1.0f, v1));
    
    // Use SpriteRenderer->DrawWithTexture instead of RenderCommand
//    char debugBuf2[512];
//    sprintf(debugBuf2, "[TextManager::PushLetterCommand] Drawing: pos=(%.1f,%.1f) size=(%.1f,%.1f) uv=(%.3f,%.3f,%.3f,%.3f) color=0x%08X texture=%p\n",
//            x, y, w, h, u0, v0, u1, v1, color, texture);
//    OutputDebugStringA(debugBuf2);
//    m_spriteRenderer->DrawWithTexture(x, y, w, h, u0, v0, u1, v1, texture, color);
}

void TextManager::DrawString(const std::string& text, float x, float y, D3DCOLOR color, float scale, FontID fontID, bool isUI, FontStyle style, float depth)
{
//    char dbg[256];
//    sprintf(dbg, "[TextManager::DrawString] ENTRY text='%s' x=%.1f y=%.1f m_font=%p m_spriteRenderer=%p\n",
//            text.c_str(), x, y, m_font, m_spriteRenderer);
//    OutputDebugStringA(dbg);

    if (!m_font) return;
    
    // Shadow offset
    D3DCOLOR shadowColor = D3DCOLOR_ARGB(255, 0, 0, 0); // Black shadow
    
    // Get font texture
    LPDIRECT3DTEXTURE9 fontTexture = m_font->GetTexture();
    if (!fontTexture) return;
    
    // Calculate line height
    float lineHeight = m_font->GetLineHeight() * scale;
    float baseDepth = depth;
    
    // Starting position
    float penX = x;
    float penY = y;
    
    // Limit text length to prevent overflow
    if (text.length() > 1000) {
        return;
    }
    
    // Get font data
    std::map<FontID, FontData>::iterator it = m_fontData.find(fontID);
    fontTexture = m_font->GetTexture(); 
    const Glyph* glyphs = nullptr;
    int glyphCount = 0;
    lineHeight = m_font->GetLineHeight();
    
    if (it != m_fontData.end()) {
        fontTexture = it->second.texture;
        glyphCount = it->second.glyphCount;
        lineHeight = it->second.lineHeight;
    }
    
    penX = x;
    penY = y;
    
    // Optimized text rendering: draw each character with DrawWithTexture
    if (text.empty()) return;
    
    // Calculate total vertices needed (4 per character + 6 for shadow if needed)
    size_t textLength = text.size();
    
    // Calculate bounding box for the text
    float textWidth = 0.0f;
    float textHeight = lineHeight;
    
    // Begin batch for this text
    // REMOVED: Begin/End calls moved to TextManager level for proper batching
    // if (m_spriteRenderer) {
    //     OutputDebugStringA("[TextManager] About to call SpriteRenderer::Begin...\n");
    //     m_spriteRenderer->Begin("sprite", fontTexture);
    //     OutputDebugStringA("[TextManager] After SpriteRenderer::Begin\n");
    //     m_spriteRenderer->SetCurrentDepth(0.1f); // Text is in front of background
    // } else {
    //     OutputDebugStringA("[TextManager] WARNING: m_spriteRenderer is NULL!\n");
    // }
    
    int letterCount = 0;
    for (size_t i = 0; i < textLength; ++i)
    {
        unsigned char c = (unsigned char)text[i];
        
        // New line handling
        if (c == '\n') { penX = x; penY += lineHeight * scale; continue; }
        if (c == '\r') continue;
        
        // Get glyph data
        Glyph glyph;
        bool hasGlyph = false;
        
        if (glyphs && c < glyphCount) {
            glyph = glyphs[c];
            hasGlyph = true;
        } else if (m_font) {
            const std::vector<FontChar>& chars = m_font->GetChars();
            if (c < chars.size()) {
                const FontChar& fc = chars[c];
                glyph.u0 = fc.u0;
                glyph.v0 = fc.v0;
                glyph.u1 = fc.u1;
                glyph.v1 = fc.v1;
                glyph.width = fc.width;
                glyph.height = fc.height;
                glyph.xOffset = fc.xOffset;
                glyph.yOffset = fc.yOffset;
                glyph.xAdvance = fc.xAdvance;
                hasGlyph = true;
            }
        }
            
        if (!hasGlyph) continue; // Skip missing glyphs
            
        // Calculate character position and UVs
        // Use scale directly - no normalization needed
        float charW = glyph.width * scale;
        float charH = glyph.height * scale;
        float charX = penX + glyph.xOffset * scale;
        float charY = penY + glyph.yOffset * scale;
            
        // UV coordinates from BitmapFont are already normalized (0-1)
        // Just add small padding to prevent atlas bleeding
        float uvPadding = 0.001f; // Small UV-space padding
        float u0 = glyph.u0 + uvPadding;
        float v0 = glyph.v0 + uvPadding;
        float u1 = glyph.u1 - uvPadding;
        float v1 = glyph.v1 - uvPadding;
        
//        char dbg[256];
//        sprintf(dbg, "[TextManager] char='%c' pos=(%.1f,%.1f) uv=(%.4f,%.4f,%.4f,%.4f) size=(%.1f,%.1f)\n", 
//                c, charX, charY, u0, v0, u1, v1, charW, charH);
//        OutputDebugStringA(dbg);
        
        // Clamp UVs to valid range
        u0 = max(0.0f, min(1.0f, u0));
        v0 = max(0.0f, min(1.0f, v0));
        u1 = max(0.0f, min(1.0f, u1));
        v1 = max(0.0f, min(1.0f, v1));
        
		// Draw shadow first if needed
		if (style == FONT_STYLE_SHADOW && m_spriteRenderer) {
			float shadowX = charX + 1.0f;
			float shadowY = charY + 1.0f;
//			char dbg[256];
//			sprintf(dbg, "[TextManager] About to call DrawWithTexture for SHADOW at (%.1f,%.1f)\n", shadowX, shadowY);
//			OutputDebugStringA(dbg);
			m_spriteRenderer->DrawWithTexture(shadowX, shadowY, charW, charH, u0, v0, u1, v1, fontTexture, 0xFF000000);
		}
        
		// Draw main character
		if (m_spriteRenderer) {
//			char dbg[256];
//			sprintf(dbg, "[TextManager] About to call DrawWithTexture for MAIN char at (%.1f,%.1f)\n", charX, charY);
//			OutputDebugStringA(dbg);
			m_spriteRenderer->DrawWithTexture(charX, charY, charW, charH, u0, v0, u1, v1, fontTexture, color);
//			sprintf(dbg, "[TextManager] DrawWithTexture for MAIN char returned\n");
//			OutputDebugStringA(dbg);
		} else {
			OutputDebugStringA("[TextManager] ERROR: m_spriteRenderer is NULL in DrawString!\n");
		}

		// Update text dimensions
		textWidth += charW;
		textHeight = (textHeight > charH) ? textHeight : charH; //  max  

		// Advance pen position
		penX += glyph.xAdvance * scale;
		letterCount++;
	}   
    
// End batch and flush to GPU
    // REMOVED: Begin/End calls moved to TextManager level for proper batching
    // if (m_spriteRenderer) {
    //     m_spriteRenderer->End();
    //     // REMOVED Flush() - let MenuScene control batch timing
    //     // m_spriteRenderer->Flush();
    // }
}

void TextManager::DrawTextToScreen(const std::string& text, float x, float y, D3DCOLOR color, float scale, FontID fontID, FontStyle style)
{
//    char debugBuf[256];
//    sprintf(debugBuf, "[TextManager::DrawTextToScreen] Drawing '%s' at (%.1f,%.1f) with color=0x%08X\n", 
//            text.c_str(), x, y, color);
//    OutputDebugStringA(debugBuf);
    
    // Получаем текстуру атласа напрямую из BitmapFont
    LPDIRECT3DTEXTURE9 fontTex = m_font->GetTexture();
    
    // Логируем адрес текстуры
//    sprintf(debugBuf, "[TextManager] Using texture: %p\n", fontTex);
//    OutputDebugStringA(debugBuf);
    
    DrawString(text, x, y, color, scale, fontID, true, style, 0.05f);
    
//    sprintf(debugBuf, "[TextManager::DrawTextToScreen] Completed drawing '%s'\n", text.c_str());
//    OutputDebugStringA(debugBuf);
}

void TextManager::DrawTextToWorld(const std::string& text, float worldX, float worldY, D3DCOLOR color, float scale, FontID fontID, FontStyle style)
{
    DrawString(text, worldX, worldY, color, scale, fontID, false, style, 0.5f);
}

void TextManager::BeginTextBatch(FontID fontID, float depth)
{
//    char dbg[256];
//    sprintf(dbg, "[TextManager::BeginTextBatch] ENTRY m_spriteRenderer=%p\n", m_spriteRenderer);
//    OutputDebugStringA(dbg);
    
    if (!m_spriteRenderer) {
        OutputDebugStringA("[TextManager::BeginTextBatch] ERROR: m_spriteRenderer is NULL!\n");
        return;
    }
    
    LPDIRECT3DTEXTURE9 fontTexture = GetFontTexture(fontID);
//    sprintf(dbg, "[TextManager::BeginTextBatch] fontTexture=%p\n", fontTexture);
//    OutputDebugStringA(dbg);
    
    if (!fontTexture) {
        // Fallback to default font texture
        if (m_font) {
            fontTexture = m_font->GetTexture();
//            sprintf(dbg, "[TextManager::BeginTextBatch] Using fallback fontTexture=%p\n", fontTexture);
//            OutputDebugStringA(dbg);
        }
    }
    
    if (fontTexture) {
//        OutputDebugStringA("[TextManager] BeginTextBatch - calling SpriteRenderer::Begin...\n");
        m_spriteRenderer->Begin("sprite", fontTexture);
        m_spriteRenderer->SetCurrentDepth(depth);
//        OutputDebugStringA("[TextManager] BeginTextBatch completed\n");
    } else {
        OutputDebugStringA("[TextManager::BeginTextBatch] ERROR: No font texture available!\n");
    }
}

void TextManager::EndTextBatch()
{
    if (m_spriteRenderer) {
//        OutputDebugStringA("[TextManager] EndTextBatch - calling SpriteRenderer::End...\n");
        m_spriteRenderer->End();
        OutputDebugStringA("[TextManager] EndTextBatch completed\n");
    }
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
//        char debugBuf[256];
//        sprintf(debugBuf, "[TextManager::RenderScreen] Copying %zu text vertices to buffer\n", m_screenVertices.size());
//        OutputDebugStringA(debugBuf);
        
        // Get the shared vertex buffer from SpriteRenderer
        ShaderManager* shaderManager = m_renderer->GetShaderManager();
        LPDIRECT3DVERTEXBUFFER9 pVB = shaderManager->GetSharedVertexBuffer();
        
        if (pVB) {
            void* pData = NULL;
            HRESULT hr = pVB->Lock(0, m_screenVertices.size() * sizeof(SpriteVertex), &pData, 0);
            if (SUCCEEDED(hr)) {
                memcpy(pData, m_screenVertices.data(), m_screenVertices.size() * sizeof(SpriteVertex));
                pVB->Unlock();
//                OutputDebugStringA("[TextManager::RenderScreen] Text vertices copied to shared buffer\n");
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
