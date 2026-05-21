#include "stdafx.h"
#include "ShaderManager.h"
#include "Renderer.h"
#include <d3dx9.h>
#include <d3d9.h>
#include <stdio.h>
#include <string>
#include <algorithm>
#include <math.h>

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

ShaderManager::ShaderManager()
    : m_pDevice(NULL), m_pActiveShader(NULL), m_pActiveEffect(NULL), m_numPasses(0), m_currentShaderID(SHADER_INVALID), m_hasFrameViewProj(false)
{
    for (int i = 0; i < SHADER_COUNT; ++i) {
        D3DXMatrixIdentity(&m_shaderMatrices[i]);
    }
    D3DXMatrixIdentity(&m_cachedView);
    D3DXMatrixIdentity(&m_cachedProj);
    InitializeCriticalSection(&m_cs);
}

ShaderManager::~ShaderManager() {
    Shutdown();
}

HRESULT ShaderManager::Initialize(LPDIRECT3DDEVICE9 device) {
    m_pDevice = device;
    return S_OK;
}

void ShaderManager::Shutdown() {
    for (std::map<ShaderID, Shader>::iterator it = m_shaders.begin(); it != m_shaders.end(); ++it) {
        if (it->second.pEffect) {
            it->second.pEffect->Release();
        }
    }
    m_shaders.clear();
    m_effects.clear();
    m_pActiveShader = NULL;
    m_pActiveEffect = NULL;
    DeleteCriticalSection(&m_cs);
}

HRESULT ShaderManager::LoadInternal(ShaderID id, const char* path, const char* technique) {
    if (!m_pDevice) {
        OutputDebugStringA("[ShaderManager] ERROR: Device not initialized\n");
        return E_FAIL;
    }

    if (m_effects.find(id) != m_effects.end()) {
        return S_OK;
    }

    ID3DXEffect* pEffect = NULL;
    ID3DXBuffer* pErrorBuffer = NULL;

    DWORD dwFlags = D3DXSHADER_DEBUG;

    HRESULT hr = D3DXCreateEffectFromFileA(
        m_pDevice,
        path,
        NULL,
        NULL,
        dwFlags,
        NULL,
        &pEffect,
        &pErrorBuffer);

    if (FAILED(hr)) {
        if (pErrorBuffer) {
            char errMsg[512];
            sprintf(errMsg, "[ShaderManager] ERROR loading %s: %s\n", path, (char*)pErrorBuffer->GetBufferPointer());
            OutputDebugStringA(errMsg);
            pErrorBuffer->Release();
        }
        return hr;
    }

    m_effects[id] = pEffect;

    Shader shader;
    shader.pEffect = pEffect;
    shader.hTechnique = pEffect->GetTechniqueByName(technique);
    shader.hMatOrtho = pEffect->GetParameterByName(NULL, "matOrtho");
    shader.hTexture = pEffect->GetParameterByName(NULL, "g_texture");

    D3DXHANDLE hIter = NULL;
    D3DXEFFECT_DESC desc;
    pEffect->GetDesc(&desc);
    for (UINT i = 0; i < desc.Parameters; i++) {
        D3DXHANDLE hParam = pEffect->GetParameter(NULL, i);
        D3DXPARAMETER_DESC paramDesc;
        pEffect->GetParameterDesc(hParam, &paramDesc);
        shader.hParams[paramDesc.Name] = hParam;
    }

    m_shaders[id] = shader;

    char buf[256];
    sprintf(buf, "[ShaderManager] Loaded shader %d from %s\n", id, path);
    OutputDebugStringA(buf);

    return S_OK;
}

HRESULT ShaderManager::LoadShader(ShaderID id, const char* filepath, const char* techniqueName) {
    return LoadInternal(id, filepath, techniqueName);
}

bool ShaderManager::SetActiveShader(ShaderID id) {
    if (!ValidateShader(id)) {
        OutputDebugStringA("[ShaderManager] ERROR: Invalid shader ID\n");
        return false;
    }

    std::map<ShaderID, Shader>::iterator it = m_shaders.find(id);
    if (it == m_shaders.end()) {
        OutputDebugStringA("[ShaderManager] ERROR: Shader not found in map\n");
        return false;
    }

    m_pActiveShader = &it->second;
    m_pActiveEffect = it->second.pEffect;
    m_currentShaderID = id;

    return true;
}

ShaderManager::Shader* ShaderManager::GetShader(ShaderID id) {
    std::map<ShaderID, Shader>::iterator it = m_shaders.find(id);
    if (it != m_shaders.end()) {
        return &it->second;
    }
    return NULL;
}

void ShaderManager::BeginShader() {
    if (!m_pActiveEffect) return;
    m_pActiveEffect->SetTechnique(m_pActiveShader->hTechnique);
    m_pActiveEffect->Begin(&m_numPasses, 0);
}

void ShaderManager::BeginPass(UINT pass) {
    if (!m_pActiveEffect) return;
    m_pActiveEffect->BeginPass(pass);
}

void ShaderManager::EndPass() {
    if (!m_pActiveEffect) return;
    m_pActiveEffect->EndPass();
}

void ShaderManager::EndShader() {
    if (!m_pActiveEffect) return;
    m_pActiveEffect->End();
}

void ShaderManager::Commit() {
    if (!m_pActiveEffect) return;
    m_pActiveEffect->CommitChanges();
}

void ShaderManager::SetMatrix(const char* paramName, const float* matrix) {
    if (!m_pActiveEffect) return;
    D3DXHANDLE hParam = m_pActiveShader->hParams[paramName];
    if (hParam) {
        m_pActiveEffect->SetMatrix(hParam, (D3DXMATRIX*)matrix);
    }
}

void ShaderManager::SetVector(const char* paramName, const float* vector) {
    if (!m_pActiveEffect) return;
    D3DXHANDLE hParam = m_pActiveShader->hParams[paramName];
    if (hParam) {
        m_pActiveEffect->SetVector(hParam, (D3DXVECTOR4*)vector);
    }
}

void ShaderManager::SetTexture(const char* paramName, LPDIRECT3DBASETEXTURE9 pTexture) {
    if (!m_pActiveEffect) return;
    D3DXHANDLE hParam = m_pActiveEffect->GetParameterByName(NULL, paramName);
    if (hParam) {
        m_pActiveEffect->SetTexture(hParam, pTexture);
    }
}

void ShaderManager::OnLostDevice() {
    for (std::map<ShaderID, ID3DXEffect*>::iterator it = m_effects.begin(); it != m_effects.end(); ++it) {
        if (it->second) {
            it->second->OnLostDevice();
        }
    }
}

void ShaderManager::OnResetDevice() {
    for (std::map<ShaderID, ID3DXEffect*>::iterator it = m_effects.begin(); it != m_effects.end(); ++it) {
        if (it->second) {
            it->second->OnResetDevice();
        }
    }
}

bool ShaderManager::HasShader(ShaderID id) const {
    return m_effects.find(id) != m_effects.end();
}

HRESULT ShaderManager::LoadAll() {
    HRESULT hr;

    hr = LoadShader(SHADER_SPRITE, "Media/Shaders/Sprite.fx", "SpriteBatchTech");
    if (FAILED(hr)) {
        OutputDebugStringA("[ShaderManager] FATAL: Failed to load SPRITE shader\n");
        return hr;
    }

    hr = LoadShader(SHADER_SPRITE_CONSTANT_INSTANCED, "Media/Shaders/SpriteConstantInstanced.fx", "SpriteBatchTech");
    if (FAILED(hr)) {
        OutputDebugStringA("[ShaderManager] FATAL: Failed to load SPRITE_CONSTANT_INSTANCED shader\n");
        return hr;
    }

    hr = LoadShader(SHADER_RADIALMENU, "Media/Shaders/RadialMenu.fx", "RadialMenuTech");
    if (FAILED(hr)) {
        OutputDebugStringA("[ShaderManager] FATAL: Failed to load RADIALMENU shader\n");
        return hr;
    }

    return S_OK;
}

void ShaderManager::Prepare(ShaderID id, const D3DXMATRIX* pViewProj) {
    if (!ValidateShader(id)) {
        OutputDebugStringA("[ShaderManager] Prepare: Invalid shader ID\n");
        return;
    }

    if (pViewProj && id >= 0 && id < SHADER_COUNT) {
        m_shaderMatrices[id] = *pViewProj;
        m_hasFrameViewProj = true;
    }

    SetActiveShader(id);
}

void ShaderManager::EndCurrent() {
    if (m_pActiveEffect) {
        m_pActiveEffect->End();
        m_pActiveEffect = NULL;
        m_pActiveShader = NULL;
    }
}

void ShaderManager::SetGlobalUniforms(ShaderID id, const D3DXMATRIX* pViewProj) {
    if (!ValidateShader(id)) return;

    if (pViewProj) {
        m_shaderMatrices[id] = *pViewProj;
        m_hasFrameViewProj = true;
    }

    if (m_shaders.find(id) != m_shaders.end()) {
        Shader& shader = m_shaders[id];
        if (shader.hMatOrtho && shader.pEffect) {
            shader.pEffect->SetMatrix(shader.hMatOrtho, &m_shaderMatrices[id]);
        }
    }
}

void ShaderManager::SetLocalUniforms(LPDIRECT3DTEXTURE9 pTexture, float depth) {
    if (!m_pActiveShader || !m_pActiveEffect) return;

    if (m_pActiveShader->hTexture) {
        m_pActiveEffect->SetTexture(m_pActiveShader->hTexture, pTexture);
    }
}

void ShaderManager::UpdateConstants(LPDIRECT3DTEXTURE9 pTexture, const D3DXMATRIX* pWorldMatrix) {
    if (!m_pActiveEffect) return;

    if (pWorldMatrix) {
        D3DXMATRIX worldViewProj = *pWorldMatrix;
        if (m_hasFrameViewProj) {
            D3DXMatrixMultiply(&worldViewProj, pWorldMatrix, &m_shaderMatrices[m_currentShaderID]);
        }
        D3DXHANDLE hWVP = m_pActiveEffect->GetParameterByName(NULL, "WVP");
        if (hWVP) {
            m_pActiveEffect->SetMatrix(hWVP, &worldViewProj);
        }
    }

    if (m_pActiveShader->hTexture) {
        m_pActiveEffect->SetTexture(m_pActiveShader->hTexture, pTexture);
    }
}

void ShaderManager::SetFrameViewProj(const D3DXMATRIX* pViewProj) {
    if (!pViewProj) return;

    for (int i = 0; i < SHADER_COUNT; ++i) {
        m_shaderMatrices[i] = *pViewProj;
    }
    m_hasFrameViewProj = true;
}

void ShaderManager::UpdateGlobalMatrices(const D3DXMATRIX* pView, const D3DXMATRIX* pProj) {
    m_cachedView = *pView;
    m_cachedProj = *pProj;

    D3DXMATRIX viewProj;
    D3DXMatrixMultiply(&viewProj, pView, pProj);

    for (int i = 0; i < SHADER_COUNT; ++i) {
        m_shaderMatrices[i] = viewProj;
    }
    m_hasFrameViewProj = true;
}

void ShaderManager::SetShaderMatrix(ShaderID id, const D3DXMATRIX* pMatrix) {
    if (id < 0 || id >= SHADER_COUNT) return;
    m_shaderMatrices[id] = *pMatrix;
}

bool ShaderManager::Init() {
    return SUCCEEDED(LoadAll());
}

bool ShaderManager::LoadBaseShaders() {
    return SUCCEEDED(LoadAll());
}

ID3DXEffect* ShaderManager::GetEffect(ShaderID id) {
    std::map<ShaderID, ID3DXEffect*>::iterator it = m_effects.find(id);
    if (it != m_effects.end()) {
        return it->second;
    }
    return NULL;
}

void ShaderManager::CommitChanges() {
    if (m_pActiveEffect) {
        m_pActiveEffect->CommitChanges();
    }
}

bool ShaderManager::ValidateShader(ShaderID id) const {
    return id >= 0 && id < SHADER_COUNT && m_effects.find(id) != m_effects.end();
}

void ShaderManager::ApplyShader(int shaderID) {
    SetActiveShader((ShaderID)shaderID);
    BeginShader();
    BeginPass(0);
    Commit();
}

}
