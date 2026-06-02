#include "stdafx.h"
#include "PostFXPasses.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

BloomPass::BloomPass()
    : IPostFXPass(PostFXType::BLOOM)
    , m_device(NULL)
    , m_threshold(0.8f)
    , m_intensity(1.0f)
    , m_blurPasses(4)
    , m_brightTexture(NULL)
    , m_blurTexture1(NULL)
    , m_blurTexture2(NULL)
    , m_brightSurface(NULL)
    , m_blurSurface1(NULL)
    , m_blurSurface2(NULL)
{
}

BloomPass::~BloomPass() {
    Shutdown();
}

void BloomPass::Initialize(IDirect3DDevice9* device) {
    m_device = device;

    D3DFORMAT format = D3DFMT_A8R8G8B8;
    int width = 512;
    int height = 512;

    m_device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET,
                           format, D3DPOOL_DEFAULT, &m_brightTexture, NULL);
    if (m_brightTexture) {
        m_brightTexture->GetSurfaceLevel(0, &m_brightSurface);
    }

    m_device->CreateTexture(width / 4, height / 4, 1, D3DUSAGE_RENDERTARGET,
                           format, D3DPOOL_DEFAULT, &m_blurTexture1, NULL);
    if (m_blurTexture1) {
        m_blurTexture1->GetSurfaceLevel(0, &m_blurSurface1);
    }

    m_device->CreateTexture(width / 4, height / 4, 1, D3DUSAGE_RENDERTARGET,
                           format, D3DPOOL_DEFAULT, &m_blurTexture2, NULL);
    if (m_blurTexture2) {
        m_blurTexture2->GetSurfaceLevel(0, &m_blurSurface2);
    }

    OutputDebugStringA("[BloomPass] Initialized\n");
}

void BloomPass::Shutdown() {
    if (m_brightSurface) { m_brightSurface->Release(); m_brightSurface = NULL; }
    if (m_brightTexture) { m_brightTexture->Release(); m_brightTexture = NULL; }
    if (m_blurSurface1) { m_blurSurface1->Release(); m_blurSurface1 = NULL; }
    if (m_blurTexture1) { m_blurTexture1->Release(); m_blurTexture1 = NULL; }
    if (m_blurSurface2) { m_blurSurface2->Release(); m_blurSurface2 = NULL; }
    if (m_blurTexture2) { m_blurTexture2->Release(); m_blurTexture2 = NULL; }
    m_device = NULL;
}

void BloomPass::SetParameters(const PostFXParams& params) {
    m_params = params;
    m_threshold = params.params[0];
    m_intensity = params.intensity;
    m_blurPasses = (int)params.params[1];
}

void BloomPass::Execute() {
    if (!m_device || !m_enabled) return;

    ExtractBrightAreas();
    ApplyBlur();
    Composite();
}

void BloomPass::ExtractBrightAreas() {
    if (!m_brightSurface) return;

    IDirect3DSurface9* pOldTarget = NULL;
    m_device->GetRenderTarget(0, &pOldTarget);

    m_device->SetRenderTarget(0, m_brightSurface);
    m_device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    if (pOldTarget) pOldTarget->Release();
}

void BloomPass::ApplyBlur() {
    if (!m_device) return;
}

void BloomPass::Composite() {
    if (!m_device) return;
}

FogPass::FogPass()
    : IPostFXPass(PostFXType::FOG)
    , m_device(NULL)
    , m_fogDensity(0.01f)
    , m_fogStart(10.0f)
    , m_fogEnd(100.0f)
{
    m_fogColor = D3DXVECTOR3(0.5f, 0.6f, 0.7f);
}

FogPass::~FogPass() {
    Shutdown();
}

void FogPass::Initialize(IDirect3DDevice9* device) {
    m_device = device;
    OutputDebugStringA("[FogPass] Initialized\n");
}

void FogPass::Shutdown() {
    m_device = NULL;
}

void FogPass::SetParameters(const PostFXParams& params) {
    m_params = params;
    m_fogDensity = params.params[0];
    m_fogStart = params.params[1];
    m_fogEnd = params.params[2];
}

void FogPass::Execute() {
    if (!m_device || !m_enabled) return;
}

TonemapPass::TonemapPass()
    : IPostFXPass(PostFXType::TONEMAP)
    , m_device(NULL)
    , m_operator(TonemapOperator::REINHARD)
    , m_exposure(1.0f)
    , m_contrast(1.0f)
    , m_brightness(0.0f)
{
}

TonemapPass::~TonemapPass() {
    Shutdown();
}

void TonemapPass::Initialize(IDirect3DDevice9* device) {
    m_device = device;
    OutputDebugStringA("[TonemapPass] Initialized\n");
}

void TonemapPass::Shutdown() {
    m_device = NULL;
}

void TonemapPass::SetParameters(const PostFXParams& params) {
    m_params = params;
    m_exposure = params.params[0];
    m_contrast = params.params[1];
}

void TonemapPass::Execute() {
    if (!m_device || !m_enabled) return;
}

VignettePass::VignettePass()
    : IPostFXPass(PostFXType::VIGNETTE)
    , m_device(NULL)
    , m_intensity(0.5f)
    , m_radius(0.75f)
    , m_smoothness(0.2f)
{
}

VignettePass::~VignettePass() {
    Shutdown();
}

void VignettePass::Initialize(IDirect3DDevice9* device) {
    m_device = device;
    OutputDebugStringA("[VignettePass] Initialized\n");
}

void VignettePass::Shutdown() {
    m_device = NULL;
}

void VignettePass::SetParameters(const PostFXParams& params) {
    m_params = params;
    m_intensity = params.params[0];
    m_radius = params.params[1];
}

void VignettePass::Execute() {
    if (!m_device || !m_enabled) return;
}

}
