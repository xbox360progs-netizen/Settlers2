// TextureRegistry.cpp - Xbox 360 SDK compatible
#include "StdAfx.h"
#include "TextureRegistry.h"
#include "TextureLoader.h"
#include "SpriteAtlas.h"
#include "BinFileManager.h"
#include <cstdio>
#include <fstream>
#include <io.h>
#include <xtl.h>
// Static BinFileManager pointer (avoids circular dependency in header)
static BinFileManager* g_binFileManager = nullptr;

// Helper to set/get BinFileManager
void SetBinFileManagerStatic(BinFileManager* mgr) { g_binFileManager = mgr; }
BinFileManager* GetBinFileManagerStatic() { return g_binFileManager; }

// Helper: trim leading/trailing whitespace
static std::string TrimString(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return std::string("");
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

TextureRegistry& TextureRegistry::instance() {
    static TextureRegistry s_instance;
    return s_instance;
}

LPDIRECT3DTEXTURE9 TextureRegistry::getTexture(const std::string& name) const {
    std::map<std::string, LPDIRECT3DTEXTURE9>::const_iterator it = m_textures.find(name);
    if (it != m_textures.end()) return it->second;
    return NULL;
}

LPDIRECT3DTEXTURE9 TextureRegistry::getNotFoundTexture() const {
    return m_notFoundTexture;
}

void TextureRegistry::registerAtlas(const std::string& name, std::tr1::shared_ptr<SpriteAtlas> atlas) {
    std::map<std::string, std::tr1::shared_ptr<SpriteAtlas> >::iterator it = m_atlases.find(name);
    if (it != m_atlases.end()) {
        it->second = atlas;
    } else {
        m_atlases.insert(std::pair<std::string, std::tr1::shared_ptr<SpriteAtlas> >(name, atlas));
    }
}

std::tr1::shared_ptr<SpriteAtlas> TextureRegistry::getAtlas(const std::string& name) const {
    std::map<std::string, std::tr1::shared_ptr<SpriteAtlas> >::const_iterator it = m_atlases.find(name);
    if (it != m_atlases.end()) return it->second;
    return std::tr1::shared_ptr<SpriteAtlas>();
}

void TextureRegistry::unregisterAtlas(const std::string& name) {
    m_atlases.erase(name);
}

LPDIRECT3DTEXTURE9 TextureRegistry::getTextureOrLoad(const std::string& name) {
    LPDIRECT3DTEXTURE9 tex = getTexture(name);
    if (tex) {
        OutputDebugStringA(("[TextureRegistry] Texture '" + name + "' found in cache\n").c_str());
        return tex;
    }
    
    // Find path in manifest
    std::map<std::string, std::wstring>::const_iterator itPath = m_texturePaths.find(name);
    if (itPath == m_texturePaths.end()) {
        OutputDebugStringA(("[TextureRegistry] Path not registered for '" + name + "'\n").c_str());
        return m_notFoundTexture;
    }
    
    std::wstring wpath = itPath->second;
    // Build full path for Xbox 360
    if (wpath.find(L"game:\\") != 0 && wpath.find(L"game:/") != 0) {
        static const std::wstring basePath = L"game:\\Media\\Textures\\";
        if (!wpath.empty() && (wpath[0] == L'/' || wpath[0] == L'\\')) {
            wpath = wpath.substr(1);
        }
        wpath = basePath + wpath;
    }
    
    // Check if .bin file exists (replace extension with .bin)
    std::wstring binPathW = wpath;
    size_t dotPos = binPathW.rfind(L'.');
    if (dotPos != std::wstring::npos) {
        binPathW = binPathW.substr(0, dotPos) + L".bin";
    }
    
    // Check if bin file exists using _wfopen
    FILE* binTest = _wfopen(binPathW.c_str(), L"rb");
    if (binTest) {
        // Load as atlas
        BinFileManager* binFileManager = GetBinFileManagerStatic();
        if (binFileManager) {
            std::string binPathA(binPathW.begin(), binPathW.end());
            char debugBuf[512];
        fclose(binTest);
            _snprintf(debugBuf, sizeof(debugBuf), "[TextureRegistry] Found .bin, loading as atlas: %s\n", binPathA.c_str());
            OutputDebugStringA(debugBuf);
            std::tr1::shared_ptr<SpriteAtlas> atlas = binFileManager->LoadAtlas(binPathA, name);
            if (atlas && atlas->GetTexture()) {
                registerTexture(name, atlas->GetTexture());
                registerAtlas(name, atlas);
                return atlas->GetTexture();
            }
        }
    }
    
    // Fallback: load as single texture
    if (!m_device) {
        OutputDebugStringA("[TextureRegistry] No device available\n");
        return m_notFoundTexture;
    }
    
    char debugBuf[512];
    _snprintf(debugBuf, sizeof(debugBuf), "[TextureRegistry] Loading '%s' from path: %s\n", 
              name.c_str(), std::string(wpath.begin(), wpath.end()).c_str());
    OutputDebugStringA(debugBuf);
    
    LPDIRECT3DTEXTURE9 texLoaded = NULL;
    if (m_device) {
        TextureLoader loader(m_device);
        HRESULT hrLoad = loader.Load(wpath.c_str(), &texLoaded);
        _snprintf(debugBuf, sizeof(debugBuf), "[TextureRegistry] Load result: 0x%08X\n", hrLoad);
        OutputDebugStringA(debugBuf);
        
        if (SUCCEEDED(hrLoad) && texLoaded) {
            _snprintf(debugBuf, sizeof(debugBuf), "[TextureRegistry] Successfully loaded '%s'\n", name.c_str());
            OutputDebugStringA(debugBuf);
            registerTexture(name, texLoaded);
            return texLoaded;
        } else {
            _snprintf(debugBuf, sizeof(debugBuf), "[TextureRegistry] Failed to load '%s', HRESULT: 0x%08X\n", 
                      std::string(wpath.begin(), wpath.end()).c_str(), hrLoad);
            OutputDebugStringA(debugBuf);
        }
    }
    
    return m_notFoundTexture;
}


void TextureRegistry::registerTexture(const std::string& name, LPDIRECT3DTEXTURE9 tex) {
    if (name.empty()) return;
    std::map<std::string, LPDIRECT3DTEXTURE9>::iterator it = m_textures.find(name);
    if (it != m_textures.end()) {
        if (it->second) it->second->Release();
        it->second = tex;
        if (tex) tex->AddRef();
        return;
    }
    m_textures.insert(std::pair<std::string, LPDIRECT3DTEXTURE9>(name, tex));
    if (tex) tex->AddRef();
}

void TextureRegistry::unregisterTexture(const std::string& name) {
    std::map<std::string, LPDIRECT3DTEXTURE9>::iterator it = m_textures.find(name);
    if (it != m_textures.end()) {
        if (it->second) it->second->Release();
        m_textures.erase(it);
    }
}

void TextureRegistry::clear() {
    for (std::map<std::string, LPDIRECT3DTEXTURE9>::iterator it = m_textures.begin(); it != m_textures.end(); ++it) {
        if (it->second) it->second->Release();
    }
    m_textures.clear();
    if (m_notFoundTexture) {
        m_notFoundTexture->Release();
        m_notFoundTexture = NULL;
    }
}

void TextureRegistry::initialize(LPDIRECT3DDEVICE9 device) {
    m_device = device;
    if (!m_notFoundTexture && m_device) {
        HRESULT hr = m_device->CreateTexture(1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_notFoundTexture, NULL);
        if (SUCCEEDED(hr) && m_notFoundTexture) {
            D3DLOCKED_RECT lr;
            if (SUCCEEDED(m_notFoundTexture->LockRect(0, &lr, NULL, 0))) {
                unsigned int color = 0xFFFFFFFF;
                *((unsigned int*)lr.pBits) = color;
                m_notFoundTexture->UnlockRect(0);
            }
        } else {
            m_notFoundTexture = NULL;
        }
    }
}

void TextureRegistry::registerTexturePath(const std::string& name, const std::string& path) {
    if (name.empty()) return;
    std::wstring wpath(path.begin(), path.end());
    m_texturePaths[name] = wpath;
}

void TextureRegistry::initializeFromManifest(const std::string& manifestPath, const std::string& sectionName) {
    if (manifestPath.empty()) {
        OutputDebugStringA("[TextureRegistry] initializeFromManifest: manifestPath is empty\n");
        return;
    }
    
    if (isManifestDisabled()) {
        OutputDebugStringA("[TextureRegistry] initializeFromManifest: manifest disabled\n");
        return;
    }
    
    char logBuf[256];
    _snprintf(logBuf, sizeof(logBuf), "[TextureRegistry] initializeFromManifest: opening %s section=%s\n", manifestPath.c_str(), sectionName.c_str());
    OutputDebugStringA(logBuf);
    
    std::wstring wpath(manifestPath.begin(), manifestPath.end());
    FILE* fin = NULL;
    errno_t err = _wfopen_s(&fin, wpath.c_str(), L"r");
    if (err != 0 || !fin) {
        _snprintf(logBuf, sizeof(logBuf), "[TextureRegistry] FAILED to open manifest: error=%d\n", err);
        OutputDebugStringA(logBuf);
        return;
    }
    
    OutputDebugStringA("[TextureRegistry] Manifest file opened\n");
    
    std::string currentSection = "Global";
    char line[512];
    int lineNum = 0;
    int totalLoaded = 0;
    bool sectionFilterEnabled = !sectionName.empty();
    
    while (fgets(line, sizeof(line), fin)) {
        lineNum++;
        std::string trimmed = TrimString(line);
        if (trimmed.empty()) continue;
        if (trimmed[0] == '#' || trimmed[0] == ';') continue;
        
        if (trimmed[0] == '[' && trimmed[trimmed.length() - 1] == ']') {
            currentSection = trimmed.substr(1, trimmed.length() - 2);
            _snprintf(logBuf, sizeof(logBuf), "[TextureRegistry] Section: %s\n", currentSection.c_str());
            OutputDebugStringA(logBuf);
            continue;
        }
        
        if (sectionFilterEnabled && currentSection != sectionName) continue;
        
        size_t eq = trimmed.find('=');
        if (eq == std::string::npos) continue;
        
        std::string name = TrimString(trimmed.substr(0, eq));
        std::string path = TrimString(trimmed.substr(eq + 1));
        
        _snprintf(logBuf, sizeof(logBuf), "[TextureRegistry] Parsing: %s = %s\n", name.c_str(), path.c_str());
        OutputDebugStringA(logBuf);
        
        if (!name.empty() && !path.empty()) {
            // Регистрируем путь как есть (относительный путь)
            registerTexturePath(name, path);
            totalLoaded++;
            _snprintf(logBuf, sizeof(logBuf), "[TextureRegistry] Registered: %s -> %s\n", name.c_str(), path.c_str());
            OutputDebugStringA(logBuf);
        }
    }
    
    fclose(fin);
    _snprintf(logBuf, sizeof(logBuf), "[TextureRegistry] Total loaded: %d\n", totalLoaded);
    OutputDebugStringA(logBuf);
}

bool TextureRegistry::isPathRegistered(const std::string& name) const {
    return m_texturePaths.find(name) != m_texturePaths.end();
}

bool TextureRegistry::doesPathExistForName(const std::string& name) const {
    std::map<std::string, std::wstring>::const_iterator it = m_texturePaths.find(name);
    if (it == m_texturePaths.end()) return false;
    return (_waccess(it->second.c_str(), 0) == 0);
}

void TextureRegistry::logManifestPathsStatus() const {
    if (m_texturePaths.empty()) {
        OutputDebugStringA("[TextureRegistry] No paths registered\n");
        return;
    }
    char buf[256];
    _snprintf(buf, sizeof(buf), "[TextureRegistry] Total paths registered: %d\n", (int)m_texturePaths.size());
    OutputDebugStringA(buf);
    for (std::map<std::string, std::wstring>::const_iterator kv = m_texturePaths.begin(); 
         kv != m_texturePaths.end(); ++kv) {
        bool exists = doesPathExistForName(kv->first);
        char log[256];
        _snprintf(log, sizeof(log), "[TextureRegistry] %s -> exists=%d\n", kv->first.c_str(), exists ? 1 : 0);
        OutputDebugStringA(log);
    }
    int missing = 0;
    for (std::map<std::string, std::wstring>::const_iterator kv = m_texturePaths.begin(); 
         kv != m_texturePaths.end(); ++kv) {
        if (!doesPathExistForName(kv->first)) missing++;
    }
    if (missing > 0) {
        _snprintf(buf, sizeof(buf), "[TextureRegistry] Missing entries: %d\n", missing);
        OutputDebugStringA(buf);
    } else {
        OutputDebugStringA("[TextureRegistry] All registered paths exist\n");
    }
}

// Step 8: Flag to disable manifest loading for local development
static bool g_manifestDisabled = false;

void TextureRegistry::setManifestDisabled(bool disabled) {
    g_manifestDisabled = disabled;
}

bool TextureRegistry::isManifestDisabled() const {
    return g_manifestDisabled;
}
