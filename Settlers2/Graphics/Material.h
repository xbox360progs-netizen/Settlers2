#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <string>

namespace Graphics {

struct MaterialProps {
    float specularStrength;
    float roughness;
    float ambientOcclusion;
    float emissive;

    MaterialProps() 
        : specularStrength(0.3f), roughness(0.5f), 
          ambientOcclusion(1.0f), emissive(0.0f) {}

    MaterialProps(float spec, float rough, float ao = 1.0f, float emv = 0.0f)
        : specularStrength(spec), roughness(rough),
          ambientOcclusion(ao), emissive(emv) {}
};

class Material {
public:
    Material();
    Material(const char* name);
    ~Material();

    void SetDiffuseTexture(LPDIRECT3DTEXTURE9 tex) { m_pDiffuseTex = tex; }
    void SetNormalTexture(LPDIRECT3DTEXTURE9 tex) { m_pNormalTex = tex; }
    void SetMaterialTexture(LPDIRECT3DTEXTURE9 tex) { m_pMaterialTex = tex; }

    LPDIRECT3DTEXTURE9 GetDiffuseTexture() const { return m_pDiffuseTex; }
    LPDIRECT3DTEXTURE9 GetNormalTexture() const { return m_pNormalTex; }
    LPDIRECT3DTEXTURE9 GetMaterialTexture() const { return m_pMaterialTex; }

    void SetName(const char* name) { m_name = name ? name : ""; }
    const char* GetName() const { return m_name.c_str(); }

    void SetProperties(const MaterialProps& props) { m_props = props; }
    MaterialProps GetProperties() const { return m_props; }

    bool HasNormalMap() const { return m_pNormalTex != NULL; }
    bool HasMaterialMap() const { return m_pMaterialTex != NULL; }

    void SetShaderFlags(DWORD flags) { m_shaderFlags = flags; }
    DWORD GetShaderFlags() const { return m_shaderFlags; }

    void SetTileID(int id) { m_tileID = id; }
    int GetTileID() const { return m_tileID; }

    bool IsValid() const;

private:
    std::string m_name;
    LPDIRECT3DTEXTURE9 m_pDiffuseTex;
    LPDIRECT3DTEXTURE9 m_pNormalTex;
    LPDIRECT3DTEXTURE9 m_pMaterialTex;
    MaterialProps m_props;
    DWORD m_shaderFlags;
    int m_tileID;
};

class MaterialManager {
public:
    MaterialManager();
    ~MaterialManager();

    Material* CreateMaterial(const char* name);
    Material* GetMaterial(const char* name);
    Material* GetMaterial(int index);

    void RemoveMaterial(const char* name);
    void RemoveAll();

    int GetMaterialCount() const { return (int)m_materials.size(); }

    Material* GetDefaultMaterial();

private:
    std::vector<Material*> m_materials;
    Material* m_pDefaultMaterial;
};

}