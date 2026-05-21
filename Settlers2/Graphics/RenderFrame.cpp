#include "stdafx.h"
#include <stdio.h>
#include "RenderFrame.h"
#include "RenderQueue.h"
#include "SpriteRenderer.h"
#include "RenderDebugOverlay.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
    ::OutputDebugStringA(msg);
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
}

void RenderFrame::Execute() {
    if (!m_initialized) return;

    char buf[128];
    int cmdCount = m_renderQueue ? m_renderQueue->GetCommandCount() : -1;
    sprintf(buf, "[RenderFrame] Execute: queue=%p cmdCount=%d\n", m_renderQueue, cmdCount);
    OutputDebugStringA(buf);

    if (m_renderQueue) {
        m_renderQueue->Sort();
    }

    if (m_renderQueue && cmdCount > 0) {
        m_batchBuilder.BuildBatches(
            m_renderQueue->GetCommands(),
            cmdCount);
    }

    int batchCount = m_batchBuilder.GetBatchCount();
    sprintf(buf, "[RenderFrame] Execute: batches=%d spriteRenderer=%p\n", batchCount, m_spriteRenderer);
    OutputDebugStringA(buf);

    if (m_spriteRenderer && batchCount > 0) {
        m_spriteRenderer->Execute(m_batchBuilder);
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
