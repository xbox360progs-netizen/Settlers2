#include "stdafx.h"
#include "SpriteAtlas.h"
#include "TextureLoader.h"

SpriteAtlas::SpriteAtlas() : m_pTexture(NULL), m_textureWidth(0), m_textureHeight(0) {
    // Pre-reserve to avoid reallocations during parsing
    m_regions.reserve(128);
}

SpriteAtlas::SpriteAtlas(const std::string& name) : m_pTexture(NULL), m_textureWidth(0), m_textureHeight(0) {
    // Pre-reserve to avoid reallocations during parsing
    m_regions.reserve(128);
}

SpriteAtlas::~SpriteAtlas() {
    if (m_pTexture) {
        m_pTexture->Release();
        m_pTexture = NULL;
    }
    m_regions.clear();
    m_nameToIndex.clear();
    m_animations.clear();
}

void SpriteAtlas::SetTexture(LPDIRECT3DTEXTURE9 pTexture) {
    char debugMsg[256];
    sprintf(debugMsg, "[SpriteAtlas] SetTexture: 0x%p\n", pTexture);
    OutputDebugStringA(debugMsg);

    if (m_pTexture != pTexture) {
        if (m_pTexture) {
            m_pTexture->Release();
        }
        m_pTexture = pTexture;
        if (m_pTexture) {
            m_pTexture->AddRef();
            D3DSURFACE_DESC desc;
            if (SUCCEEDED(m_pTexture->GetLevelDesc(0, &desc))) {
                m_textureWidth = desc.Width;
                m_textureHeight = desc.Height;
                sprintf(debugMsg, "[SpriteAtlas] SetTexture texture size: %dx%d\n", m_textureWidth, m_textureHeight);
                OutputDebugStringA(debugMsg);
            }
        } else {
            m_textureWidth = 0;
            m_textureHeight = 0;
        }
    }
}

void SpriteAtlas::Load(IDirect3DDevice9* device, const std::string& filename) {
    char debugMsg[256];
    sprintf(debugMsg, "[SpriteAtlas] Load: %s\n", filename.c_str());
    OutputDebugStringA(debugMsg);

    // Convert to wide string
    std::wstring wfilename(filename.begin(), filename.end());

    // Use TextureLoader for XG loading
    TextureLoader loader(device);
    LPDIRECT3DTEXTURE9 tex = nullptr;
#ifdef _XBOX
    HRESULT hr = loader.LoadAtlas(wfilename.c_str(), &tex);
#else
    HRESULT hr = loader.Load(wfilename.c_str(), &tex);
#endif
    if (SUCCEEDED(hr) && tex) {
        SetTexture(tex);
        tex->Release(); // SetTexture adds ref
        sprintf(debugMsg, "[SpriteAtlas] Load SUCCESS: %s\n", filename.c_str());
        OutputDebugStringA(debugMsg);
    } else {
        sprintf(debugMsg, "[SpriteAtlas] Load FAILED: %s hr=0x%08X\n", filename.c_str(), hr);
        OutputDebugStringA(debugMsg);
    }
}

uint32_t SpriteAtlas::GetTextureWidth() const {
    if (!m_pTexture) return 0;
    D3DSURFACE_DESC desc;
    m_pTexture->GetLevelDesc(0, &desc);
    return desc.Width;
}

uint32_t SpriteAtlas::GetTextureHeight() const {
    if (!m_pTexture) return 0;
    D3DSURFACE_DESC desc;
    m_pTexture->GetLevelDesc(0, &desc);
    return desc.Height;
}

void SpriteAtlas::AddRegion(const SpriteRegion& region) {
    uint32_t newIndex = (uint32_t)m_regions.size();
    m_regions.push_back(region);

    // Map the name to the index for the initial lookup
    if (!region.name.empty()) {
        m_nameToIndex[region.name] = newIndex;
    }
}

uint32_t SpriteAtlas::GetIndex(const char* spriteName) const {
    // This is the "Slow Path" - call only once during object initialization
    std::map<std::string, uint32_t>::const_iterator it = m_nameToIndex.find(spriteName);
    if (it != m_nameToIndex.end()) {
        return it->second;
    }

    // Return a sentinel value (0xFFFFFFFF) if not found
    return 0xFFFFFFFF;
}

const SpriteRegion* SpriteAtlas::GetRegion(uint32_t index) const {
    // This is the "Fast Path" - O(1) access for the render loop
    if (index < m_regions.size()) {
        return &m_regions[index];
    }
    return NULL;
}

void SpriteAtlas::AddAnimation(const SpriteAnimation& anim) {
    m_animations[anim.name] = anim;
}

const SpriteAnimation* SpriteAtlas::GetAnimation(const char* animName) const {
    std::map<std::string, SpriteAnimation>::const_iterator it = m_animations.find(animName);
    if (it != m_animations.end()) {
        return &it->second;
    }
    return NULL;
}


