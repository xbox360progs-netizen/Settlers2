#include "stdafx.h"
#include <stdio.h>
#include "RenderFrame.h"
#include "RenderQueue.h"
#include "SpriteRenderer.h"
#include "RenderDebugOverlay.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

RenderFrame::RenderFrame()
    : m_pDevice(NULL)
    , m_renderQueue(NULL)
    , m_spriteRenderer(NULL)
    , m_tileRenderer(NULL)
    , m_gpuTimer(NULL)
    , m_debugOverlay(NULL)
    , m_pBackBuffer(NULL)
    , m_initialized(false)
{
}

RenderFrame::~RenderFrame() {
    Shutdown();
}

void RenderFrame::Initialize(LPDIRECT3DDEVICE9 pDevice) {
    if (m_initialized) return;

    m_pDevice = pDevice;
    pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer);

    m_initialized = true;
    OutputDebugStringA("[RenderFrame] Initialized\n");
}

void RenderFrame::Shutdown() {
    if (m_pBackBuffer) { m_pBackBuffer->Release(); m_pBackBuffer = NULL; }
    m_initialized = false;
}

void RenderFrame::BeginFrame() {
    if (m_gpuTimer) {
        m_gpuTimer->BeginFrame();
    }

    m_batchBuilder.BeginFrame();

    if (m_spriteRenderer) {
        m_spriteRenderer->BeginFrame();
    }
}

void RenderFrame::Execute() {
    if (!m_initialized) {
        OutputDebugStringA("[RenderFrame] Execute SKIP: not initialized\n");
        return;
    }

    int cmdCount = 0;

    if (m_renderQueue) {
        m_renderQueue->Sort();
        cmdCount = m_renderQueue->GetCommandCount();
        char buf[256];
        sprintf(buf, "[RenderFrame] Execute: renderQueue=%p, cmdCount=%d\n", m_renderQueue, cmdCount);
        OutputDebugStringA(buf);
    } else {
        OutputDebugStringA("[RenderFrame] Execute: m_renderQueue IS NULL!\n");
    }

    if (cmdCount > 0) {
        m_batchBuilder.BuildBatches(
            m_renderQueue->GetCommands(),
            cmdCount);
    }

    int batchCount = m_batchBuilder.GetBatchCount();
    char buf2[256];
    sprintf(buf2, "[RenderFrame] Execute: batchCount=%d, spriteRenderer=%p\n", batchCount, m_spriteRenderer);
    OutputDebugStringA(buf2);

    if (m_spriteRenderer && batchCount > 0) {
        OutputDebugStringA("[RenderFrame] Execute: calling SpriteRenderer::Execute()\n");
        m_spriteRenderer->Execute(m_batchBuilder);
    } else {
        OutputDebugStringA("[RenderFrame] Execute: SKIP SpriteRenderer (NULL or no batches)\n");
    }

    if (m_debugOverlay) {
        m_debugOverlay->RenderOverlay(1280, 720);
    }
}

void RenderFrame::EndFrame() {
    if (m_gpuTimer) {
        m_gpuTimer->EndFrame();
    }
}

}
