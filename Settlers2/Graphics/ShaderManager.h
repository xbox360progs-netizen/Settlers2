#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <map>
#include <string>
#include "RenderTypes.h"

enum ShaderID {
    SHADER_INVALID = -1,
    SHADER_SPRITE = 0,
    SHADER_SPRITE_CONSTANT_INSTANCED = 1,
    SHADER_RADIALMENU = 2,
    SHADER_UI = 3,
    SHADER_TERRAIN = 4,
    SHADER_WORLD = 5,
    SHADER_COUNT
};

namespace Graphics {

class ShaderManager {
public:
    struct Shader {
        LPD3DXEFFECT pEffect;
        D3DXHANDLE hTechnique;
        D3DXHANDLE hMatOrtho;
        D3DXHANDLE hTexture;
        std::map<std::string, D3DXHANDLE> hParams;
    };

    ShaderManager();
    ~ShaderManager();

    HRESULT Initialize(LPDIRECT3DDEVICE9 device);
    void Shutdown();

    HRESULT LoadShader(ShaderID id, const char* filepath, const char* techniqueName = "SpriteBatchTech");
    bool SetActiveShader(ShaderID id);

    Shader* GetActiveShader() { return m_pActiveShader; }
    Shader* GetShader(ShaderID id);

    void BeginShader();
    void BeginPass(UINT pass = 0);
    void EndPass();
    void EndShader();

    UINT GetNumPasses() const { return m_numPasses; }

    void Commit();

    void SetMatrix(const char* paramName, const float* matrix);
    void SetVector(const char* paramName, const float* vector);
    void SetTexture(const char* paramName, LPDIRECT3DBASETEXTURE9 pTexture);

    void OnLostDevice();
    void OnResetDevice();

    bool HasShader(ShaderID id) const;

    HRESULT LoadAll();

    void Prepare(ShaderID id, const D3DXMATRIX* pViewProj = NULL);
    void EndCurrent();

    void SetGlobalUniforms(ShaderID id, const D3DXMATRIX* pViewProj);
    void SetLocalUniforms(LPDIRECT3DTEXTURE9 pTexture, float depth);
    void UpdateConstants(LPDIRECT3DTEXTURE9 pTexture, const D3DXMATRIX* pWorldMatrix = NULL);

    void SetFrameViewProj(const D3DXMATRIX* pViewProj);

    void UpdateGlobalMatrices(const D3DXMATRIX* pView, const D3DXMATRIX* pProj);

    void SetShaderMatrix(ShaderID id, const D3DXMATRIX* pMatrix);

    bool Init();
    bool LoadBaseShaders();

    ID3DXEffect* GetEffect(ShaderID id);

    const D3DXMATRIX& GetFrameViewProj() const {
        return m_shaderMatrices[0];
    }

    const D3DXMATRIX& GetShaderMatrix(ShaderID id) const {
        return m_shaderMatrices[id];
    }

    ShaderID GetCurrentShaderID() const { return m_currentShaderID; }
    
    bool IsShaderBegan() const { return m_shaderBegan; }

    void CommitChanges();
    bool ValidateShader(ShaderID id) const;
    void ApplyShader(int shaderID);

    LPDIRECT3DDEVICE9 GetDevice() const { return m_pDevice; }

private:
    LPDIRECT3DDEVICE9 m_pDevice;

    std::map<ShaderID, Shader> m_shaders;
    Shader* m_pActiveShader;
    ID3DXEffect* m_pActiveEffect;
    ShaderID m_currentShaderID;
    UINT m_numPasses;

    D3DXMATRIX m_shaderMatrices[SHADER_COUNT];
    bool m_hasFrameViewProj;

    D3DXMATRIX m_cachedView;
    D3DXMATRIX m_cachedProj;

    std::map<ShaderID, ID3DXEffect*> m_effects;

    HRESULT LoadInternal(ShaderID id, const char* path, const char* technique);

    CRITICAL_SECTION m_cs;
    
    bool m_shaderBegan;  // Track if BeginShader was called
};

}
