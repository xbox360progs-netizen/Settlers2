// Graphics/TextManager.cpp
#include "stdafx.h"
#include "TextManager.h"
#include <d3dx9.h>

#define DEBUG_TEXT_SKIP_RENDER 0
#define DEBUG_TEXT_SKIP_SETSTREAM 0
#define DEBUG_TEXT_SKIP_DRAWPRIM 0

TextManager::TextManager(BitmapFont* font, float screenWidth, float screenHeight, IDirect3DDevice9* device)
: m_font(font), m_device(device), m_screenWidth(screenWidth), m_screenHeight(screenHeight)
, m_screenVB(nullptr), m_screenVBCapacity(0), m_integrityHook(NULL), m_integrityCtx(NULL)
{
}

TextManager::~TextManager()
{
    if (m_screenVB) {
        m_screenVB->Release();
        m_screenVB = nullptr;
    }
}



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

    // 
    if (text.length() > 1000) {
        // 
        return;
    }

    float penX = pos.x;
    // Temporarily disable baseline offset for testing
    // float penY = pos.y - m_font->GetBaseLine() * scale;
    float penY = pos.y;

    const std::vector<FontChar>& chars = m_font->GetChars();
    if (chars.empty()) return;
    
    for (size_t i = 0; i < text.size(); ++i)
    {
        unsigned char c = (unsigned char)text[i];
        // New line handling
        if (c == '\n') { penX = pos.x; penY += m_font->GetLineHeight() * scale; continue; }
        if (c == '\r') continue;
        if (c >= chars.size()) {
            continue;
        }
        
        // ПРЕДОХРАНИТЕЛЬ от переполнения
        if (m_screenVertices.size() + 6 > 49990) {  // Оставляем запас
            break;
        }
        
        const FontChar& ch = chars[c];
        if (ch.width <= 0.0f || ch.height <= 0.0f) { penX += ch.xAdvance * scale; continue; }

        float x0 = penX + ch.xOffset * scale;
        float y0 = penY + ch.yOffset * scale;
        float x1 = penX + (ch.xOffset + ch.width) * scale;
        float y1 = penY + (ch.yOffset + ch.height) * scale;
		if (i == 0) // 
		{
//			sprintf(debugMsg, "[TextManager] Char '%c' yoffset = %.1f\n", 
//				text[i], ch.yOffset);
//			OutputDebugString(debugMsg);
//			sprintf(debugMsg, "[TextManager] Vertex Y: %.1f to %.1f\n", y0, y1);
//			OutputDebugString(debugMsg);
		}
        // 
        TextVertex v0(D3DXVECTOR3(x0, y0, pos.z), color, D3DXVECTOR2(ch.u0, ch.v0)); // Top-left
        TextVertex v1(D3DXVECTOR3(x1, y0, pos.z), color, D3DXVECTOR2(ch.u1, ch.v0)); // Top-right  
        TextVertex v2(D3DXVECTOR3(x0, y1, pos.z), color, D3DXVECTOR2(ch.u0, ch.v1)); // Bottom-left
        TextVertex v3(D3DXVECTOR3(x1, y1, pos.z), color, D3DXVECTOR2(ch.u1, ch.v1)); // Bottom-right

        //  (0,1,2)
        m_screenVertices.push_back(v0);
        m_screenVertices.push_back(v1);
        m_screenVertices.push_back(v2);

        //  (1,3,2)
        m_screenVertices.push_back(v1);
        m_screenVertices.push_back(v3);
        m_screenVertices.push_back(v2);

        // 
        penX += ch.xAdvance * scale;
    }
}


// ==========================================================
// 
// ==========================================================
void TextManager::DrawTextToScreen(const std::string& text, float x, float y, D3DCOLOR color, float scale)
{
	ScreenTextEntry entry;
    entry.text = text;
    entry.x = x;
    entry.y = y;
    entry.color = color;
    entry.scale = scale;
    m_pendingScreenTexts.push_back(entry);
//	char debugMsg[256];
//    sprintf(debugMsg, "[TextManager] Queued text: '%s' at (%.1f, %.1f)\n", text.c_str(), x, y);
//    OutputDebugString(debugMsg);
}


void TextManager::RenderScreen()
{
    if (m_pendingScreenTexts.empty())
        return;

    if (!m_device || !m_font)
    {
        m_pendingScreenTexts.clear();
        return;
    }

    if (DEBUG_TEXT_SKIP_RENDER)
    {
        m_pendingScreenTexts.clear();
        return;
    }

    CheckIntegrity("Text:enter");

    m_screenVertices.clear();
    
    // 
    if (m_pendingScreenTexts.size() > 1000) {
        m_pendingScreenTexts.clear();
        return;
    }

    for (size_t i = 0; i < m_pendingScreenTexts.size(); ++i)
    {
        const ScreenTextEntry& entry = m_pendingScreenTexts[i];
        
        // 
        if (entry.text.empty() || entry.scale <= 0) {
            continue;
        }

        D3DXVECTOR3 screenPos(entry.x, entry.y, 0.0f);
        AddTextScreen(entry.text, screenPos, entry.color, entry.scale);
        
        // 
        if (m_screenVertices.size() > 10000) {
            break;
        }
    }

    CheckIntegrity("Text:after build");

    if (m_screenVertices.empty())
        return;

    //  VB
    if (m_screenVertices.size() > 50000) {
        m_screenVertices.clear();
        m_pendingScreenTexts.clear();
        return;
    }

    EnsureScreenVB(m_screenVertices.size());
    CheckIntegrity("Text:after EnsureVB");
    if (!m_screenVB) {
        return;
    }

    // 
    TextVertex* vbData = nullptr;
    HRESULT hr = m_screenVB->Lock(
        0,
        sizeof(TextVertex) * m_screenVertices.size(),
        (void**)&vbData,
        0
    );

    if (FAILED(hr)) {
        return;
    }

    CheckIntegrity("Text:after Lock");

    // Копируем безопасно: не больше емкости буфера
    size_t safeCount = m_screenVertices.size();
    if (safeCount > m_screenVBCapacity) {
        safeCount = m_screenVBCapacity;
    }
    
    if (safeCount > 0) {
        size_t bytesToCopy = sizeof(TextVertex) * safeCount;
        // Дополнительная проверка на разумный размер
        if (bytesToCopy > m_screenVBCapacity * sizeof(TextVertex)) {
//            OutputDebugString("[TextManager] Bytes to copy exceeds buffer size!\n");
            bytesToCopy = m_screenVBCapacity * sizeof(TextVertex);
            safeCount = bytesToCopy / sizeof(TextVertex);
        }
        
        memcpy(vbData, m_screenVertices.data(), bytesToCopy);
    }

    CheckIntegrity("Text:after memcpy");


    m_screenVB->Unlock();
    CheckIntegrity("Text:after Unlock");

	
    
   if (!DEBUG_TEXT_SKIP_SETSTREAM) {
    if (!m_screenVB) {
        return;
    }
    
    if (m_screenVBCapacity == 0) {
        return;
    }
    
    if (sizeof(TextVertex) != 24) {
        return;
    }
    
    try {
        D3DVERTEXBUFFER_DESC vbDesc;
        HRESULT descHR = m_screenVB->GetDesc(&vbDesc);
        if (FAILED(descHR)) {
            return;
        }
        
        if (vbDesc.Size == 0) {
            return;
        }
    } catch (...) {
        return;
    }
    
    UINT stride = sizeof(TextVertex);
    UINT offset = 0;
    
    
    HRESULT hr = m_device->SetStreamSource(0, m_screenVB, 0, sizeof(TextVertex));
    if (FAILED(hr)) {
        return;
    }
    
//    OutputDebugString("[TextManager] SetStreamSource succeeded\n");
}

    CheckIntegrity("Text:after SetStreamSource");
    

    m_device->SetVertexDeclaration(m_font->GetDecl());
    m_device->SetVertexShader(m_font->GetVS());
    m_device->SetPixelShader(m_font->GetPS());
    CheckIntegrity("Text:after SetShaders");


    
    D3DXMATRIX proj;
    D3DXMatrixOrthoOffCenterLH(
        &proj,
        0.0f,
        m_screenWidth,
        m_screenHeight,
        0.0f,
        -1.0f,
        1.0f
    );

    m_device->SetVertexShaderConstantF(0, (float*)&proj, 4);
    CheckIntegrity("Text:after SetProjection");
    m_device->SetTexture(0, m_font->GetTexture());
    // Render state
    m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    UINT primitiveCount = m_screenVertices.size() / 3;

    if (!DEBUG_TEXT_SKIP_DRAWPRIM) {
    UINT primitiveCount = m_screenVertices.size() / 3;
    
    // Проверяем device перед вызовом
    if (!m_device) {
        return;
    }
    
    HRESULT hr = m_device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, primitiveCount);
    if (FAILED(hr)) {
        return;
    }
    
//    OutputDebugString("[TextManager] DrawPrimitive succeeded\n");
}
    CheckIntegrity("Text:after DrawPrimitive");
	m_device->SetTexture(0, nullptr);
    m_pendingScreenTexts.clear();
    m_screenVertices.clear();
    CheckIntegrity("TextManager:RenderScreen completely finished");
}


void TextManager::EnsureScreenVB(size_t vertexCount)
{
    if (!m_device) {
        return;
    }

    // Защита от огромных значений
    if (vertexCount > 50000) {
        vertexCount = 50000;
    }
    
    // Проверка, нужен ли новый буфер
    if (vertexCount <= m_screenVBCapacity && m_screenVB != nullptr) {
        return; // Буфер уже достаточного размера
    }
    
    // Освобождаем старый буфер если есть
    if (m_screenVB) {
        m_screenVB->Release();
        m_screenVB = nullptr;
    }

    // Создаем новый буфер с запасом
    size_t safeCapacity = vertexCount + 1000; // Запас 1000 вершин
    if (safeCapacity > 50000) {
        safeCapacity = 50000;
    }
    

    HRESULT hr = m_device->CreateVertexBuffer(
        sizeof(TextVertex) * safeCapacity,
        D3DUSAGE_WRITEONLY, 
        0,
        D3DPOOL_DEFAULT,
        &m_screenVB,
        nullptr
    );
    
    if (FAILED(hr) || !m_screenVB) {
        m_screenVB = nullptr;
        m_screenVBCapacity = 0;
        return;
    }
    
    m_screenVBCapacity = (UINT)safeCapacity;;
}


void TextManager::DrawTextToWorld(const std::string& text, float worldX, float worldY, D3DCOLOR color, float scale)
{
    WorldText wt;
    wt.text = text;
    wt.position = D3DXVECTOR3(worldX, worldY, 0.0f);
    wt.color = color;
    wt.scale = scale;
    m_worldTexts.push_back(wt);
}

void TextManager::RenderWorld(Camera* camera)
{
    if (!m_font || m_worldTexts.empty()) {
        return;
    }

   // m_font
    std::vector<TextVertex> savedVertices = m_font->GetVertices();

    for (size_t i = 0; i < m_worldTexts.size(); ++i) {
        const WorldText& wt = m_worldTexts[i];
        
        // world 
        m_font->Begin();
        m_font->DrawText(wt.text, wt.color);
        
        
        m_font->Render(wt.position, camera->GetViewMatrix(), camera->GetProjectionMatrix(), camera->GetZoom() * wt.scale);
    }
    
    m_worldTexts.clear();
}