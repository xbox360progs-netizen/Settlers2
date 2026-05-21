#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include "RenderTypes.h"
#include "BatchBuilder.h"

namespace Graphics { class ShaderManager; }

namespace Graphics {

struct RenderStateCache {
    WORD currentTexture;
    WORD currentShader;
    BYTE currentBlend;

    DWORD alphaBlendEnable;
    DWORD srcBlend;
    DWORD destBlend;
    DWORD zEnable;
    DWORD zWriteEnable;
    DWORD cullMode;

    RenderStateCache()
        : currentTexture(0xFFFF), currentShader(0xFFFF), currentBlend(0xFF),
          alphaBlendEnable(0xFFFFFFFF), srcBlend(0xFFFFFFFF), destBlend(0xFFFFFFFF),
          zEnable(0xFFFFFFFF), zWriteEnable(0xFFFFFFFF), cullMode(0xFFFFFFFF) {}

    bool TextureChanged(WORD tex) const { return currentTexture != tex; }
    bool ShaderChanged(WORD sh) const { return currentShader != sh; }
    bool BlendChanged(BYTE blend) const { return currentBlend != blend; }

    void Update(WORD tex, WORD sh, BYTE blend) {
        currentTexture = tex;
        currentShader = sh;
        currentBlend = blend;
    }
};

class SpriteRenderer {
public:
    SpriteRenderer();
    ~SpriteRenderer();

    HRESULT Initialize(LPDIRECT3DDEVICE9 device, ShaderManager* shaderManager, int maxSprites = 4096);
    void Shutdown();

    void OnLostDevice();
    void OnResetDevice();

    void BeginFrame();
    void EndFrame();

    void Execute(const BatchBuilder& builder);

    LPDIRECT3DDEVICE9 GetDevice() const { return m_pDevice; }
    LPDIRECT3DVERTEXBUFFER9 GetVertexBuffer() const { return m_vertexBuffer; }
    LPDIRECT3DINDEXBUFFER9 GetIndexBuffer() const { return m_indexBuffer; }
    LPDIRECT3DVERTEXDECLARATION9 GetVertexDeclaration() const { return m_vertexDecl; }

    int GetDrawCalls() const { return m_drawCalls; }
    int GetTextureSwitches() const { return m_textureSwitches; }
    int GetShaderSwitches() const { return m_shaderSwitches; }
    int GetStateChanges() const { return m_stateChanges; }

private:
    void SetTexture(WORD textureID);
    void SetShader(WORD shaderID);
    void SetBlendMode(BYTE blendMode);

    LPDIRECT3DDEVICE9 m_pDevice;
    ShaderManager* m_pShaderManager;

    LPDIRECT3DVERTEXBUFFER9 m_vertexBuffer;
    LPDIRECT3DINDEXBUFFER9 m_indexBuffer;
    LPDIRECT3DVERTEXDECLARATION9 m_vertexDecl;

    int m_maxSprites;

    RenderStateCache m_stateCache;

    int m_drawCalls;
    int m_textureSwitches;
    int m_shaderSwitches;
    int m_stateChanges;
};

}
