#include "stdafx.h"
#include "BatchBuilder.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

BatchBuilder::BatchBuilder()
    : m_device(NULL)
    , m_batchCount(0)
    , m_vertexWritePos(0)
    , m_indexWritePos(0)
    , m_vertexBuffer(NULL)
    , m_indexBuffer(NULL)
    , m_currentTexture(0xFFFF)
    , m_currentShader(0xFFFF)
    , m_currentBlend(0xFF)
{
}

BatchBuilder::~BatchBuilder() {
    Shutdown();
}

void BatchBuilder::Initialize(LPDIRECT3DDEVICE9 device) {
    m_device = device;

    HRESULT hr = device->CreateVertexBuffer(
        MAX_VERTICES * sizeof(SpriteVertex),
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        0,
        D3DPOOL_DEFAULT,
        &m_vertexBuffer,
        NULL);

    if (FAILED(hr)) {
        OutputDebugStringA("[BatchBuilder] ERROR: Failed to create vertex buffer\n");
    }

    hr = device->CreateIndexBuffer(
        MAX_VERTICES * sizeof(uint32_t),
        D3DUSAGE_WRITEONLY,
        D3DFMT_INDEX32,
        D3DPOOL_DEFAULT,
        &m_indexBuffer,
        NULL);

    if (FAILED(hr)) {
        OutputDebugStringA("[BatchBuilder] WARNING: Failed to create index buffer, using vertex buffer only\n");
    }

    char buf[256];
    sprintf(buf, "[BatchBuilder] Initialized: maxVertices=%d, maxBatches=%d\n", MAX_VERTICES, MAX_BATCHES);
    OutputDebugStringA(buf);
}

void BatchBuilder::Shutdown() {
    if (m_vertexBuffer) {
        m_vertexBuffer->Release();
        m_vertexBuffer = NULL;
    }
    if (m_indexBuffer) {
        m_indexBuffer->Release();
        m_indexBuffer = NULL;
    }
    m_batchCount = 0;
    m_vertexWritePos = 0;
    m_indexWritePos = 0;
}

void BatchBuilder::BeginFrame() {
    Clear();
}

void BatchBuilder::EndFrame() {
}

void BatchBuilder::BuildBatches(const RenderCommand* commands, uint32_t commandCount) {
    if (commandCount == 0 || !commands) return;

    m_batchCount = 0;
    m_vertexWritePos = 0;
    m_indexWritePos = 0;

    m_currentTexture = 0xFFFF;
    m_currentShader = 0xFFFF;
    m_currentBlend = 0xFF;

    RenderBatch* currentBatch = NULL;

    for (uint32_t i = 0; i < commandCount; i++) {
        const RenderCommand& cmd = commands[i];

        bool needsNewBatch = (m_batchCount == 0)
            || (cmd.textureID != m_currentTexture)
            || (cmd.shaderID != m_currentShader)
            || (cmd.blendMode != m_currentBlend);

        if (needsNewBatch) {
            if (m_batchCount >= MAX_BATCHES) {
                OutputDebugStringA("[BatchBuilder] WARNING: Max batches exceeded\n");
                break;
            }

            currentBatch = &m_batches[m_batchCount++];
            currentBatch->textureID = cmd.textureID;
            currentBatch->shaderID = cmd.shaderID;
            currentBatch->blendMode = cmd.blendMode;
            currentBatch->startIndex = m_indexWritePos;
            currentBatch->indexCount = 0;

            m_currentTexture = cmd.textureID;
            m_currentShader = cmd.shaderID;
            m_currentBlend = cmd.blendMode;
        }

        if (m_vertexWritePos + 4 > MAX_VERTICES) {
            OutputDebugStringA("[BatchBuilder] WARNING: Max vertices exceeded\n");
            break;
        }

        float hw = cmd.width * 0.5f;
        float hh = cmd.height * 0.5f;

        SpriteVertex* dst = m_vertexPool + m_vertexWritePos;
        uint32_t baseIdx = m_vertexWritePos;

        dst[0].x = cmd.x;                  dst[0].y = cmd.y;                  dst[0].z = 0.0f;
        dst[0].u = cmd.u0;                 dst[0].v = cmd.v0;
        dst[0].color = cmd.color;
        dst[0].padding[0] = 0; dst[0].padding[1] = 0;

        dst[1].x = cmd.x + cmd.width;      dst[1].y = cmd.y;                  dst[1].z = 0.0f;
        dst[1].u = cmd.u1;                 dst[1].v = cmd.v0;
        dst[1].color = cmd.color;
        dst[1].padding[0] = 0; dst[1].padding[1] = 0;

        dst[2].x = cmd.x;                  dst[2].y = cmd.y + cmd.height;     dst[2].z = 0.0f;
        dst[2].u = cmd.u0;                 dst[2].v = cmd.v1;
        dst[2].color = cmd.color;
        dst[2].padding[0] = 0; dst[2].padding[1] = 0;

        dst[3].x = cmd.x + cmd.width;      dst[3].y = cmd.y + cmd.height;     dst[3].z = 0.0f;
        dst[3].u = cmd.u1;                 dst[3].v = cmd.v1;
        dst[3].color = cmd.color;
        dst[3].padding[0] = 0; dst[3].padding[1] = 0;

        uint32_t* idx = m_indexPool + m_indexWritePos;
        idx[0] = baseIdx + 0;
        idx[1] = baseIdx + 1;
        idx[2] = baseIdx + 2;
        idx[3] = baseIdx + 2;
        idx[4] = baseIdx + 1;
        idx[5] = baseIdx + 3;

        m_vertexWritePos += 4;
        m_indexWritePos += 6;

        currentBatch->indexCount += 6;
    }

    FlushVertexStream();
}

void BatchBuilder::FlushVertexStream() {
    if (m_vertexWritePos == 0 || !m_vertexBuffer) return;

    void* pData = NULL;
    HRESULT hr = m_vertexBuffer->Lock(0, m_vertexWritePos * sizeof(SpriteVertex), &pData, D3DLOCK_DISCARD);
    if (SUCCEEDED(hr) && pData) {
        memcpy(pData, m_vertexPool, m_vertexWritePos * sizeof(SpriteVertex));
        m_vertexBuffer->Unlock();
    }

    if (m_indexWritePos > 0 && m_indexBuffer) {
        hr = m_indexBuffer->Lock(0, m_indexWritePos * sizeof(uint32_t), &pData, D3DLOCK_DISCARD);
        if (SUCCEEDED(hr) && pData) {
            memcpy(pData, m_indexPool, m_indexWritePos * sizeof(uint32_t));
            m_indexBuffer->Unlock();
        }
    }
}

void BatchBuilder::Clear() {
    m_batchCount = 0;
    m_vertexWritePos = 0;
    m_indexWritePos = 0;
    m_currentTexture = 0xFFFF;
    m_currentShader = 0xFFFF;
    m_currentBlend = 0xFF;
}

}
