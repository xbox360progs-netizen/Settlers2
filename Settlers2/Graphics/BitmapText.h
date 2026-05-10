// Graphics/BitmapText.h
#pragma once
#include "TextVertex.h"
#include <vector>
#include <string>

class BitmapFont; // forward

class BitmapText
{
public:
    BitmapText(BitmapFont* font) : m_font(font), m_x(0), m_y(0), m_scale(1.0f), m_color(0xFFFFFFFF) 
	{
		D3DXMatrixIdentity(&m_world);
	}

    void SetText(const std::string& text, float x, float y, DWORD color = 0xFFFFFFFF, float scale = 1.0f)
    {
        m_text = text;
        m_x = x;
        m_y = y;
        m_color = color;
        m_scale = scale;
    }

    void UpdateVertices(std::vector<TextVertex>& outVertices) const;
	void SetWorldMatrix(const D3DXMATRIX& world) { m_world = world; }
	const D3DXMATRIX& GetWorldMatrix() const { return m_world; }

private:
    BitmapFont* m_font;
	D3DXMATRIX m_world; // матрица мира для текста
    std::string m_text;
    float m_x, m_y;
    float m_scale;
    DWORD m_color;

};