#include "stdafx.h"
#include "PostFX.h"
#include <cmath>

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

PostFXManager::PostFXManager()
    : m_pDevice(NULL)
    , m_width(0)
    , m_height(0)
    , m_initialized(false)
    , m_pSceneTexture(NULL)
    , m_pBrightPassTexture(NULL)
    , m_pBloomTexture1(NULL)
    , m_pBloomTexture2(NULL)
    , m_pSceneSurface(NULL)
    , m_pBrightPassSurface(NULL)
    , m_pBloomSurface1(NULL)
    , m_pBloomSurface2(NULL)
{
}

PostFXManager::~PostFXManager() {
    Shutdown();
}

void PostFXManager::Initialize(IDirect3DDevice9* pDevice, int width, int height) {
    m_pDevice = pDevice;
    m_width = width;
    m_height = height;
    CreateRenderTargets();
    m_initialized = true;
    OutputDebugStringA("[PostFXManager] Initialized\n");
}

void PostFXManager::Shutdown() {
    ReleaseRenderTargets();
    m_initialized = false;
}

void PostFXManager::CreateRenderTargets() {
    if (!m_pDevice) return;

    m_pDevice->CreateTexture(m_width, m_height, 1, D3DUSAGE_RENDERTARGET, 
        D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pSceneTexture, NULL);

    int halfWidth = m_width / 2;
    int halfHeight = m_height / 2;

    m_pDevice->CreateTexture(halfWidth, halfHeight, 1, D3DUSAGE_RENDERTARGET,
        D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pBrightPassTexture, NULL);
    m_pDevice->CreateTexture(halfWidth, halfHeight, 1, D3DUSAGE_RENDERTARGET,
        D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pBloomTexture1, NULL);
    m_pDevice->CreateTexture(halfWidth, halfHeight, 1, D3DUSAGE_RENDERTARGET,
        D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pBloomTexture2, NULL);
}

void PostFXManager::ReleaseRenderTargets() {
    if (m_pSceneTexture) { m_pSceneTexture->Release(); m_pSceneTexture = NULL; }
    if (m_pBrightPassTexture) { m_pBrightPassTexture->Release(); m_pBrightPassTexture = NULL; }
    if (m_pBloomTexture1) { m_pBloomTexture1->Release(); m_pBloomTexture1 = NULL; }
    if (m_pBloomTexture2) { m_pBloomTexture2->Release(); m_pBloomTexture2 = NULL; }
    if (m_pSceneSurface) { m_pSceneSurface->Release(); m_pSceneSurface = NULL; }
    if (m_pBrightPassSurface) { m_pBrightPassSurface->Release(); m_pBrightPassSurface = NULL; }
    if (m_pBloomSurface1) { m_pBloomSurface1->Release(); m_pBloomSurface1 = NULL; }
    if (m_pBloomSurface2) { m_pBloomSurface2->Release(); m_pBloomSurface2 = NULL; }
}

void PostFXManager::BeginFrame() {
}

void PostFXManager::EndFrame() {
}

void PostFXManager::Execute() {
    OutputDebugStringA("[PostFXManager] Execute - placeholder for post-processing chain\n");
}

void PostFXManager::OnResize(int width, int height) {
    m_width = width;
    m_height = height;
    ReleaseRenderTargets();
    CreateRenderTargets();
}

BloomEffect::BloomEffect()
    : m_pDevice(NULL)
    , m_width(0)
    , m_height(0)
    , m_initialized(false)
    , m_enabled(true)
    , m_threshold(0.8f)
    , m_intensity(0.5f)
    , m_blurPasses(4)
    , m_pBrightPass(NULL)
    , m_pBlurH(NULL)
    , m_pBlurV(NULL)
{
}

BloomEffect::~BloomEffect() {
    Shutdown();
}

void BloomEffect::Initialize(IDirect3DDevice9* pDevice, int width, int height) {
    m_pDevice = pDevice;
    m_width = width;
    m_height = height;
    CreateTextures();
    m_initialized = true;
    OutputDebugStringA("[BloomEffect] Initialized\n");
}

void BloomEffect::Shutdown() {
    ReleaseTextures();
    m_initialized = false;
}

void BloomEffect::CreateTextures() {
    if (!m_pDevice) return;

    int halfW = m_width / 2;
    int halfH = m_height / 2;

    m_pDevice->CreateTexture(halfW, halfH, 1, D3DUSAGE_RENDERTARGET,
        D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pBrightPass, NULL);
    m_pDevice->CreateTexture(halfW, halfH, 1, D3DUSAGE_RENDERTARGET,
        D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pBlurH, NULL);
    m_pDevice->CreateTexture(halfW, halfH, 1, D3DUSAGE_RENDERTARGET,
        D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pBlurV, NULL);
}

void BloomEffect::ReleaseTextures() {
    if (m_pBrightPass) { m_pBrightPass->Release(); m_pBrightPass = NULL; }
    if (m_pBlurH) { m_pBlurH->Release(); m_pBlurH = NULL; }
    if (m_pBlurV) { m_pBlurV->Release(); m_pBlurV = NULL; }
}

void BloomEffect::RenderBrightPass(IDirect3DTexture9* input) {
}

void BloomEffect::RenderBlur(IDirect3DTexture9* input) {
}

void BloomEffect::RenderComposite(IDirect3DTexture9* bloom, IDirect3DTexture9* original, IDirect3DTexture9* output) {
}

void BloomEffect::Render(IDirect3DDevice9* pDevice, IDirect3DTexture9* input, IDirect3DTexture9* output) {
    if (!m_enabled || !m_initialized) return;

    OutputDebugStringA("[BloomEffect] Render\n");

    RenderBrightPass(input);
    RenderBlur(m_pBrightPass);
    RenderComposite(m_pBlurV, input, output);
}

SSAOEffect::SSAOEffect()
    : m_pDevice(NULL)
    , m_width(0)
    , m_height(0)
    , m_initialized(false)
    , m_enabled(true)
    , m_radius(0.5f)
    , m_intensity(1.0f)
    , m_sampleCount(16)
    , m_pSSAOTexture(NULL)
{
}

SSAOEffect::~SSAOEffect() {
    Shutdown();
}

void SSAOEffect::Initialize(IDirect3DDevice9* pDevice, int width, int height) {
    m_pDevice = pDevice;
    m_width = width;
    m_height = height;
    CreateTextures();
    m_initialized = true;
    OutputDebugStringA("[SSAOEffect] Initialized\n");
}

void SSAOEffect::Shutdown() {
    ReleaseTextures();
    m_initialized = false;
}

void SSAOEffect::CreateTextures() {
    if (!m_pDevice) return;

    m_pDevice->CreateTexture(m_width, m_height, 1, D3DUSAGE_RENDERTARGET,
        D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pSSAOTexture, NULL);
}

void SSAOEffect::ReleaseTextures() {
    if (m_pSSAOTexture) { m_pSSAOTexture->Release(); m_pSSAOTexture = NULL; }
}

void SSAOEffect::Render(IDirect3DDevice9* pDevice, 
                        IDirect3DTexture9* depthTexture, 
                        IDirect3DTexture9* normalTexture,
                        IDirect3DTexture9* output) {
    if (!m_enabled || !m_initialized) return;

    OutputDebugStringA("[SSAOEffect] Render\n");
}

FogEffect::FogEffect()
    : m_pDevice(NULL)
    , m_initialized(false)
    , m_enabled(true)
    , m_color(0.7f, 0.7f, 0.8f)
    , m_startDistance(10.0f)
    , m_endDistance(100.0f)
    , m_density(0.02f)
    , m_mode(FOG_LINEAR)
{
}

FogEffect::~FogEffect() {
    Shutdown();
}

void FogEffect::Initialize(IDirect3DDevice9* pDevice) {
    m_pDevice = pDevice;
    m_initialized = true;
    OutputDebugStringA("[FogEffect] Initialized\n");
}

void FogEffect::Shutdown() {
    m_initialized = false;
}

void FogEffect::Render(IDirect3DDevice9* pDevice, IDirect3DTexture9* sceneTexture, IDirect3DTexture9* depthTexture, IDirect3DTexture9* output) {
    if (!m_enabled || !m_initialized) return;

    OutputDebugStringA("[FogEffect] Render\n");
}

ToneMapEffect::ToneMapEffect()
    : m_pDevice(NULL)
    , m_initialized(false)
    , m_enabled(true)
    , m_operator(TM_ACES)
    , m_exposure(1.0f)
{
}

ToneMapEffect::~ToneMapEffect() {
    Shutdown();
}

void ToneMapEffect::Initialize(IDirect3DDevice9* pDevice) {
    m_pDevice = pDevice;
    m_initialized = true;
    OutputDebugStringA("[ToneMapEffect] Initialized\n");
}

void ToneMapEffect::Shutdown() {
    m_initialized = false;
}

void ToneMapEffect::Render(IDirect3DDevice9* pDevice, IDirect3DTexture9* input, IDirect3DTexture9* output) {
    if (!m_enabled || !m_initialized) return;

    OutputDebugStringA("[ToneMapEffect] Render\n");
}

ColorGradeEffect::ColorGradeEffect()
    : m_pDevice(NULL)
    , m_initialized(false)
    , m_enabled(true)
    , m_currentPreset(PRESET_DEFAULT)
    , m_lift(0.0f)
    , m_gamma(1.0f)
    , m_gain(1.0f)
{
}

ColorGradeEffect::~ColorGradeEffect() {
    Shutdown();
}

void ColorGradeEffect::Initialize(IDirect3DDevice9* pDevice) {
    m_pDevice = pDevice;
    m_initialized = true;
    OutputDebugStringA("[ColorGradeEffect] Initialized\n");
}

void ColorGradeEffect::Shutdown() {
    m_initialized = false;
}

D3DXVECTOR3 ColorGradeEffect::GetLiftColor() {
    switch (m_currentPreset) {
        case PRESET_WARM: return D3DXVECTOR3(0.0f, 0.05f, -0.05f);
        case PRESET_COOL: return D3DXVECTOR3(-0.05f, 0.0f, 0.1f);
        case PRESET_NIGHT: return D3DXVECTOR3(0.0f, -0.05f, 0.05f);
        case PRESET_DESERT: return D3DXVECTOR3(0.1f, 0.05f, 0.0f);
        case PRESET_FOREST: return D3DXVECTOR3(-0.05f, 0.1f, 0.0f);
        default: return D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    }
}

D3DXVECTOR3 ColorGradeEffect::GetGammaColor() {
    switch (m_currentPreset) {
        case PRESET_WARM: return D3DXVECTOR3(1.1f, 1.0f, 0.9f);
        case PRESET_COOL: return D3DXVECTOR3(0.9f, 1.0f, 1.1f);
        case PRESET_NIGHT: return D3DXVECTOR3(0.9f, 0.9f, 1.0f);
        default: return D3DXVECTOR3(1.0f, 1.0f, 1.0f);
    }
}

D3DXVECTOR3 ColorGradeEffect::GetGainColor() {
    switch (m_currentPreset) {
        case PRESET_WARM: return D3DXVECTOR3(1.2f, 1.1f, 0.9f);
        case PRESET_COOL: return D3DXVECTOR3(0.9f, 1.0f, 1.2f);
        case PRESET_NIGHT: return D3DXVECTOR3(0.8f, 0.8f, 1.1f);
        default: return D3DXVECTOR3(1.0f, 1.0f, 1.0f);
    }
}

void ColorGradeEffect::Render(IDirect3DDevice9* pDevice, IDirect3DTexture9* input, IDirect3DTexture9* output) {
    if (!m_enabled || !m_initialized) return;

    OutputDebugStringA("[ColorGradeEffect] Render\n");
}

}