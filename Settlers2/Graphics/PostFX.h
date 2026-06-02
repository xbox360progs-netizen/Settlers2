#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>

namespace Graphics {

class PostFXManager {
public:
    PostFXManager();
    ~PostFXManager();

    void Initialize(IDirect3DDevice9* pDevice, int width, int height);
    void Shutdown();

    void BeginFrame();
    void EndFrame();
    void Execute();

    void OnResize(int width, int height);

private:
    IDirect3DDevice9* m_pDevice;
    int m_width;
    int m_height;
    bool m_initialized;

    IDirect3DTexture9* m_pSceneTexture;
    IDirect3DTexture9* m_pBrightPassTexture;
    IDirect3DTexture9* m_pBloomTexture1;
    IDirect3DTexture9* m_pBloomTexture2;
    IDirect3DSurface9* m_pSceneSurface;
    IDirect3DSurface9* m_pBrightPassSurface;
    IDirect3DSurface9* m_pBloomSurface1;
    IDirect3DSurface9* m_pBloomSurface2;

    void CreateRenderTargets();
    void ReleaseRenderTargets();
};

class BloomEffect {
public:
    BloomEffect();
    ~BloomEffect();

    void Initialize(IDirect3DDevice9* pDevice, int width, int height);
    void Shutdown();

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void SetThreshold(float threshold) { m_threshold = threshold; }
    float GetThreshold() const { return m_threshold; }

    void SetIntensity(float intensity) { m_intensity = intensity; }
    float GetIntensity() const { return m_intensity; }

    void SetBlurPasses(int passes) { m_blurPasses = passes; }
    int GetBlurPasses() const { return m_blurPasses; }

    void Render(IDirect3DDevice9* pDevice, IDirect3DTexture9* input, IDirect3DTexture9* output);

private:
    IDirect3DDevice9* m_pDevice;
    int m_width;
    int m_height;
    bool m_initialized;
    bool m_enabled;
    float m_threshold;
    float m_intensity;
    int m_blurPasses;

    IDirect3DTexture9* m_pBrightPass;
    IDirect3DTexture9* m_pBlurH;
    IDirect3DTexture9* m_pBlurV;

    void CreateTextures();
    void ReleaseTextures();
    void RenderBrightPass(IDirect3DTexture9* input);
    void RenderBlur(IDirect3DTexture9* input);
    void RenderComposite(IDirect3DTexture9* bloom, IDirect3DTexture9* original, IDirect3DTexture9* output);
};

class SSAOEffect {
public:
    SSAOEffect();
    ~SSAOEffect();

    void Initialize(IDirect3DDevice9* pDevice, int width, int height);
    void Shutdown();

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void SetRadius(float radius) { m_radius = radius; }
    float GetRadius() const { return m_radius; }

    void SetIntensity(float intensity) { m_intensity = intensity; }
    float GetIntensity() const { return m_intensity; }

    void SetSampleCount(int count) { m_sampleCount = count; }
    int GetSampleCount() const { return m_sampleCount; }

    void Render(IDirect3DDevice9* pDevice, 
                IDirect3DTexture9* depthTexture, 
                IDirect3DTexture9* normalTexture,
                IDirect3DTexture9* output);

private:
    IDirect3DDevice9* m_pDevice;
    int m_width;
    int m_height;
    bool m_initialized;
    bool m_enabled;
    float m_radius;
    float m_intensity;
    int m_sampleCount;

    IDirect3DTexture9* m_pSSAOTexture;

    void CreateTextures();
    void ReleaseTextures();
};

class FogEffect {
public:
    FogEffect();
    ~FogEffect();

    void Initialize(IDirect3DDevice9* pDevice);
    void Shutdown();

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void SetColor(D3DXVECTOR3 color) { m_color = color; }
    D3DXVECTOR3 GetColor() const { return m_color; }

    void SetStartDistance(float distance) { m_startDistance = distance; }
    float GetStartDistance() const { return m_startDistance; }

    void SetEndDistance(float distance) { m_endDistance = distance; }
    float GetEndDistance() const { return m_endDistance; }

    void SetDensity(float density) { m_density = density; }
    float GetDensity() const { return m_density; }

    enum FogMode {
        FOG_LINEAR,
        FOG_EXP,
        FOG_EXP2
    };

    void SetMode(FogMode mode) { m_mode = mode; }
    FogMode GetMode() const { return m_mode; }

    void Render(IDirect3DDevice9* pDevice, IDirect3DTexture9* sceneTexture, IDirect3DTexture9* depthTexture, IDirect3DTexture9* output);

private:
    IDirect3DDevice9* m_pDevice;
    bool m_initialized;
    bool m_enabled;
    D3DXVECTOR3 m_color;
    float m_startDistance;
    float m_endDistance;
    float m_density;
    FogMode m_mode;
};

class ToneMapEffect {
public:
    ToneMapEffect();
    ~ToneMapEffect();

    void Initialize(IDirect3DDevice9* pDevice);
    void Shutdown();

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    enum ToneMapOperator {
        TM_REINHARD,
        TM_ACES,
        TM_FILMIC,
        TM_EXPOSURE
    };

    void SetOperator(ToneMapOperator op) { m_operator = op; }
    ToneMapOperator GetOperator() const { return m_operator; }

    void SetExposure(float exposure) { m_exposure = exposure; }
    float GetExposure() const { return m_exposure; }

    void Render(IDirect3DDevice9* pDevice, IDirect3DTexture9* input, IDirect3DTexture9* output);

private:
    IDirect3DDevice9* m_pDevice;
    bool m_initialized;
    bool m_enabled;
    ToneMapOperator m_operator;
    float m_exposure;
};

class ColorGradeEffect {
public:
    ColorGradeEffect();
    ~ColorGradeEffect();

    void Initialize(IDirect3DDevice9* pDevice);
    void Shutdown();

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    enum ColorGradePreset {
        PRESET_DEFAULT,
        PRESET_WARM,
        PRESET_COOL,
        PRESET_NIGHT,
        PRESET_DESERT,
        PRESET_FOREST,
        PRESET_COUNT
    };

    void SetPreset(ColorGradePreset preset) { m_currentPreset = preset; }
    ColorGradePreset GetPreset() const { return m_currentPreset; }

    void SetLift(float lift) { m_lift = lift; }
    void SetGamma(float gamma) { m_gamma = gamma; }
    void SetGain(float gain) { m_gain = gain; }

    void Render(IDirect3DDevice9* pDevice, IDirect3DTexture9* input, IDirect3DTexture9* output);

private:
    IDirect3DDevice9* m_pDevice;
    bool m_initialized;
    bool m_enabled;
    ColorGradePreset m_currentPreset;
    float m_lift;
    float m_gamma;
    float m_gain;

    D3DXVECTOR3 GetLiftColor();
    D3DXVECTOR3 GetGammaColor();
    D3DXVECTOR3 GetGainColor();
};

}
