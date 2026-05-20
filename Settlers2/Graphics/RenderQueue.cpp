#include "stdafx.h"
#include "RenderQueue.h"
#include "Material.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

RenderQueue::RenderQueue()
    : m_pDevice(NULL)
    , m_materialManager(NULL)
    , m_batchCount(0)
    , m_drawCallCount(0)
    , m_maxCommands(8192)
{
}

RenderQueue::~RenderQueue() {
    Shutdown();
}

void RenderQueue::Initialize(LPDIRECT3DDEVICE9 pDevice) {
    m_pDevice = pDevice;

    m_opaqueQueue.reserve(m_maxCommands);
    m_transparentQueue.reserve(m_maxCommands);
    m_uiQueue.reserve(m_maxCommands);

    char buf[256];
    sprintf(buf, "[RenderQueue] Initialized with max %d commands per layer\n", m_maxCommands);
    OutputDebugStringA(buf);
}

void RenderQueue::Shutdown() {
    Clear();
    m_pDevice = NULL;
    m_materialManager = NULL;
}

void RenderQueue::BeginFrame() {
    m_opaqueQueue.clear();
    m_transparentQueue.clear();
    m_uiQueue.clear();
    m_batches.clear();
    m_batchCount = 0;
    m_drawCallCount = 0;
}

void RenderQueue::EndFrame() {
    char buf[256];
    sprintf(buf, "[RenderQueue] Frame: opaque=%d, transparent=%d, ui=%d, batches=%d\n",
            (int)m_opaqueQueue.size(), (int)m_transparentQueue.size(),
            (int)m_uiQueue.size(), m_batchCount);
    OutputDebugStringA(buf);
}

void RenderQueue::Submit(const SpriteCommand& cmd) {
    RenderCommand renderCmd;
    renderCmd.materialID = cmd.material;
    renderCmd.x = cmd.x;
    renderCmd.y = cmd.y;
    renderCmd.width = cmd.width;
    renderCmd.height = cmd.height;
    renderCmd.u0 = cmd.u0;
    renderCmd.v0 = cmd.v0;
    renderCmd.u1 = cmd.u1;
    renderCmd.v1 = cmd.v1;
    renderCmd.color = cmd.color;
    renderCmd.depth = cmd.depth;
    renderCmd.isUI = cmd.isUI;

    SubmitBatch(renderCmd);
}

void RenderQueue::SubmitBatch(const RenderCommand& cmd) {
    if (cmd.isUI) {
        if ((int)m_uiQueue.size() < m_maxCommands) {
            m_uiQueue.push_back(cmd);
        }
    } else if (cmd.depth > 0.5f) {
        if ((int)m_opaqueQueue.size() < m_maxCommands) {
            m_opaqueQueue.push_back(cmd);
        }
    } else {
        if ((int)m_transparentQueue.size() < m_maxCommands) {
            m_transparentQueue.push_back(cmd);
        }
    }
}

int RenderQueue::GetCommandCount(RenderLayer layer) const {
    switch (layer) {
    case LAYER_OPAQUE: return (int)m_opaqueQueue.size();
    case LAYER_TRANSPARENT: return (int)m_transparentQueue.size();
    case LAYER_UI: return (int)m_uiQueue.size();
    default: return 0;
    }
}

int RenderQueue::GetTotalCommandCount() const {
    return (int)m_opaqueQueue.size() + (int)m_transparentQueue.size() + (int)m_uiQueue.size();
}

void RenderQueue::Sort(RenderQueueSortMode mode) {
    SortQueue(m_opaqueQueue, mode);
    SortQueue(m_transparentQueue, mode);
    SortQueue(m_uiQueue, mode);
}

void RenderQueue::SortQueue(std::vector<RenderCommand>& queue, RenderQueueSortMode mode) {
    switch (mode) {
    case SORT_BY_DEPTH:
        std::sort(queue.begin(), queue.end(),
                  [](const RenderCommand& a, const RenderCommand& b) {
                      return a.depth > b.depth;
                  });
        break;
    case SORT_BY_SHADER:
        std::sort(queue.begin(), queue.end(),
                  [](const RenderCommand& a, const RenderCommand& b) {
                      return a.shaderID < b.shaderID;
                  });
        break;
    case SORT_BY_TEXTURE:
        std::sort(queue.begin(), queue.end(),
                  [](const RenderCommand& a, const RenderCommand& b) {
                      return a.pTexture < b.pTexture;
                  });
        break;
    case SORT_BY_DEPTH_THEN_SHADER_THEN_TEXTURE:
        std::sort(queue.begin(), queue.end(),
                  [](const RenderCommand& a, const RenderCommand& b) {
                      if (a.depth != b.depth) return a.depth > b.depth;
                      if (a.shaderID != b.shaderID) return a.shaderID < b.shaderID;
                      return a.pTexture < b.pTexture;
                  });
        break;
    default:
        break;
    }
}

void RenderQueue::Batch() {
    m_batches.clear();
    m_batchCount = 0;

    CreateBatches(m_opaqueQueue);
    CreateBatches(m_transparentQueue);
    CreateBatches(m_uiQueue);
}

void RenderQueue::CreateBatches(std::vector<RenderCommand>& queue) {
    if (queue.empty()) return;

    int startIndex = 0;
    int currentShader = queue[0].shaderID;
    void* currentTexture = queue[0].pTexture;

    for (size_t i = 1; i <= queue.size(); i++) {
        bool endOfBatch = (i == queue.size()) ||
                          (queue[i].shaderID != currentShader) ||
                          (queue[i].pTexture != currentTexture);

        if (endOfBatch) {
            DrawBatch batch;
            batch.shaderID = currentShader;
            batch.texture = currentTexture;
            batch.startIndex = startIndex;
            batch.commandCount = (int)(i - startIndex);
            batch.minDepth = queue[startIndex].depth;
            batch.maxDepth = queue[startIndex].depth;

            for (int j = startIndex; j < (int)i; j++) {
                if (queue[j].depth < batch.minDepth) batch.minDepth = queue[j].depth;
                if (queue[j].depth > batch.maxDepth) batch.maxDepth = queue[j].depth;
            }

            m_batches.push_back(batch);
            m_batchCount++;
            m_drawCallCount++;

            if (i < queue.size()) {
                startIndex = (int)i;
                currentShader = queue[i].shaderID;
                currentTexture = queue[i].pTexture;
            }
        }
    }
}

void RenderQueue::Clear() {
    m_opaqueQueue.clear();
    m_transparentQueue.clear();
    m_uiQueue.clear();
    m_batches.clear();
}

void RenderQueue::DispatchOpaque(void* backend) {
    if (!backend) return;
    m_drawCallCount += (int)m_opaqueQueue.size();
}

void RenderQueue::DispatchTransparent(void* backend) {
    if (!backend) return;
    m_drawCallCount += (int)m_transparentQueue.size();
}

void RenderQueue::DispatchUI(void* backend) {
    if (!backend) return;
    m_drawCallCount += (int)m_uiQueue.size();
}

void RenderQueue::DispatchAll(void* backend) {
    DispatchOpaque(backend);
    DispatchTransparent(backend);
    DispatchUI(backend);
}

}