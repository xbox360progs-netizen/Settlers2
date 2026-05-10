// Graphics/Camera.h
#pragma once
#include <d3dx9.h>

class Camera
{
public:

    Camera();

    void Initialize(float screenWidth, float screenHeight);

    void Update();     // world camera
    void UpdateUI();   // ui camera

    void SetPosition(float x, float y);
    void Move(float dx, float dy);
    void Zoom(float dz);
    void Zoom(float dz, float centerScreenX, float centerScreenY);
    void Reset();
    float GetZoom() const { return m_zoom; }
    float GetScreenWidth() const { return m_screenWidth; }
    float GetScreenHeight() const { return m_screenHeight; }
	float GetPosX() const { return m_posX; }
	float GetPosY() const { return m_posY; }

    void ScreenToWorld(float sx,float sy,float& wx,float& wy) const;
    void WorldToScreen(float wx,float wy,float& sx,float& sy) const;

    const D3DXMATRIX& GetViewMatrix() const { return m_view; }
    const D3DXMATRIX& GetProjectionMatrix() const { return m_proj; }

private:

    float m_screenWidth;
    float m_screenHeight;

    float m_posX;
    float m_posY;
    float m_zoom;

    D3DXMATRIX m_view;
    D3DXMATRIX m_proj;
};
