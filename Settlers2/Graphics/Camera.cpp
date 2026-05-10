// Graphics/Camera.cpp
#include "stdafx.h"
#include "Camera.h"

Camera::Camera()
: m_screenWidth(0)
, m_screenHeight(0)
, m_posX(0)
, m_posY(0)
, m_zoom(1.0f)
{
    D3DXMatrixIdentity(&m_view);
    D3DXMatrixIdentity(&m_proj);
}

void Camera::Initialize(float screenWidth,float screenHeight)
{
    m_screenWidth  = screenWidth;
    m_screenHeight = screenHeight;

    Reset();
    Update();
}

//
// WORLD CAMERA (RTS style)
//
void Camera::Update()
{
    float halfW = m_screenWidth  * 0.5f;
    float halfH = m_screenHeight * 0.5f;

    D3DXMATRIX translate;
    D3DXMATRIX center;
    D3DXMATRIX scale;
    D3DXMATRIX uncenter;

    // камера двигает мир в обратную сторону
    D3DXMatrixTranslation(&translate, -m_posX, -m_posY, 0.0f);

    // zoom от центра экрана
    D3DXMatrixTranslation(&center, -halfW, -halfH, 0.0f);
    D3DXMatrixScaling(&scale, m_zoom, m_zoom, 1.0f);
    D3DXMatrixTranslation(&uncenter, halfW, halfH, 0.0f);

    // VIEW matrix
    m_view = translate * center * scale * uncenter;

    // projection фиксированная
    D3DXMatrixOrthoOffCenterLH(
        &m_proj,
        0.0f,
        m_screenWidth,
        m_screenHeight,
        0.0f,
        -1.0f,
        1.0f);
}

//
// UI CAMERA
//
void Camera::UpdateUI()
{
    D3DXMatrixIdentity(&m_view);

    D3DXMatrixOrthoOffCenterLH(
        &m_proj,
        0.0f,
        m_screenWidth,
        m_screenHeight,
        0.0f,
        -1.0f,
        1.0f);
}

void Camera::SetPosition(float x,float y)
{
    m_posX = x;
    m_posY = y;
}

void Camera::Move(float dx,float dy)
{
    m_posX += dx / m_zoom;
    m_posY += dy / m_zoom;
}

void Camera::Zoom(float dz)
{
    if (abs(dz) < 0.001f) return;
    
    float oldZoom = m_zoom;
    m_zoom += dz;

    if(m_zoom < 0.25f) m_zoom = 0.25f;
    if(m_zoom > 4.0f)  m_zoom = 4.0f;
    
    // Корректируем позицию камеры чтобы зум был от центра экрана
    float halfW = m_screenWidth * 0.5f;
    float halfH = m_screenHeight * 0.5f;
    
    // Точка в мире до зума
    float worldX, worldY;
    ScreenToWorld(halfW, halfH, worldX, worldY);
    
    // Точка в мире после зума должна быть той же
    float newWorldX = (halfW - halfW) / m_zoom + halfW + m_posX;
    float newWorldY = (halfH - halfH) / m_zoom + halfH + m_posY;
    
    // Корректируем позицию камеры
    m_posX += worldX - newWorldX;
    m_posY += worldY - newWorldY;
}

void Camera::Zoom(float dz, float centerScreenX, float centerScreenY)
{
    if (abs(dz) < 0.001f) return;
    
    float oldZoom = m_zoom;
    m_zoom += dz;

    if(m_zoom < 0.25f) m_zoom = 0.25f;
    if(m_zoom > 4.0f)  m_zoom = 4.0f;
    
    // Корректируем позицию камеры чтобы зум был от центра (centerScreenX, centerScreenY)
    float halfW = m_screenWidth * 0.5f;
    float halfH = m_screenHeight * 0.5f;
    
    // Точка в мире до зума
    float worldX, worldY;
    ScreenToWorld(centerScreenX, centerScreenY, worldX, worldY);
    
    // Точка в мире после зума должна быть той же
    float newWorldX = (centerScreenX - halfW) / m_zoom + halfW + m_posX;
    float newWorldY = (centerScreenY - halfH) / m_zoom + halfH + m_posY;
    
    // Корректируем позицию камеры
    m_posX += worldX - newWorldX;
    m_posY += worldY - newWorldY;
}

void Camera::Reset()
{
    m_posX = 0;
    m_posY = 0;
    m_zoom = 1.0f;
}

//
// Screen → World
//
void Camera::ScreenToWorld(float sx,float sy,float& wx,float& wy) const
{
    float halfW = m_screenWidth*0.5f;
    float halfH = m_screenHeight*0.5f;

    wx = (sx - halfW)/m_zoom + halfW + m_posX;
    wy = (sy - halfH)/m_zoom + halfH + m_posY;
}

//
// World → Screen
//
void Camera::WorldToScreen(float wx,float wy,float& sx,float& sy) const
{
    float halfW = m_screenWidth*0.5f;
    float halfH = m_screenHeight*0.5f;

    sx = (wx - m_posX - halfW)*m_zoom + halfW;
    sy = (wy - m_posY - halfH)*m_zoom + halfH;
}
