// Graphics/BitmapFont.h
#ifndef BITMAPFONT_H
#define BITMAPFONT_H

#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>
#include "TextVertex.h"

// ==========================================================
// Структура символа шрифта
// ==========================================================
struct FontChar
{
    float u0, v0;    // левый верхний угол UV
    float u1, v1;    // правый нижний угол UV
    float width;     // ширина символа в пикселях
    float height;    // высота символа в пикселях
    float xOffset;   // смещение символа по X
    float yOffset;   // смещение символа по Y
    float xAdvance;  // смещение пера для следующего символа
};
// ==========================================================
// Пространство шрифта (для выбора между экранным и мировым)
// ==========================================================
enum FontSpace
{
    FONT_SPACE_SCREEN = 0, // экранные координаты
    FONT_SPACE_WORLD       // мировые координаты
};

// ==========================================================
// Класс BitmapFont
// ==========================================================
class BitmapFont
{
public:
    explicit BitmapFont(LPDIRECT3DDEVICE9 device);
    ~BitmapFont();
	float GetBaseLine() const { return m_baseLine; }
    // Инициализация шейдеров, VB и декларации вертексов
    bool Initialize();

    // Загрузка шрифта из .fnt и текстуры
    bool LoadFromFile(const wchar_t* fontDefinitionFile);

    // CPU: сборка вертексов для текста
    void Begin(); // очищает m_vertices
    void DrawText(const std::string& text, DWORD color);
    void DrawText(const std::string& text); // по умолчанию белый цвет

    // GPU: рендер текста    
	void Render(
        const D3DXVECTOR3& origin,
        const D3DXMATRIX& view,
        const D3DXMATRIX& proj,
        float scale,
        bool mirrorX = false,
		bool mirrorY = false
    );

    // Получить доступ к устройству и текстуре
    LPDIRECT3DDEVICE9 GetDevice() const { return m_device; }
    IDirect3DTexture9* GetTexture() const { return m_texture; }

	LPDIRECT3DVERTEXDECLARATION9 GetDecl() const { return m_decl; }
    LPDIRECT3DVERTEXSHADER9 GetVS() const { return m_vs; }
    LPDIRECT3DPIXELSHADER9 GetPS() const { return m_ps; }

    // Управление CPU вертексами (для TextManager)
    void SetVertices(const std::vector<TextVertex>& vertices) { m_vertices = vertices; }
	const std::vector<TextVertex>& GetVertices() const { return m_vertices; }
    // Доступ к массиву символов
    const std::vector<FontChar>& GetChars() const { return m_chars; }

    // Основная высота строки (lineHeight из .fnt)
    float GetLineHeight() const { return m_lineHeight; }

private:
    // Создание/расширение VertexBuffer
    void EnsureVB(size_t vertexCount);

    // Загрузка текстуры в память
    bool LoadTextureFromMemory(const wchar_t* texturePath);

private:
    // ======================================================
    // D3D объекты
    // ======================================================
    LPDIRECT3DDEVICE9 m_device;
    LPDIRECT3DTEXTURE9 m_texture;
    LPDIRECT3DVERTEXBUFFER9 m_vb;
    UINT m_vbCapacity;

    LPDIRECT3DVERTEXSHADER9 m_vs;		// для 2D
	LPDIRECT3DVERTEXSHADER9 m_vsWorld;  // для World
    LPDIRECT3DPIXELSHADER9  m_ps;
    LPDIRECT3DVERTEXDECLARATION9 m_decl;

    // ======================================================
    // CPU буфер вертексов
    // ======================================================
    std::vector<TextVertex> m_vertices; // локальные / мировые вертексы

    // ======================================================
    // Шрифт
    // ======================================================
    std::vector<FontChar> m_chars;
    float m_lineHeight;
    float m_textureWidth;
    float m_textureHeight;
	float m_baseLine;
};

#endif // BITMAPFONT_H