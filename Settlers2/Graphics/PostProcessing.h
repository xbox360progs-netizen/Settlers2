#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>

namespace Graphics {

struct BloomSettings {
    float threshold;
    float intensity;
    float radius;
    int blurPasses;

    BloomSettings() : threshold(0.8f), intensity(0.5f), radius(4.0f), blurPasses(4) {}
};

struct ToneMappingSettings {
    float exposure;
    float contrast;
    float gamma;
    float whitePoint;

    ToneMappingSettings() : exposure(1.0f), contrast(1.0f), gamma(2.2f), whitePoint(1.0f) {}
};

struct ColorGradingSettings {
    float temperature;
    float tint;
    float saturation;
    float brightness;
    float lift;
    float gamma;
    float gain;

    ColorGradingSettings()
        : temperature(0.0f), tint(0.0f), saturation(1.0f), brightness(1.0f),
          lift(0.0f), gamma(1.0f), gain(1.0f) {}
};

struct FogSettings {
    bool enabled;
    D3DXVECTOR3 fogColor;
    float fogStart;
    float fogEnd;
    float fogDensity;
    bool useDistanceFog;
    bool useHeightFog;
    float fogHeight;

    FogSettings()
        : enabled(false), fogColor(0.5f, 0.5f, 0.5f),
          fogStart(0.0f), fogEnd(100.0f), fogDensity(0.01f),
          useDistanceFog(true), useHeightFog(false), fogHeight(10.0f) {}
};

class PostProcessingSystem {
public:
    PostProcessingSystem();
    ~PostProcessingSystem();

    void Initialize(LPDIRECT3DDEVICE9 pDevice, int screenWidth, int screenHeight);
    void Shutdown();

    void SetEnabled(bool enable) { m_enabled = enable; }
    bool IsEnabled() const { return m_enabled; }

    // Bloom
    void SetBloomEnabled(bool enable);
    bool IsBloomEnabled() const { return m_bloomEnabled; }
    void SetBloomSettings(const BloomSettings& settings) { m_bloomSettings = settings; }
    BloomSettings GetBloomSettings() const { return m_bloomSettings; }

    // Tone Mapping
    void SetToneMappingEnabled(bool enable);
    bool IsToneMappingEnabled() const { return m_toneMappingEnabled; }
    void SetToneMappingSettings(const ToneMappingSettings& settings) { m_toneMappingSettings = settings; }
    ToneMappingSettings GetToneMappingSettings() const { return m_toneMappingSettings; }

    // Color Grading
    void SetColorGradingEnabled(bool enable);
    bool IsColorGradingEnabled() const { return m_colorGradingEnabled; }
    void SetColorGradingSettings(const ColorGradingSettings& settings) { m_colorGradingSettings = settings; }
    ColorGradingSettings GetColorGradingSettings() const { return m_colorGradingSettings; }

    // Fog
    void SetFogSettings(const FogSettings& settings) { m_fogSettings = settings; }
    FogSettings GetFogSettings() const { return m_fogSettings; }
    void SetFogEnabled(bool enable) { m_fogSettings.enabled = enable; }
    bool IsFogEnabled() const { return m_fogSettings.enabled; }

    void Render(IDirect3DTexture9* inputTexture, IDirect3DSurface9* outputSurface);

    void SetDebugView(bool debug) { m_debugView = debug; }
    bool IsDebugViewEnabled() const { return m_debugView; }

private:
    LPDIRECT3DDEVICE9 m_pDevice;
    bool m_enabled;
    bool m_debugView;

    int m_screenWidth;
    int m_screenHeight;

    bool m_bloomEnabled;
    BloomSettings m_bloomSettings;

    bool m_toneMappingEnabled;
    ToneMappingSettings m_toneMappingSettings;

    bool m_colorGradingEnabled;
    ColorGradingSettings m_colorGradingSettings;

    FogSettings m_fogSettings;

    LPDIRECT3DTEXTURE9 m_pBloomExtractTexture;
    LPDIRECT3DSURFACE9 m_pBloomExtractSurface;

    LPDIRECT3DTEXTURE9 m_pBloomBlurTexture1;
    LPDIRECT3DSURFACE9 m_pBloomBlurSurface1;

    LPDIRECT3DTEXTURE9 m_pBloomBlurTexture2;
    LPDIRECT3DSURFACE9 m_pBloomBlurSurface2;

    LPDIRECT3DTEXTURE9 m_pTempTexture;
    LPDIRECT3DSURFACE9 m_pTempSurface;

    LPDIRECT3DVERTEXBUFFER9 m_pQuadVB;
    LPDIRECT3DINDEXBUFFER9 m_pQuadIB;
    LPDIRECT3DVERTEXDECLARATION9 m_pQuadDecl;

    void CreateRenderTargets();
    void DestroyRenderTargets();
    void CreateQuadMesh();

    void ApplyBloom(IDirect3DTexture9* input, IDirect3DTexture9* output);
    void ExtractBrightPixels(IDirect3DTexture9* input, IDirect3DTexture9* output);
    void GaussianBlur(IDirect3DTexture9* input, IDirect3DTexture9* output, float radius);
    void AddBloom(IDirect3DTexture9* base, IDirect3DTexture9* bloom);

    void ApplyToneMapping(IDirect3DTexture9* input, IDirect3DTexture9* output);
    void ApplyColorGrading(IDirect3DTexture9* input, IDirect3DTexture9* output);
    void ApplyFog(IDirect3DTexture9* input, IDirect3DTexture9* output);
};

}
