#include "stdafx.h"
#include "BatchBuilder.h"
#include "RenderTypes.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

BatchBuilder::BatchBuilder()
    : m_device(NULL)
    , m_vertexBuffer(NULL)
    , m_maxVertices(65536)
    , m_currentShader(SHADER_INVALID)
    , m_currentTexture(NULL)
    , m_batchStartVertex(0)
{
}

BatchBuilder::~BatchBuilder() {
    Shutdown();
}

void BatchBuilder::Initialize(LPDIRECT3DDEVICE9 device) {
    m_device = device;

    HRESULT hr = device->CreateVertexBuffer(
        m_maxVertices * sizeof(SpriteVertex),
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        0,
        D3DPOOL_DEFAULT,
        &m_vertexBuffer,
        NULL);

    if (FAILED(hr)) {
        OutputDebugStringA("[BatchBuilder] ERROR: Failed to create vertex buffer\n");
    }

    m_vertices.reserve(m_maxVertices);
    OutputDebugStringA("[BatchBuilder] Initialized\n");
}

void BatchBuilder::Shutdown() {
    if (m_vertexBuffer) {
        m_vertexBuffer->Release();
        m_vertexBuffer = NULL;
    }
    m_batches.clear();
    m_vertices.clear();
}

void BatchBuilder::BeginFrame() {
    Clear();
}

void BatchBuilder::EndFrame() {
}

void BatchBuilder::BuildBatches(const std::vector<RenderCommand>& commands) {
    if (commands.empty()) return;

    for (const auto& cmd : commands) {
        CreateBatch(cmd);
    }

    FlushCurrentBatch();
}

void BatchBuilder::CreateBatch(const RenderCommand& cmd) {
    bool needsNewBatch = false;

    if (m_batches.empty()) {
        needsNewBatch = true;
    }
    else if (cmd.shaderID != m_currentShader || cmd.pTexture != m_currentTexture) {
        needsNewBatch = true;
    }

    if (needsNewBatch) {
        FlushCurrentBatch();

        RenderBatch batch;
        batch.shaderID = (ShaderID)cmd.shaderID;
        batch.texture = cmd.pTexture;
        batch.startVertex = (int)m_vertices.size();
        batch.minDepth = cmd.depth;
        batch.maxDepth = cmd.depth;

        m_currentShader = (ShaderID)cmd.shaderID;
        m_currentTexture = cmd.pTexture;
        m_batchStartVertex = batch.startVertex;

        m_batches.push_back(batch);
    }

    SpriteVertex vertices[4];
    vertices[0] = cmd.vertices[0];
    vertices[1] = cmd.vertices[1];
    vertices[2] = cmd.vertices[2];
    vertices[3] = cmd.vertices[3];

    for (int i = 0; i < 4; i++) {
        m_vertices.push_back(vertices[i]);
    }

    if (m_batches.size() > 0) {
        RenderBatch& currentBatch = m_batches.back();
        currentBatch.vertexCount += 4;
        currentBatch.primitiveCount += 2;
        currentBatch.minDepth = min(currentBatch.minDepth, cmd.depth);
        currentBatch.maxDepth = max(currentBatch.maxDepth, cmd.depth);
    }
}

void BatchBuilder::FlushCurrentBatch() {
    if (m_vertices.empty() || m_batches.empty()) return;

    if (m_vertexBuffer) {
        void* pData = NULL;
        HRESULT hr = m_vertexBuffer->Lock(0, m_vertices.size() * sizeof(SpriteVertex), &pData, D3DLOCK_DISCARD);
        if (SUCCEEDED(hr) && pData) {
            memcpy(pData, m_vertices.data(), m_vertices.size() * sizeof(SpriteVertex));
            m_vertexBuffer->Unlock();
        }
    }
}

void BatchBuilder::Clear() {
    m_batches.clear();
    m_vertices.clear();
    m_currentShader = SHADER_INVALID;
    m_currentTexture = NULL;
}

}