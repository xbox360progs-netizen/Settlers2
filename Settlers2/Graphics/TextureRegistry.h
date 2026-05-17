#pragma once

#include <string>
#include <map>
#include <memory>
#include "../World/TileType.h"
#include "SpriteAtlas.h"

// Simple texture registry to centralize access to textures loaded by the engine
// Note: This is a lightweight registry; textures are AddRef'ed when registered
// and released when replaced or cleared.
class TextureRegistry {
public:
    static TextureRegistry& instance();
    LPDIRECT3DTEXTURE9 getTexture(const std::string& name);
    LPDIRECT3DTEXTURE9 getNotFoundTexture() const;
    // Load on demand: try to load texture from disk if not registered yet, using a sensible default path map
    LPDIRECT3DTEXTURE9 getTextureOrLoad(const std::string& name);
    void registerTexture(const std::string& name, LPDIRECT3DTEXTURE9 tex);
    void unregisterTexture(const std::string& name);
    void clear();
    // Initialize with a Direct3DDevice for loading textures on demand
    void initialize(LPDIRECT3DDEVICE9 device = nullptr);
    
    // Register a custom filesystem path for a texture name (for manifest-based loading)
    void registerTexturePath(const std::string& name, const std::string& path);
    // Initialize from a manifest file (INI-like) mapping texture names to file paths
    // If sectionName is empty, load all sections; otherwise load only the specified section
    void initializeFromManifest(const std::string& manifestPath = "textures.ini", const std::string& sectionName = "");
    // Diagnostics helpers for tests/validation
    bool isPathRegistered(const std::string& name);
    bool doesPathExistForName(const std::string& name);
    // Diagnostics helper to log manifest paths and statuses
    void logManifestPathsStatus();
    // Step 8: Flag to disable manifest loading for local development
    void setManifestDisabled(bool disabled);
    bool isManifestDisabled() const;
    // Atlas registry
    void registerAtlas(const std::string& name, std::tr1::shared_ptr<class SpriteAtlas> atlas);
    std::tr1::shared_ptr<class SpriteAtlas> getAtlas(const std::string& name);
    void unregisterAtlas(const std::string& name);

private:
    TextureRegistry()
        : m_notFoundTexture(nullptr)
        , m_device(nullptr)
        , m_threadSafetyInitialized(false)
    {
        initThreadSafety();
    }

    ~TextureRegistry()
    {
        shutdownThreadSafety();
    }
    
public:
    // Call this once at startup to initialize critical section
    void initThreadSafety() {
        if (!m_threadSafetyInitialized) {
            InitializeCriticalSection(&m_cs);
            m_threadSafetyInitialized = true;
        }
    }
    void shutdownThreadSafety() {
        if (m_threadSafetyInitialized) {
            DeleteCriticalSection(&m_cs);
            m_threadSafetyInitialized = false;
        }
    }
    
private:

    LPDIRECT3DTEXTURE9 getTextureImpl(const std::string& name) const;
    LPDIRECT3DTEXTURE9 getTextureOrLoadImpl(const std::string& name);

    std::map<std::string, LPDIRECT3DTEXTURE9> m_textures;
    LPDIRECT3DTEXTURE9 m_notFoundTexture;
    LPDIRECT3DDEVICE9 m_device;
    // Atlas registry (in-memory)
    std::map<std::string, std::tr1::shared_ptr<class SpriteAtlas> > m_atlases;
    // Additional registry for manifest-based texture paths
    std::map<std::string, std::wstring> m_texturePaths;
    // Thread safety (non-const for EnterCriticalSection)
    mutable CRITICAL_SECTION m_cs;
    bool m_threadSafetyInitialized;
};
