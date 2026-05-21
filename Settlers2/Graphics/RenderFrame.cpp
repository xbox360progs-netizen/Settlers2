#include "stdafx.h"
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

    m_batchBuilder.Initialize(pDevice);

    m_initialized = true;
    OutputDebugStringA("[RenderFrame] Initialized\n");
}

void RenderFrame::Shutdown() {
    if (m_pBackBuffer) { m_pBackBuffer->Release(); m_pBackBuffer = NULL; }
    m_batchBuilder.Shutdown();
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

    if (m_renderQueue) {
        m_renderQueue->Sort();
    }

    if (m_renderQueue && m_renderQueue->GetCommandCount() > 0) {
        m_batchBuilder.BuildBatches(
            m_renderQueue->GetCommands(),
            m_renderQueue->GetCommandCount());
    }

    if (m_spriteRenderer && m_batchBuilder.GetBatchCount() > 0) {
        m_spriteRenderer->Execute(
            m_batchBuilder.GetBatches(),
            m_batchBuilder.GetBatchCount());
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
