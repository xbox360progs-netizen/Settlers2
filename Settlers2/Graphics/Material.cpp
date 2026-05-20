#include "stdafx.h"
#include "Material.h"

namespace Graphics {

MaterialManager::MaterialManager() : m_pDevice(nullptr), m_nextID(1) {
}

MaterialManager::~MaterialManager() {
    Shutdown();
}

void MaterialManager::Initialize(IDirect3DDevice9* pDevice) {
    m_pDevice = pDevice;
}

void MaterialManager::Shutdown() {
    for (size_t i = 0; i < m_materials.size(); i++) {
        delete m_materials[i];
    }
    m_materials.clear();
    m_nextID = 1;
}

int MaterialManager::CreateMaterial(const Material& material) {
    Material* mat = new Material(material);
    m_materials.push_back(mat);
    return m_nextID++;
}

Material* MaterialManager::GetMaterial(int id) {
    if (id > 0 && id <= (int)m_materials.size()) {
        return m_materials[id - 1];
    }
    return nullptr;
}

void MaterialManager::SetMaterial(int id, const Material& material) {
    if (id > 0 && id <= (int)m_materials.size()) {
        *m_materials[id - 1] = material;
    }
}

void MaterialManager::RemoveMaterial(int id) {
    if (id > 0 && id <= (int)m_materials.size()) {
        delete m_materials[id - 1];
        m_materials.erase(m_materials.begin() + (id - 1));
    }
}

void MaterialManager::BindMaterial(int id) {
    Material* mat = GetMaterial(id);
    if (!mat) return;

    if (m_pDevice && mat->pDiffuseMap) {
        m_pDevice->SetTexture(0, mat->pDiffuseMap);
    }
}

MaterialManager::ShaderVariant MaterialManager::ResolveShaderVariant(int materialID) const {
    Material* mat = const_cast<MaterialManager*>(this)->GetMaterial(materialID);
    if (!mat) return SHADER_VARIANT_OPAQUE;

    if (mat->Flags & MATERIAL_FLAG_TRANSPARENT) {
        return SHADER_VARIANT_TRANSPARENT;
    }
    if (mat->Flags & MATERIAL_FLAG_ALPHATEST) {
        return SHADER_VARIANT_ALPHATEST;
    }
    if (mat->Flags & MATERIAL_FLAG_NORMALMAP) {
        return SHADER_VARIANT_NORMALMAP;
    }
    if (mat->Flags & MATERIAL_FLAG_EMISSIVE) {
        return SHADER_VARIANT_EMISSIVE;
    }
    return SHADER_VARIANT_OPAQUE;
}

ShaderID MaterialManager::ResolveShader(int materialID) const {
    ShaderVariant variant = ResolveShaderVariant(materialID);
    switch (variant) {
        case SHADER_VARIANT_TRANSPARENT: return SHADER_SPRITE;
        case SHADER_VARIANT_ALPHATEST: return SHADER_SPRITE_ALPHATEST;
        case SHADER_VARIANT_NORMALMAP: return SHADER_SPRITE_GBUFFER;
        case SHADER_VARIANT_EMISSIVE: return SHADER_SPRITE_EMISSIVE;
        case SHADER_VARIANT_PBR: return SHADER_SPRITE_PBR;
        default: return SHADER_SPRITE;
    }
}

}