// BinFileManager.h
#pragma once
#include <xtl.h>      // Xbox 360 main header (instead of windows.h)
#include <string>
#include <map>
#include <memory>
#include "SpriteAtlas.h"

// Forward declaration
class Texture;

// Define standard types if not already defined in xtl.h
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;

// In Xbox 360 SDK, BYTE and DWORD are usually defined in xtl.h
// If not, uncomment these:
// typedef unsigned char BYTE;
// typedef unsigned long DWORD;

typedef std::shared_ptr<SpriteAtlas> AtlasPtr;

class BinFileManager
{
private:
    LPDIRECT3DDEVICE9 m_device;
    std::map<std::string, AtlasPtr> m_loadedAtlases;
    std::map<std::string, std::string> m_atlasPaths; // Store paths for reload after device reset

    bool ParseBinFile(BYTE* buffer, DWORD bufferSize, SpriteAtlas* atlas);
    bool ParseMultiLevelAtlas(BYTE* buffer, DWORD bufferSize, SpriteAtlas* atlas, uint16_t version, bool bigEndian);
    bool ParseAnimationAtlas(BYTE* buffer, DWORD bufferSize, SpriteAtlas* atlas, uint16_t version, bool bigEndian);
public:
    BinFileManager();
    ~BinFileManager();

    void SetDevice(LPDIRECT3DDEVICE9 device) { m_device = device; }

    AtlasPtr LoadAtlas(const std::string& binFilePath, const std::string& name);
    AtlasPtr CreateAtlasFromSingleTexture(LPDIRECT3DDEVICE9 pDevice, const char* name, const char* filePath);
    AtlasPtr GetAtlas(const std::string& name);
    AtlasPtr StealAtlas(const std::string& name);
    bool HasAtlas(const std::string& name) const;
    bool ScanForBinFile(const std::wstring& texturePath, const std::wstring& textureName);
    void Clear();

    // Device loss handling for Xbox 360
    void OnLostDevice();
    void OnResetDevice();
};
