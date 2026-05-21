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
}

void RenderFrame::Execute() {
    if (!m_initialized) return;

    if (m_renderQueue) {
        m_renderQueue->Sort();
        m_renderQueue->Batch();
    }

    if (m_spriteRenderer && m_renderQueue) {
        m_spriteRenderer->Execute(
            m_renderQueue->GetBuiltBatches(),
            m_renderQueue->GetBuiltBatchCount());
    }

    if (m_debugOverlay) {
        m_debugOverlay->Render();
    }
}

void RenderFrame::EndFrame() {
    if (m_gpuTimer) {
        m_gpuTimer->EndFrame();
    }

    if (m_renderQueue) {
        m_renderQueue->EndFrame();
    }
}

}
