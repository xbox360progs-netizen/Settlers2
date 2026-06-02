#include "stdafx.h"
#include "ShaderManager.h"
#include "Renderer.h"
#include <d3dx9.h>
#include <d3d9.h>
#include <stdio.h>
#include <string>
#include <algorithm>
#include <math.h>
#include <errno.h>

#ifdef _XBOX
#define SHADER_ROOT "game:\\Media\\Shaders\\"
#else
#define SHADER_ROOT "Shaders/"
#endif

namespace Graphics {

ShaderManager::ShaderManager()
    : m_pDevice(NULL), m_pActiveShader(NULL), m_pActiveEffect(NULL), m_numPasses(0), m_currentShaderID(SHADER_INVALID), m_hasFrameViewProj(false), m_shaderBegan(false)
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
    m_shaderBegan = false;
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

    char logBuf[512];
    sprintf(logBuf, "[ShaderManager] Loading shader %d from: %s\n", id, path);
    OutputDebugStringA(logBuf);

    FILE* fTest = fopen(path, "rb");
    if (fTest) {
        OutputDebugStringA("[ShaderManager] fopen SUCCESS - file exists\n");
        fclose(fTest);
    } else {
        char ferr[256];
        sprintf(ferr, "[ShaderManager] fopen FAILED errno=%d\n", errno);
        OutputDebugStringA(ferr);
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
        char errMsg[512];
        if (pErrorBuffer) {
            sprintf(errMsg, "[ShaderManager] ERROR loading %s: %s\n", path, (char*)pErrorBuffer->GetBufferPointer());
            OutputDebugStringA(errMsg);
            pErrorBuffer->Release();
        } else {
            sprintf(errMsg, "[ShaderManager] ERROR loading %s: NO ERROR BUFFER (file not found or access denied)\n", path);
            OutputDebugStringA(errMsg);
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
    if (id == SHADER_INVALID) {
        m_pActiveShader = NULL;
        m_pActiveEffect = NULL;
        m_currentShaderID = SHADER_INVALID;
        m_shaderBegan = false;
        return true;
    }

    if (!ValidateShader(id)) {
        char buf[256];
        sprintf(buf, "[ShaderManager] ERROR: Invalid shader ID=%d (valid range 0-%d)\n", (int)id, SHADER_COUNT-1);
        OutputDebugStringA(buf);
        if (m_effects.find(id) == m_effects.end()) {
            sprintf(buf, "[ShaderManager] ERROR: Shader %d not loaded in m_effects map\n", (int)id);
            OutputDebugStringA(buf);
        }
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
    m_shaderBegan = true;
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
    m_shaderBegan = false;
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
    m_shaderBegan = false;
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

    hr = LoadShader(SHADER_SPRITE, SHADER_ROOT "SpriteShader.fx", "SpriteBatchTech");
    if (FAILED(hr)) {
        OutputDebugStringA("[ShaderManager] WARNING: SPRITE shader not loaded, continuing\n");
    }

    hr = LoadShader(SHADER_SPRITE_CONSTANT_INSTANCED, SHADER_ROOT "SpriteConstantInstanced.fx", "SpriteBatchTech");
    if (FAILED(hr)) {
        OutputDebugStringA("[ShaderManager] WARNING: SPRITE_CONSTANT_INSTANCED shader not loaded, continuing\n");
    }

    hr = LoadShader(SHADER_RADIALMENU, SHADER_ROOT "RadialMenu.fx", "RadialMenu");
    if (FAILED(hr)) {
        OutputDebugStringA("[ShaderManager] WARNING: RADIALMENU shader not loaded, continuing\n");
    }

    hr = LoadShader(SHADER_UI, SHADER_ROOT "UI.fx", "UITech");
    if (FAILED(hr)) {
        OutputDebugStringA("[ShaderManager] WARNING: UI shader not loaded, continuing\n");
    }

    hr = LoadShader(SHADER_WORLD, SHADER_ROOT "World.fx", "WorldTech");
    if (FAILED(hr)) {
        OutputDebugStringA("[ShaderManager] WARNING: WORLD shader not loaded, continuing\n");
    } else {
        ID3DXEffect* pWorldFx = GetEffect(SHADER_WORLD);
        if (pWorldFx) {
            m_effects[SHADER_TERRAIN] = pWorldFx;
            pWorldFx->AddRef();
            Shader shader;
            shader.pEffect = pWorldFx;
            shader.hTechnique = pWorldFx->GetTechniqueByName("WorldTech");
            shader.hMatOrtho = pWorldFx->GetParameterByName(NULL, "matOrtho");
            shader.hTexture = pWorldFx->GetParameterByName(NULL, "g_texture");
            D3DXEFFECT_DESC desc;
            pWorldFx->GetDesc(&desc);
            for (UINT i = 0; i < desc.Parameters; i++) {
                D3DXHANDLE hParam = pWorldFx->GetParameter(NULL, i);
                D3DXPARAMETER_DESC paramDesc;
                pWorldFx->GetParameterDesc(hParam, &paramDesc);
                shader.hParams[paramDesc.Name] = hParam;
            }
            m_shaders[SHADER_TERRAIN] = shader;
            OutputDebugStringA("[ShaderManager] Shared World.fx effect for TERRAIN shader\n");
        }
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
        m_shaderBegan = false;
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
