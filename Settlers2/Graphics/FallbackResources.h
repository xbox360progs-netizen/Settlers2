#pragma once
#include <d3d9.h>
#include "Texture.h"

namespace Graphics {

enum class FallbackType {
    WHITE,
    FLAT_NORMAL,
    BLACK_AO,
    ERROR_RED,
    ERROR_PINK
};

class FallbackResources {
public:
    FallbackResources();
    ~FallbackResources();
    
    void Initialize(IDirect3DDevice9* device);
    void Shutdown();
    
    Texture* GetFallbackTexture(FallbackType type);
    IDirect3DTexture9* GetFallbackD3DTexture(FallbackType type);
    
    void SetErrorShader(IDirect3DPixelShader9* shader) { m_errorPixelShader = shader; }
    void SetErrorVertexShader(IDirect3DVertexShader9* shader) { m_errorVertexShader = shader; }
    
    IDirect3DPixelShader9* GetErrorShader() { return m_errorPixelShader; }
    IDirect3DVertexShader9* GetErrorVertexShader() { return m_errorVertexShader; }
    
    bool IsInitialized() const { return m_initialized; }

private:
    IDirect3DDevice9* m_device;
    bool m_initialized;
    
    IDirect3DTexture9* m_whiteTexture;
    IDirect3DTexture9* m_flatNormalTexture;
    IDirect3DTexture9* m_blackAOTexture;
    IDirect3DTexture9* m_errorRedTexture;
    IDirect3DTexture9* m_errorPinkTexture;
    
    IDirect3DPixelShader9* m_errorPixelShader;
    IDirect3DVertexShader9* m_errorVertexShader;
    
    void CreateSolidTexture(IDirect3DTexture9** outTex, D3DCOLOR color);
    void CreateFlatNormalTexture(IDirect3DTexture9** outTex);
};

FallbackResources* GetGlobalFallbackResources();
void SetGlobalFallbackResources(FallbackResources* res);

}