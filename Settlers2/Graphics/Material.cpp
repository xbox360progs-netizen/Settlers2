#include "stdafx.h"
#include "Material.h"

namespace Graphics {

Material::Material() 
    : m_pDiffuseTex(NULL), m_pNormalTex(NULL), m_pMaterialTex(NULL),
      m_shaderFlags(0), m_tileID(-1) {
}

Material::Material(const char* name) 
    : m_name(name ? name : ""),
      m_pDiffuseTex(NULL), m_pNormalTex(NULL), m_pMaterialTex(NULL),
      m_shaderFlags(0), m_tileID(-1) {
}

Material::~Material() {
}

bool Material::IsValid() const {
    return m_pDiffuseTex != NULL;
}

MaterialManager::MaterialManager() : m_pDefaultMaterial(NULL) {
    m_pDefaultMaterial = new Material("__default");
    MaterialProps defaultProps(0.2f, 0.5f, 1.0f, 0.0f);
    m_pDefaultMaterial->SetProperties(defaultProps);
}

MaterialManager::~MaterialManager() {
    RemoveAll();
    if (m_pDefaultMaterial) {
        delete m_pDefaultMaterial;
        m_pDefaultMaterial = NULL;
    }
}

Material* MaterialManager::CreateMaterial(const char* name) {
    Material* mat = new Material(name);
    m_materials.push_back(mat);
    return mat;
}

Material* MaterialManager::GetMaterial(const char* name) {
    for (size_t i = 0; i < m_materials.size(); i++) {
        if (m_materials[i]->GetName() == std::string(name)) {
            return m_materials[i];
        }
    }
    return m_pDefaultMaterial;
}

Material* MaterialManager::GetMaterial(int index) {
    if (index >= 0 && index < (int)m_materials.size()) {
        return m_materials[index];
    }
    return m_pDefaultMaterial;
}

void MaterialManager::RemoveMaterial(const char* name) {
    for (size_t i = 0; i < m_materials.size(); i++) {
        if (m_materials[i]->GetName() == std::string(name)) {
            delete m_materials[i];
            m_materials.erase(m_materials.begin() + i);
            return;
        }
    }
}

void MaterialManager::RemoveAll() {
    for (size_t i = 0; i < m_materials.size(); i++) {
        delete m_materials[i];
    }
    m_materials.clear();
}

Material* MaterialManager::GetDefaultMaterial() {
    return m_pDefaultMaterial;
}

}