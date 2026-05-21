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
    , m_vertexBuffer(NULL)
    , m_maxVertices(65536)
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

    RenderBatch currentBatch;
    bool hasCurrentBatch = false;

    for (size_t i = 0; i < commands.size(); i++) {
        const RenderCommand& cmd = commands[i];

        bool needsNewBatch = !hasCurrentBatch
            || cmd.textureID != m_currentTexture
            || cmd.shaderID != m_currentShader
            || cmd.blendMode != m_currentBlend;

        if (needsNewBatch) {
            if (hasCurrentBatch) {
                currentBatch.vertexCount = (DWORD)m_vertices.size() - currentBatch.vertexOffset;
            }

            currentBatch.textureID = cmd.textureID;
            currentBatch.shaderID = cmd.shaderID;
            currentBatch.blendMode = cmd.blendMode;
            currentBatch.vertexOffset = (DWORD)m_vertices.size();
            currentBatch.vertexCount = 0;

            m_currentTexture = cmd.textureID;
            m_currentShader = cmd.shaderID;
            m_currentBlend = cmd.blendMode;
            hasCurrentBatch = true;

            m_batches.push_back(currentBatch);
        }

        float hw = cmd.width * 0.5f;
        float hh = cmd.height * 0.5f;

        SpriteVertex v[4];

        v[0].x = cmd.x;          v[0].y = cmd.y;          v[0].z = 0.0f;
        v[0].u = cmd.u0;         v[0].v = cmd.v0;
        v[0].color = cmd.color;
        v[0].padding[0] = 0; v[0].padding[1] = 0;

        v[1].x = cmd.x + cmd.width; v[1].y = cmd.y;          v[1].z = 0.0f;
        v[1].u = cmd.u1;         v[1].v = cmd.v0;
        v[1].color = cmd.color;
        v[1].padding[0] = 0; v[1].padding[1] = 0;

        v[2].x = cmd.x;          v[2].y = cmd.y + cmd.height; v[2].z = 0.0f;
        v[2].u = cmd.u0;         v[2].v = cmd.v1;
        v[2].color = cmd.color;
        v[2].padding[0] = 0; v[2].padding[1] = 0;

        v[3].x = cmd.x + cmd.width; v[3].y = cmd.y + cmd.height; v[3].z = 0.0f;
        v[3].u = cmd.u1;         v[3].v = cmd.v1;
        v[3].color = cmd.color;
        v[3].padding[0] = 0; v[3].padding[1] = 0;

        m_vertices.push_back(v[0]);
        m_vertices.push_back(v[1]);
        m_vertices.push_back(v[2]);
        m_vertices.push_back(v[3]);
    }

    if (hasCurrentBatch) {
        m_batches.back().vertexCount = (DWORD)m_vertices.size() - m_batches.back().vertexOffset;
    }

    FlushVertexStream();
}

void BatchBuilder::FlushVertexStream() {
    if (m_vertices.empty() || !m_vertexBuffer) return;

    void* pData = NULL;
    HRESULT hr = m_vertexBuffer->Lock(0, m_vertices.size() * sizeof(SpriteVertex), &pData, D3DLOCK_DISCARD);
    if (SUCCEEDED(hr) && pData) {
        memcpy(pData, m_vertices.data(), m_vertices.size() * sizeof(SpriteVertex));
        m_vertexBuffer->Unlock();
    }
}

void BatchBuilder::Clear() {
    m_batches.clear();
    m_vertices.clear();
    m_currentTexture = 0xFFFF;
    m_currentShader = 0xFFFF;
    m_currentBlend = 0xFF;
}

}
