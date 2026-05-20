#include "stdafx.h"
#include "PostFXChain.h"
#include "GPUTimer.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

PostFXChain::PostFXChain()
    : m_device(NULL)
    , m_gpuTimer(NULL)
    , m_inputTexture(NULL)
    , m_outputTexture(NULL)
    , m_outputSurface(NULL)
    , m_debugOutput(false)
    , m_gpuTimerIndex(-1)
{
}

PostFXChain::~PostFXChain() {
    Shutdown();
}

void PostFXChain::Initialize(IDirect3DDevice9* device, GPUTimer* timer) {
    m_device = device;
    m_gpuTimer = timer;

    D3DSURFACE_DESC desc;
    if (m_inputTexture) {
        m_inputTexture->GetLevelDesc(0, &desc);
    } else {
        desc.Width = 1280;
        desc.Height = 720;
        desc.Format = D3DFMT_A8R8G8B8;
    }

    HRESULT hr = m_device->CreateTexture(
        desc.Width, desc.Height, 1, D3DUSAGE_RENDERTARGET,
        desc.Format, D3DPOOL_DEFAULT, &m_outputTexture, NULL);

    if (SUCCEEDED(hr)) {
        m_outputTexture->GetSurfaceLevel(0, &m_outputSurface);
    }

    OutputDebugStringA("[PostFXChain] Initialized\n");
}

void PostFXChain::Shutdown() {
    for (auto pass : m_passes) {
        pass->Shutdown();
        delete pass;
    }
    m_passes.clear();
    m_params.clear();

    if (m_outputSurface) {
        m_outputSurface->Release();
        m_outputSurface = NULL;
    }
    if (m_outputTexture) {
        m_outputTexture->Release();
        m_outputTexture = NULL;
    }

    m_device = NULL;
    m_gpuTimer = NULL;
}

IPostFXPass* PostFXChain::AddPass(PostFXType type, const PostFXParams& params) {
    OutputDebugStringA("[PostFXChain] AddPass called - base implementation\n");
    return NULL;
}

void PostFXChain::RemovePass(PostFXType type) {
    for (auto it = m_passes.begin(); it != m_passes.end(); ) {
        if ((*it)->GetType() == type) {
            (*it)->Shutdown();
            delete *it;
            it = m_passes.erase(it);
        } else {
            ++it;
        }
    }
}

void PostFXChain::RemovePass(const char* name) {
    for (auto it = m_passes.begin(); it != m_passes.end(); ) {
        if (strcmp((*it)->GetName(), name) == 0) {
            (*it)->Shutdown();
            delete *it;
            it = m_passes.erase(it);
        } else {
            ++it;
        }
    }
}

IPostFXPass* PostFXChain::GetPass(PostFXType type) {
    for (auto pass : m_passes) {
        if (pass->GetType() == type) {
            return pass;
        }
    }
    return NULL;
}

IPostFXPass* PostFXChain::GetPass(const char* name) {
    for (auto pass : m_passes) {
        if (strcmp(pass->GetName(), name) == 0) {
            return pass;
        }
    }
    return NULL;
}

void PostFXChain::SetEnabled(PostFXType type, bool enabled) {
    IPostFXPass* pass = GetPass(type);
    if (pass) {
        pass->SetEnabled(enabled);
    }
}

void PostFXChain::SetParameters(PostFXType type, const PostFXParams& params) {
    IPostFXPass* pass = GetPass(type);
    if (pass) {
        pass->SetParameters(params);
    }
}

void PostFXChain::Execute() {
    if (!m_device || m_passes.empty()) return;

    if (m_gpuTimer) {
        m_gpuTimerIndex = m_gpuTimer->StartTimer("PostFXChain");
    }

    IDirect3DSurface9* pOldTarget = NULL;
    m_device->GetRenderTarget(0, &pOldTarget);

    for (auto pass : m_passes) {
        if (!pass->IsEnabled()) continue;
        pass->Execute();
    }

    if (pOldTarget) {
        pOldTarget->Release();
    }

    if (m_gpuTimer && m_gpuTimerIndex >= 0) {
        m_gpuTimer->EndTimer(m_gpuTimerIndex);
        m_gpuTimerIndex = -1;
    }
}

void PostFXChain::SetInputTexture(IDirect3DTexture9* texture) {
    m_inputTexture = texture;
}

void PostFXChain::RenderFullscreenQuad() {
    if (!m_device) return;

    D3DXVECTOR3 vertices[] = {
        D3DXVECTOR3(-1.0f, -1.0f, 0.0f),
        D3DXVECTOR3(1.0f, -1.0f, 0.0f),
        D3DXVECTOR3(-1.0f, 1.0f, 0.0f),
        D3DXVECTOR3(1.0f, 1.0f, 0.0f)
    };

    WORD indices[] = { 0, 1, 2, 1, 3, 2 };

    m_device->DrawIndexedPrimitiveUP(
        D3DPT_TRIANGLELIST, 0, 4, 2, indices,
        D3DFMT_INDEX16, vertices, sizeof(D3DXVECTOR3));
}

}