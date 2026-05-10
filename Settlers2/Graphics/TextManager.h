// Graphics/TextManager.h
#ifndef TEXTMANAGER_H
#define TEXTMANAGER_H

#pragma once
#include "BitmapFont.h"
#include "TextVertex.h"
#include "Camera.h"
#include <vector>
#include <string>
struct ScreenTextEntry {
    std::string text;
    float x, y;
    D3DCOLOR color;
    float scale;
};
class TextManager
{
public:
    typedef void (*IntegrityHook)(void* ctx, const char* tag);

    TextManager(BitmapFont* font, float screenWidth, float screenHeight, IDirect3DDevice9* device);
    ~TextManager();
	std::vector<TextVertex> m_vertices;
	std::vector<ScreenTextEntry> m_pendingScreenTexts;
    void Begin(); // Очистка всех буферов вертексов

    // Добавление текста
    void AddTextScreen(const std::string& text, const D3DXVECTOR3& pos, D3DCOLOR color = 0xFFFFFFFF, float scale = 0.10f);
    // Рендер
    void RenderScreen(); // мировые тексты
	void DrawTextToScreen(const std::string& text, float x, float y, D3DCOLOR color = 0xFFFFFFFF, float scale = 0.10f);

	void DrawTextToWorld(const std::string& text, float worldX, float worldY, D3DCOLOR color = 0xFFFFFFFF, float scale = 0.1f);
    void RenderWorld(Camera* camera); // Новый метод для рендеринга world текста
    void SetIntegrityHook(IntegrityHook hook, void* ctx) { m_integrityHook = hook; m_integrityCtx = ctx; }

private:
    BitmapFont* m_font;
    IDirect3DDevice9* m_device;

    float m_screenWidth;
    float m_screenHeight;


    std::vector<TextVertex> m_screenVertices; // экранные тексты
    std::vector<TextVertex> m_worldVertices;  // мировые тексты

	LPDIRECT3DVERTEXBUFFER9 m_screenVB;
    UINT m_screenVBCapacity;
    
    void EnsureScreenVB(size_t vertexCount);
    void CheckIntegrity(const char* tag) const
    {
        if (m_integrityHook)
            m_integrityHook(m_integrityCtx, tag);
    }

    IntegrityHook m_integrityHook;
    void* m_integrityCtx;

	struct WorldText {
        std::string text;
        D3DXVECTOR3 position;
        D3DCOLOR color;
        float scale;
    };
    std::vector<WorldText> m_worldTexts;
};

#endif // TEXTMANAGER_H
