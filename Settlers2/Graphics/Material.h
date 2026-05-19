#pragma once
#include <d3d9.h>
#include <string>
#include <vector>
#include <cstdint>
#include "ShaderManager.h"

namespace Graphics {

enum MaterialFlags {
    MATERIAL_FLAG_NONE = 0,
    MATERIAL_FLAG_ALPHATEST = 1 << 0,
    MATERIAL_FLAG_TRANSPARENT = 1 << 1,
    MATERIAL_FLAG_EMISSIVE = 1 << 2,
    MATERIAL_FLAG_NORMALMAP = 1 << 3,
    MATERIAL_FLAG_SPECULAR = 1 << 4,
    MATERIAL_FLAG_AMBIENT_OCCLUSION = 1 << 5,
    MATERIAL_FLAG_CAST_SHADOWS = 1 << 6,
    MATERIAL_FLAG_RECEIVE_SHADOWS = 1 << 7
};

class Material {
public:
    IDirect3DTexture9* pDiffuseMap;
    IDirect3DTexture9* pNormalMap;
    IDirect3DTexture9* pMaterialMap;

    uint32_t Flags;

    float Roughness;
    float Metallic;
    float EmissiveIntensity;
    float AmbientOcclusion;

    std::string m_name;
    uint32_t m_shaderFlags;
    int m_tileID;

    Material() 
        : pDiffuseMap(NULL), pNormalMap(NULL), pMaterialMap(NULL),
          Flags(MATERIAL_FLAG_NONE), Roughness(0.5f), Metallic(0.0f),
          EmissiveIntensity(0.0f), AmbientOcclusion(1.0f),
          m_name(""), m_shaderFlags(0), m_tileID(-1) {}

    Material(const char* name) 
        : pDiffuseMap(NULL), pNormalMap(NULL), pMaterialMap(NULL),
          Flags(MATERIAL_FLAG_NONE), Roughness(0.5f), Metallic(0.0f),
          EmissiveIntensity(0.0f), AmbientOcclusion(1.0f),
          m_name(name ? name : ""), m_shaderFlags(0), m_tileID(-1) {}

    ~Material() {}

    bool IsValid() const {
        return pDiffuseMap != NULL;
    }
};

inline uint32_t PackMaterialMap(float roughness, float ao, float emissive, float metallic) {
    uint8_t r = (uint8_t)(metallic * 255.0f);
    uint8_t g = (uint8_t)(roughness * 255.0f);
    uint8_t b = (uint8_t)(ao * 255.0f);
    uint8_t a = (uint8_t)(emissive * 255.0f);
    return (a << 24) | (b << 16) | (g << 8) | r;
}

inline void UnpackMaterialMap(uint32_t packed, float& roughness, float& ao, float& emissive, float& metallic) {
    metallic = ((packed >> 0) & 0xFF) / 255.0f;
    roughness = ((packed >> 8) & 0xFF) / 255.0f;
    ao = ((packed >> 16) & 0xFF) / 255.0f;
    emissive = ((packed >> 24) & 0xFF) / 255.0f;
}

class MaterialManager {
public:
    MaterialManager();
    ~MaterialManager();

    void Initialize(IDirect3DDevice9* pDevice);
    void Shutdown();

    int CreateMaterial(const Material& material);
    Material* GetMaterial(int id);
    void SetMaterial(int id, const Material& material);
    void RemoveMaterial(int id);

    void BindMaterial(int id);

    enum ShaderVariant {
        SHADER_VARIANT_OPAQUE,
        SHADER_VARIANT_ALPHATEST,
        SHADER_VARIANT_TRANSPARENT,
        SHADER_VARIANT_NORMALMAP,
        SHADER_VARIANT_EMISSIVE,
        SHADER_VARIANT_PBR
    };

    ShaderVariant ResolveShaderVariant(int materialID) const;
    ShaderID ResolveShader(int materialID) const;

private:
    IDirect3DDevice9* m_pDevice;
    std::vector<Material*> m_materials;
    int m_nextID;
};

}