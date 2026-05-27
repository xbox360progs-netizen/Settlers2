// Graphics/Camera.cpp
#include "stdafx.h"
#include "Camera.h"
#include "ShaderManager.h"

Camera::Camera()
: m_pShaderManager(nullptr)
, m_screenWidth(0)
, m_screenHeight(0)
, m_posX(0)
, m_posY(0)
, m_zoom(1.0f)
{
    D3DXMatrixIdentity(&m_view);
    D3DXMatrixIdentity(&m_proj);
}

void Camera::Initialize(float screenWidth, float screenHeight, ShaderManager* pShaderManager)
{
    m_screenWidth  = screenWidth;
    m_screenHeight = screenHeight;
    m_pShaderManager = pShaderManager;

    Reset();
    Update();
}

//
// WORLD CAMERA (RTS style)
//
void Camera::Update()
{
    D3DXMATRIX translate;
    D3DXMATRIX scale;

    if (m_pShaderManager) {
        D3DXMATRIX identity;
        D3DXMatrixIdentity(&identity);
        m_pShaderManager->UpdateGlobalMatrices(&identity, &identity);
    }

    D3DXMatrixTranslation(&translate, -m_posX, -m_posY, 0.0f);
    D3DXMatrixScaling(&scale, m_zoom, m_zoom, 1.0f);

    m_view = translate * scale;

    D3DXMatrixOrthoOffCenterLH(
        &m_proj,
        0.0f,
        m_screenWidth,
        m_screenHeight,
        0.0f,
        -1.0f,
        1.0f);

    if (m_pShaderManager) {
        D3DXMATRIX viewProj = m_view * m_proj;
        m_pShaderManager->UpdateGlobalMatrices(&m_view, &m_proj);
        m_pShaderManager->SetShaderMatrix(SHADER_TERRAIN, &viewProj);
        m_pShaderManager->SetShaderMatrix(SHADER_WORLD, &viewProj);
    }
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

    if (m_pShaderManager) {
        D3DXMATRIX viewProj = m_proj;
        m_pShaderManager->UpdateGlobalMatrices(&m_view, &m_proj);
        m_pShaderManager->SetGlobalUniforms(SHADER_WORLD, &viewProj);
    }
}

void Camera::SetPosition(float x,float y)
{
    m_posX = x;
    m_posY = y;
    Update();
}

void Camera::Move(float dx,float dy)
{
    m_posX += dx / m_zoom;
    m_posY += dy / m_zoom;
    Update();
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
    float newWorldX = m_posX + halfW / m_zoom;
    float newWorldY = m_posY + halfH / m_zoom;
    
    // Корректируем позицию камеры
    m_posX += worldX - newWorldX;
    m_posY += worldY - newWorldY;
    
    Update();
}

void Camera::Zoom(float dz, float centerScreenX, float centerScreenY)
{
    if (abs(dz) < 0.001f) return;
    
    float oldZoom = m_zoom;
    m_zoom += dz;

    if(m_zoom < 0.25f) m_zoom = 0.25f;
    if(m_zoom > 4.0f)  m_zoom = 4.0f;
    
    // Корректируем позицию камеры чтобы зум был от центра (centerScreenX, centerScreenY)
    // Точка в мире до зума
    float worldX, worldY;
    ScreenToWorld(centerScreenX, centerScreenY, worldX, worldY);
    
    // Точка в мире после зума должна быть той же
    float newWorldX = m_posX + centerScreenX / m_zoom;
    float newWorldY = m_posY + (m_screenHeight - centerScreenY) / m_zoom;
    
    // Корректируем позицию камеры
    m_posX += worldX - newWorldX;
    m_posY += worldY - newWorldY;
    
    Update();
}

void Camera::Reset()
{
    m_posX = 0;
    m_posY = 0;
    m_zoom = 1.0f;
    Update();
}

//
// Screen → World
//
void Camera::ScreenToWorld(float sx,float sy,float& wx,float& wy) const
{
    wx = m_posX + sx / m_zoom;
    wy = m_posY + (m_screenHeight - sy) / m_zoom;
}

//
// World → Screen
//
void Camera::WorldToScreen(float wx,float wy,float& sx,float& sy) const
{
    sx = (wx - m_posX) * m_zoom;
    sy = m_screenHeight - (wy - m_posY) * m_zoom;
}

void Camera::GetWorldCenter(float& wx, float& wy) const
{
    wx = m_posX + m_screenWidth * 0.5f / m_zoom;
    wy = m_posY + m_screenHeight * 0.5f / m_zoom;
}
