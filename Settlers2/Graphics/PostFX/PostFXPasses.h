#pragma once
#include "PostFXChain.h"

namespace Graphics {

class BloomPass : public IPostFXPass
{
public:
    BloomPass();
    virtual ~BloomPass();

    virtual const char* GetName() const override { return "BloomPass"; }
    virtual PostFXType GetType() const override { return PostFXType::BLOOM; }
    virtual bool IsEnabled() const override { return m_enabled; }
    virtual void SetEnabled(bool enabled) override { m_enabled = enabled; }

    virtual void Initialize(IDirect3DDevice9* device) override;
    virtual void Shutdown() override;

    virtual void SetParameters(const PostFXParams& params) override;
    virtual const PostFXParams& GetParameters() const override { return m_params; }

    virtual void Execute() override;

private:
    void ExtractBrightAreas();
    void ApplyBlur();
    void Composite();

    IDirect3DDevice9* m_device;
    PostFXParams m_params;

    IDirect3DTexture9* m_brightTexture;
    IDirect3DTexture9* m_blurTexture1;
    IDirect3DTexture9* m_blurTexture2;
    IDirect3DSurface9* m_brightSurface;
    IDirect3DSurface9* m_blurSurface1;
    IDirect3DSurface9* m_blurSurface2;

    float m_threshold;
    float m_intensity;
    int m_blurPasses;
};

class FogPass : public IPostFXPass
{
public:
    FogPass();
    virtual ~FogPass();

    virtual const char* GetName() const override { return "FogPass"; }
    virtual PostFXType GetType() const override { return PostFXType::FOG; }
    virtual bool IsEnabled() const override { return m_enabled; }
    virtual void SetEnabled(bool enabled) override { m_enabled = enabled; }

    virtual void Initialize(IDirect3DDevice9* device) override;
    virtual void Shutdown() override;

    virtual void SetParameters(const PostFXParams& params) override;
    virtual const PostFXParams& GetParameters() const override { return m_params; }

    virtual void Execute() override;

private:
    IDirect3DDevice9* m_device;
    PostFXParams m_params;

    float m_fogDensity;
    float m_fogStart;
    float m_fogEnd;
    D3DXVECTOR3 m_fogColor;
};

class TonemapPass : public IPostFXPass
{
public:
    TonemapPass();
    virtual ~TonemapPass();

    virtual const char* GetName() const override { return "TonemapPass"; }
    virtual PostFXType GetType() const override { return PostFXType::TONEMAP; }
    virtual bool IsEnabled() const override { return m_enabled; }
    virtual void SetEnabled(bool enabled) override { m_enabled = enabled; }

    virtual void Initialize(IDirect3DDevice9* device) override;
    virtual void Shutdown() override;

    virtual void SetParameters(const PostFXParams& params) override;
    virtual const PostFXParams& GetParameters() const override { return m_params; }

    virtual void Execute() override;

private:
    enum class TonemapOperator {
        REINHARD,
        REINHARD2,
        TONEMAP_ACES,
        UNCHARTED2
    };

    IDirect3DDevice9* m_device;
    PostFXParams m_params;

    TonemapOperator m_operator;
    float m_exposure;
    float m_contrast;
    float m_brightness;
};

class VignettePass : public IPostFXPass
{
public:
    VignettePass();
    virtual ~VignettePass();

    virtual const char* GetName() const override { return "VignettePass"; }
    virtual PostFXType GetType() const override { return PostFXType::VIGNETTE; }
    virtual bool IsEnabled() const override { return m_enabled; }
    virtual void SetEnabled(bool enabled) override { m_enabled = enabled; }

    virtual void Initialize(IDirect3DDevice9* device) override;
    virtual void Shutdown() override;

    virtual void SetParameters(const PostFXParams& params) override;
    virtual const PostFXParams& GetParameters() const override { return m_params; }

    virtual void Execute() override;

private:
    IDirect3DDevice9* m_device;
    PostFXParams m_params;

    float m_intensity;
    float m_radius;
    float m_smoothness;
};

}
