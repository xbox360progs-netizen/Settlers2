#include "stdafx.h"
#include "GPUDrivenRenderer.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

GPUDrivenRenderer::GPUDrivenRenderer()
    : m_pDevice(NULL), m_pInstanceBuffer(NULL), m_pQuadIndexBuffer(NULL),
      m_pQuadVertexBuffer(NULL), m_pInstancedDecl(NULL),
      m_instancingEnabled(true), m_textureAtlasesEnabled(true),
      m_maxInstancesPerBatch(4096), m_currentInstanceCount(0), m_drawCallCount(0),
      m_atlasBatchSize(1024) {
}

GPUDrivenRenderer::~GPUDrivenRenderer() {
    Shutdown();
}

void GPUDrivenRenderer::Initialize(LPDIRECT3DDEVICE9 pDevice) {
    m_pDevice = pDevice;

    if (m_pInstanceBuffer) m_pInstanceBuffer->Release();
    int bufferSize = m_maxInstancesPerBatch * sizeof(SpriteInstanceData);
    HRESULT hr = m_pDevice->CreateVertexBuffer(
        bufferSize, D3DUSAGE_WRITEONLY,
        0, D3DPOOL_DEFAULT, &m_pInstanceBuffer, NULL
    );

    CreateQuadGeometry();
    CreateInstancedDeclaration();

    char buf[256];
    sprintf(buf, "[GPUDriven] Initialized with max %d instances per batch\n", m_maxInstancesPerBatch);
    OutputDebugStringA(buf);
}

void GPUDrivenRenderer::Shutdown() {
    if (m_pInstanceBuffer) { m_pInstanceBuffer->Release(); m_pInstanceBuffer = NULL; }
    if (m_pQuadIndexBuffer) { m_pQuadIndexBuffer->Release(); m_pQuadIndexBuffer = NULL; }
    if (m_pQuadVertexBuffer) { m_pQuadVertexBuffer->Release(); m_pQuadVertexBuffer = NULL; }
    if (m_pInstancedDecl) { m_pInstancedDecl->Release(); m_pInstancedDecl = NULL; }

    m_instanceBuffer.clear();
    m_atlasBatches.clear();
}

void GPUDrivenRenderer::CreateQuadGeometry() {
    if (!m_pDevice) return;

    struct Vertex { float x, y, z; float u, v; };

    Vertex quadVerts[4] = {
        { 0, 0, 0, 0, 0 },
        { 1, 0, 0, 1, 0 },
        { 1, 1, 0, 1, 1 },
        { 0, 1, 0, 0, 1 }
    };

    WORD quadIndices[6] = { 0, 1, 2, 0, 2, 3 };

    m_pDevice->CreateVertexBuffer(4 * sizeof(Vertex), 0, 0, D3DPOOL_DEFAULT, &m_pQuadVertexBuffer, NULL);
    m_pDevice->CreateIndexBuffer(6 * sizeof(WORD), 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_pQuadIndexBuffer, NULL);

    void* pData;
    m_pQuadVertexBuffer->Lock(0, 0, &pData, 0);
    memcpy(pData, quadVerts, sizeof(quadVerts));
    m_pQuadVertexBuffer->Unlock();

    m_pQuadIndexBuffer->Lock(0, 0, &pData, 0);
    memcpy(pData, quadIndices, sizeof(quadIndices));
    m_pQuadIndexBuffer->Unlock();
}

void GPUDrivenRenderer::CreateInstancedDeclaration() {
    if (!m_pDevice) return;

    D3DVERTEXELEMENT9 decl[] = {
        { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        { 1, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
        { 1, 16, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
        { 1, 32, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 1 },
        D3DDECL_END()
    };

    m_pDevice->CreateVertexDeclaration(decl, &m_pInstancedDecl);
}

void GPUDrivenRenderer::BeginInstancedBatch(int maxInstances) {
    m_instanceBuffer.clear();
    m_instanceBuffer.reserve(maxInstances);
    m_currentInstanceCount = 0;
}

void GPUDrivenRenderer::AddInstance(const SpriteInstanceData& instance) {
    if (m_currentInstanceCount >= m_maxInstancesPerBatch) {
        OutputDebugStringA("[GPUDriven] WARNING: Instance buffer full, dropping instance\n");
        return;
    }
    m_instanceBuffer.push_back(instance);
    m_currentInstanceCount++;
}

void GPUDrivenRenderer::EndInstancedBatch() {
    if (m_instanceBuffer.empty() || !m_pDevice) return;

    UploadInstanceData();

    char buf[256];
    sprintf(buf, "[GPUDriven] Instanced batch complete: %d instances\n", m_currentInstanceCount);
    OutputDebugStringA(buf);

    m_drawCallCount++;
}

void GPUDrivenRenderer::BeginTextureAtlasBatch(int maxSprites) {
    m_atlasBatches.clear();
    AtlasBatch batch;
    batch.atlasIndex = -1;
    batch.spriteCount = 0;
    batch.sprites.reserve(maxSprites);
    m_atlasBatches.push_back(batch);
}

void GPUDrivenRenderer::AddAtlasSprite(int atlasIndex, int spriteIndex, float x, float y, float depth, DWORD color) {
    if (m_atlasBatches.empty()) return;

    AtlasBatch& currentBatch = m_atlasBatches.back();

    if (currentBatch.atlasIndex != -1 && currentBatch.atlasIndex != atlasIndex) {
        AtlasBatch newBatch;
        newBatch.atlasIndex = atlasIndex;
        newBatch.spriteCount = 0;
        newBatch.sprites.reserve(m_atlasBatchSize);
        m_atlasBatches.push_back(newBatch);
    }

    if (currentBatch.atlasIndex == -1) {
        currentBatch.atlasIndex = atlasIndex;
    }

    SpriteInstanceData sprite;
    sprite.posX = x;
    sprite.posY = y;
    sprite.depth = depth;
    sprite.color = color;
    sprite.u0 = 0; sprite.v0 = 0;
    sprite.u1 = 1; sprite.v1 = 1;
    sprite.width = 1; sprite.height = 1;

    currentBatch.sprites.push_back(sprite);
    currentBatch.spriteCount++;
}

void GPUDrivenRenderer::EndTextureAtlasBatch() {
    char buf[256];
    sprintf(buf, "[GPUDriven] Atlas batch complete: %d atlas groups, total sprites=%d\n",
            (int)m_atlasBatches.size(), (int)m_instanceBuffer.size());
    OutputDebugStringA(buf);

    for (size_t i = 0; i < m_atlasBatches.size(); i++) {
        sprintf(buf, "  Atlas[%d]: %d sprites\n", m_atlasBatches[i].atlasIndex, m_atlasBatches[i].spriteCount);
        OutputDebugStringA(buf);
        m_drawCallCount++;
    }

    m_atlasBatches.clear();
}

void GPUDrivenRenderer::UploadInstanceData() {
    if (!m_pInstanceBuffer || m_instanceBuffer.empty()) return;

    void* pData;
    HRESULT hr = m_pInstanceBuffer->Lock(0, 0, &pData, 0);

    if (SUCCEEDED(hr)) {
        memcpy(pData, m_instanceBuffer.data(), m_instanceBuffer.size() * sizeof(SpriteInstanceData));
        m_pInstanceBuffer->Unlock();
    }
}

void GPUDrivenRenderer::ResetCounters() {
    m_currentInstanceCount = 0;
    m_drawCallCount = 0;
    m_instanceBuffer.clear();
    m_atlasBatches.clear();
}

}