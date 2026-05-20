#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include "RenderTypes.h"
#include "Material.h"

class ShaderManager;

class SpriteRenderer {
public:
    SpriteRenderer();
    ~SpriteRenderer();

    HRESULT Initialize(LPDIRECT3DDEVICE9 device, ShaderManager* shaderManager, int maxSprites = 4096);
    void Shutdown();

    void SetMaterialManager(MaterialManager* manager) { m_pMaterialManager = manager; }
    MaterialManager* GetMaterialManager() const { return m_pMaterialManager; }

    void OnLostDevice();
    void OnResetDevice();

    void BeginFrame();
    void EndFrame();
    void ResetBatchState();

    void SubmitSprite(const RenderCommand& cmd);

    void Flush(ShaderManager* pShader);
    void Flush();

    LPDIRECT3DDEVICE9 GetDevice() const { return m_pDevice; }
    LPDIRECT3DVERTEXBUFFER9 GetVertexBuffer() const { return m_pVertexBuffer; }
    LPDIRECT3DINDEXBUFFER9 GetIndexBuffer() const { return m_pIndexBuffer; }
    LPDIRECT3DVERTEXDECLARATION9 GetVertexDeclaration() const { return m_pVertexDecl; }

    int GetSpriteCount() const { return m_spriteCount; }
    int GetTotalVertexCount() const { return m_totalVertexCount; }
    void IncrementTotalVertexCount(int count) { m_totalVertexCount += count; }

    void PushCommand(const RenderCommand& cmd);

#ifdef _XBOX
    void SetAsyncCommandBuffer(IDirect3DCommandBuffer9* pBuffer, IDirect3DAsyncCommandBufferCall9* pAsyncCall);
    void FlushBatchesAsync();
#endif

private:
    void CreateQuad(float x, float y, float width, float height,
                    float u0, float v0, float u1, float v1,
                    DWORD color);

    void InternalDraw(const RenderCommand& cmd);

    static const int MAX_BUFFER_VERTICES = 65536;

    LPDIRECT3DDEVICE9 m_pDevice;
    ShaderManager* m_pShaderManager;
    MaterialManager* m_pMaterialManager;

    LPDIRECT3DVERTEXBUFFER9 m_pVB[2];
    LPDIRECT3DVERTEXBUFFER9 m_pGpuBufferA;
    LPDIRECT3DVERTEXBUFFER9 m_pGpuBufferB;
    LPDIRECT3DVERTEXBUFFER9 m_pVertexBuffer;
    int m_activeBuffer;

    LPDIRECT3DINDEXBUFFER9 m_pIndexBuffer;
    LPDIRECT3DVERTEXDECLARATION9 m_pVertexDecl;

    __declspec(align(16)) SpriteVertex* m_pStagingBuffer;
    std::vector<RenderCommand> m_commands;

    DWORD m_totalVertexCount;
    DWORD m_totalIndexCount;
    int m_maxSprites;
    int m_spriteCount;

#ifdef _XBOX
    IDirect3DCommandBuffer9* m_pAsyncCommandBuffer;
    IDirect3DAsyncCommandBufferCall9* m_pAsyncCall;
    IDirect3DQuery9* m_pGpuFence;
    bool m_isFirstFlush;
#endif
};