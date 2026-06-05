// BinFileManager.cpp
#include "stdafx.h"
#include "BinFileManager.h"
#include "SpriteAtlas.h"
#include "Texture.h"
#include "TextureLoader.h"
#include <fstream>
#include <xtl.h>
#include <cstring>
#include <utility>

// Helper functions for reading with endianness conversion
static inline uint32_t ReadU32LE(const BYTE* ptr)
{
    return  (uint32_t(ptr[0])      ) |
            (uint32_t(ptr[1]) << 8 ) |
            (uint32_t(ptr[2]) << 16) |
            (uint32_t(ptr[3]) << 24);
}

static inline uint16_t ReadU16LE(const BYTE* ptr)
{
    return  (uint16_t(ptr[0])      ) |
            (uint16_t(ptr[1]) << 8 );
}

static inline uint32_t ReadU32BE(const BYTE* ptr)
{
    return  (uint32_t(ptr[0]) << 24) |
            (uint32_t(ptr[1]) << 16) |
            (uint32_t(ptr[2]) << 8 ) |
            (uint32_t(ptr[3])      );
}
static inline uint16_t ReadU16BE(const BYTE* ptr)
{
    return  (uint16_t(ptr[0]) << 8 ) |
            (uint16_t(ptr[1])      );
}
// Universal read with selected byte order
static inline uint32_t ReadU32(const BYTE* ptr, bool bigEndian)
{
    return bigEndian ? ReadU32BE(ptr) : ReadU32LE(ptr);
}
static inline uint16_t ReadU16(const BYTE* ptr, bool bigEndian)
{
    return bigEndian ? ReadU16BE(ptr) : ReadU16LE(ptr);
}
static inline float ReadF32(const BYTE* ptr, bool bigEndian)
{
    uint32_t raw = ReadU32(ptr, bigEndian);
    float f;
    memcpy(&f, &raw, sizeof(float));
    return f;
}
namespace {
    const uint32_t kMaxFileSize     = 2 * 1024 * 1024; // 2 MB safety cap
    const uint32_t kMaxSpriteCount  = 8192;
    const uint32_t kMaxFrameDim     = 4096;
}

// Static function to replace lambda for header validation
static bool IsHeaderValid(uint16_t v, uint16_t h, uint8_t t, uint32_t bufferSize)
{
    if (v == 0 || v > 10 || h > bufferSize) return false;
    switch (t) {
    case 1: // MultiLevel
        return (v == 1 || v == 2 || v == 3 || v == 4 || v == 5 || v == 6 || v == 7 || v == 8 || v == 9 || v == 10) && h == 24;
    default:
        return false;
    }
}

BinFileManager::BinFileManager()
    : m_device(NULL)
{
}

BinFileManager::~BinFileManager()
{
    Clear();
}

void BinFileManager::Clear()
{
    m_loadedAtlases.clear();
}

AtlasPtr BinFileManager::LoadAtlas(const std::string& binFilePath, const std::string& name)
{
    if (HasAtlas(name)) {
        return GetAtlas(name);
    }
    
    std::wstring wBinPath(binFilePath.begin(), binFilePath.end());
    char pathA[512];
    WideCharToMultiByte(CP_ACP, 0, wBinPath.c_str(), -1, pathA, 512, NULL, NULL);
    
    HANDLE hFile = CreateFileA(
        pathA,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (hFile == INVALID_HANDLE_VALUE) {
        char debugMsg[512];
        sprintf(debugMsg, "[BinFileManager] Failed to open file: %s\n", pathA);
        OutputDebugStringA(debugMsg);
        return NULL;
    }
    
    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == 0 || fileSize > kMaxFileSize) {
        CloseHandle(hFile);
        OutputDebugStringA("[BinFileManager] Invalid file size\n");
        return NULL;
    }
    
    BYTE* buffer = new(std::nothrow) BYTE[fileSize]; 
    if (!buffer) {
        CloseHandle(hFile);
        OutputDebugStringA("[BinFileManager] Memory allocation failed\n");
        return NULL;
    }
    
    DWORD bytesRead = 0;
    if (!ReadFile(hFile, buffer, fileSize, &bytesRead, NULL) || bytesRead != fileSize) {
        delete[] buffer;
        CloseHandle(hFile);
        OutputDebugStringA("[BinFileManager] Failed to read file\n");
        return NULL;
    }
    
    CloseHandle(hFile);
    
    AtlasPtr atlas(new(std::nothrow) SpriteAtlas(name));
    if (!atlas) {
        delete[] buffer;
        OutputDebugStringA("[BinFileManager] Failed to create atlas\n");
        return NULL;
    }
    
    bool parseSuccess = ParseBinFile(buffer, fileSize, atlas.get());
    delete[] buffer; 
    
    if (parseSuccess) {
        // Load PNG texture
        std::string pngPath = binFilePath;
        size_t dotPos = pngPath.rfind('.');
        if (dotPos != std::string::npos) {
            pngPath = pngPath.substr(0, dotPos) + ".png";
        }
        
        char debugMsg[512];
        sprintf(debugMsg, "[BinFileManager] Loading PNG for atlas: %s\n", pngPath.c_str());
        OutputDebugStringA(debugMsg);
        
        // Use TextureLoader instead of Texture class
        TextureLoader loader(m_device);
        LPDIRECT3DTEXTURE9 pD3DTex = nullptr;
        
        // Convert path to WideString
        std::wstring wPngPath(pngPath.begin(), pngPath.end());
        
        // Load through our stable method
        if (SUCCEEDED(loader.Load(wPngPath.c_str(), &pD3DTex))) {
            // Set texture to atlas
            atlas->SetTexture(pD3DTex);
            
            // Release local reference since atlas owns the texture now
            pD3DTex->Release();
            
            m_loadedAtlases[name] = atlas;
            return atlas;
        } else {
            OutputDebugStringA("[BinFileManager] Failed to load PNG texture for atlas\n");
            return NULL; 
        }
    }
    
    OutputDebugStringA("[BinFileManager] ParseBinFile FAILED\n");
    return NULL;
}

bool BinFileManager::ParseBinFile(BYTE* buffer, DWORD bufferSize, SpriteAtlas* atlas)
{
    if (bufferSize < 5) {
        return false;
    }

    // Read atlas type from 5th byte
    uint8_t atlasType = buffer[4]; // 5th byte

    char debugHeader[256];
    sprintf(debugHeader, "[ParseBinFile] Raw type: %d\n", atlasType);
    OutputDebugStringA(debugHeader);

    // Support MultiLevelAtlas (type 1) and AnimationAtlas (type 2)
    if (atlasType == 1) {
        // Version and headerSize are 2 bytes each
        uint16_t versionBE    = ReadU16BE(buffer + 0);
        uint16_t headerSizeBE = ReadU16BE(buffer + 2);
        uint16_t versionLE    = ReadU16LE(buffer + 0);
        uint16_t headerSizeLE = ReadU16LE(buffer + 2);

        sprintf(debugHeader, "[ParseBinFile] MultiLevel header: type=%d, verBE=%d, verLE=%d, hdrBE=%d, hdrLE=%d\n",
                atlasType, versionBE, versionLE, headerSizeBE, headerSizeLE);
        OutputDebugStringA(debugHeader);

        bool validBE = IsHeaderValid(versionBE, headerSizeBE, atlasType, bufferSize);
        bool validLE = IsHeaderValid(versionLE, headerSizeLE, atlasType, bufferSize);
        bool bigEndian = false;

        if (validBE && !validLE) bigEndian = true;
        else if (!validBE && validLE) bigEndian = false;
        else if (validBE && validLE) bigEndian = true;
        else {
            if ((versionBE >= 3 && versionBE <= 10) && headerSizeBE == 24) {
                bigEndian = true;
                OutputDebugStringA("[ParseBinFile] Forced BigEndian for MultiLevel\n");
            } else {
                OutputDebugStringA("[ParseBinFile] Invalid MultiLevel file header!\n");
                return false;
            }
        }

        uint16_t version    = bigEndian ? versionBE    : versionLE;
        uint16_t headerSize = bigEndian ? headerSizeBE : headerSizeLE;

        sprintf(debugHeader, "[ParseBinFile] Determined MultiLevel version: %d, endian: %s\n",
                version, bigEndian ? "big" : "little");
        OutputDebugStringA(debugHeader);

        OutputDebugStringA("[ParseBinFile] Starting MultiLevel parsing\n");
        return ParseMultiLevelAtlas(buffer, bufferSize, atlas, version, bigEndian);
    }
    else if (atlasType == 2) {
        // AnimationAtlas
        uint16_t versionBE    = ReadU16BE(buffer + 0);
        uint16_t headerSizeBE = ReadU16BE(buffer + 2);
        uint16_t versionLE    = ReadU16LE(buffer + 0);
        uint16_t headerSizeLE = ReadU16LE(buffer + 2);

        sprintf(debugHeader, "[ParseBinFile] AnimationAtlas header: type=%d, verBE=%d, verLE=%d, hdrBE=%d, hdrLE=%d\n",
                atlasType, versionBE, versionLE, headerSizeBE, headerSizeLE);
        OutputDebugStringA(debugHeader);

        // AnimationAtlas uses same validation as MultiLevel for now
        bool validBE = IsHeaderValid(versionBE, headerSizeBE, atlasType, bufferSize);
        bool validLE = IsHeaderValid(versionLE, headerSizeLE, atlasType, bufferSize);
        bool bigEndian = false;

        if (validBE && !validLE) bigEndian = true;
        else if (!validBE && validLE) bigEndian = false;
        else if (validBE && validLE) bigEndian = true;
        else {
            // Default to big endian for animation atlases
            bigEndian = true;
            OutputDebugStringA("[ParseBinFile] Forced BigEndian for AnimationAtlas\n");
        }

        uint16_t version = bigEndian ? versionBE : versionLE;

        sprintf(debugHeader, "[ParseBinFile] Determined AnimationAtlas version: %d, endian: %s\n",
                version, bigEndian ? "big" : "little");
        OutputDebugStringA(debugHeader);

        OutputDebugStringA("[ParseBinFile] Starting AnimationAtlas parsing\n");
        return ParseAnimationAtlas(buffer, bufferSize, atlas, version, bigEndian);
    }
    else {
        char unknownType[128];
        sprintf(unknownType, "[ParseBinFile] Unknown atlas type: %d (supported: MultiLevelAtlas type 1, AnimationAtlas type 2)\n", atlasType);
        OutputDebugStringA(unknownType);
        return false;
    }
}

bool BinFileManager::ParseMultiLevelAtlas(BYTE* buffer, DWORD bufferSize, SpriteAtlas* atlas, uint16_t version, bool bigEndian)
{
    OutputDebugStringA("[ParseMultiLevelAtlas] Starting parsing\n");
    
    uint32_t nameLen = ReadU32(buffer + 5, bigEndian);
    uint32_t nameStart = 9;
    if (nameLen == 0 || nameLen > 256 || nameStart + nameLen > bufferSize) {
        OutputDebugStringA("[ParseMultiLevelAtlas] Error reading atlas name\n");
        return false;
    }

    DWORD pos = nameStart + nameLen;
    std::string atlasName(reinterpret_cast<char const*>(buffer + nameStart), nameLen);
    char debugMsg[256];
    sprintf(debugMsg, "[ParseMultiLevelAtlas] Atlas name: %s\n", atlasName.c_str());
    OutputDebugStringA(debugMsg);

    uint32_t prefixLen = ReadU32(buffer + pos, bigEndian); pos += 4;
    if (prefixLen == 0 || prefixLen > 256 || pos + prefixLen > bufferSize) {
        OutputDebugStringA("[ParseMultiLevelAtlas] Error reading prefix\n");
        return false;
    }
    std::string prefix(reinterpret_cast<char const*>(buffer + pos), prefixLen);
    pos += prefixLen;
    sprintf(debugMsg, "[ParseMultiLevelAtlas] Prefix: %s\n", prefix.c_str());
    OutputDebugStringA(debugMsg);

    uint32_t spritesPerLevel = ReadU32(buffer + pos, bigEndian); pos += 4;
    bool autoArrange = (buffer[pos] != 0); pos += 1;
    sprintf(debugMsg, "[ParseMultiLevelAtlas] SpritesPerLevel: %d, AutoArrange: %s\n",
            spritesPerLevel, autoArrange ? "true" : "false");
    OutputDebugStringA(debugMsg);

    uint32_t groupCount = ReadU32(buffer + pos, bigEndian); pos += 4;
    sprintf(debugMsg, "[ParseMultiLevelAtlas] Group count: %d\n", groupCount);
    OutputDebugStringA(debugMsg);
    
    if (groupCount > 512) {
        OutputDebugStringA("[ParseMultiLevelAtlas] Too many groups\n");
        return false;
    }
    
    // Read groups and collect names
    std::vector<std::string> groupNames;
    std::vector<std::vector<uint32_t>> groupSpriteIndices;
    
    for (uint32_t i = 0; i < groupCount; i++) {
        if (pos + 4 > bufferSize) {
            sprintf(debugMsg, "[ParseMultiLevelAtlas] Error in group %d: insufficient data\n", i);
            OutputDebugStringA(debugMsg);
            return false;
        }
        uint32_t groupNameLen = ReadU32(buffer + pos, bigEndian); pos += 4;
        if (groupNameLen > 256 || pos + groupNameLen > bufferSize) {
            sprintf(debugMsg, "[ParseMultiLevelAtlas] Error in group %d: invalid name length\n", i);
            OutputDebugStringA(debugMsg);
            return false;
        }
        
        std::string groupName(reinterpret_cast<char const*>(buffer + pos), groupNameLen);
        pos += groupNameLen;
        groupNames.push_back(groupName);
        
        if (pos + 4 > bufferSize) {
            sprintf(debugMsg, "[ParseMultiLevelAtlas] Error in group %d: no sprite data\n", i);
            OutputDebugStringA(debugMsg);
            return false;
        }
        uint32_t spriteCount = ReadU32(buffer + pos, bigEndian); pos += 4;
        if (spriteCount > kMaxSpriteCount) {
            sprintf(debugMsg, "[ParseMultiLevelAtlas] Error in group %d: too many sprites\n", i);
            OutputDebugStringA(debugMsg);
            return false;
        }
        
        std::vector<uint32_t> spriteIndices;
        for (uint32_t j = 0; j < spriteCount; j++) {
            if (pos + 4 > bufferSize) {
                sprintf(debugMsg, "[ParseMultiLevelAtlas] Error in group %d, sprite %d: insufficient data\n", i, j);
                OutputDebugStringA(debugMsg);
                return false;
            }
            uint32_t spriteIndex = ReadU32(buffer + pos, bigEndian); pos += 4;
            spriteIndices.push_back(spriteIndex);
        }
        groupSpriteIndices.push_back(spriteIndices);
        
        sprintf(debugMsg, "[ParseMultiLevelAtlas] Group %d: %s (%d sprites)\n", i, groupName.c_str(), spriteCount);
        OutputDebugStringA(debugMsg);
    }

    if (pos + 4 > bufferSize) {
        OutputDebugStringA("[ParseMultiLevelAtlas] Error reading total sprite count\n");
        return false;
    }
    uint32_t totalSpriteCount = ReadU32(buffer + pos, bigEndian); pos += 4;
    sprintf(debugMsg, "[ParseMultiLevelAtlas] Total sprites: %d\n", totalSpriteCount);
    OutputDebugStringA(debugMsg);
    
    if (totalSpriteCount == 0 || totalSpriteCount > kMaxSpriteCount) {
        OutputDebugStringA("[ParseMultiLevelAtlas] Invalid sprite count\n");
        return false;
    }

    // Now read sprites and create SpriteRegions
    for (uint32_t spriteIdx = 0; spriteIdx < totalSpriteCount; spriteIdx++) {
        if (pos + 20 > bufferSize) {
            sprintf(debugMsg, "[ParseMultiLevelAtlas] Error reading sprite %d: insufficient data\n", spriteIdx);
            OutputDebugStringA(debugMsg);
            return false;
        }

        uint32_t x = ReadU32(buffer + pos, bigEndian); pos += 4;
        uint32_t y = ReadU32(buffer + pos, bigEndian); pos += 4;
        uint32_t width = ReadU32(buffer + pos, bigEndian); pos += 4;
        uint32_t height = ReadU32(buffer + pos, bigEndian); pos += 4;

        // Pivots are in BigEndian (as in C# code)
        uint16_t pivotX = ReadU16BE(buffer + pos); pos += 2;
        uint16_t pivotY = ReadU16BE(buffer + pos); pos += 2;

        float uv_min_x = 0.0f, uv_min_y = 0.0f;
        float uv_max_x = 1.0f, uv_max_y = 1.0f;
        if (version >= 2) {
            if (pos + 16 > bufferSize) {
                sprintf(debugMsg, "[ParseMultiLevelAtlas] Error reading UV for sprite %d\n", spriteIdx);
                OutputDebugStringA(debugMsg);
                return false;
            }
            uv_min_x = ReadF32(buffer + pos, bigEndian); pos += 4;
            uv_min_y = ReadF32(buffer + pos, bigEndian); pos += 4;
            uv_max_x = ReadF32(buffer + pos, bigEndian); pos += 4;
            uv_max_y = ReadF32(buffer + pos, bigEndian); pos += 4;
        }

        bool isPacked = false;
        uint32_t blockOffsetX = 0, blockOffsetY = 0;
        if (version >= 4) {
            if (pos >= bufferSize) {
                sprintf(debugMsg, "[ParseMultiLevelAtlas] Error reading IsPacked for sprite %d\n", spriteIdx);
                OutputDebugStringA(debugMsg);
                return false;
            }
            isPacked = (buffer[pos] != 0); pos += 1;
            if (isPacked) {
                if (pos + 8 > bufferSize) {
                    sprintf(debugMsg, "[ParseMultiLevelAtlas] Error reading BlockOffset for sprite %d\n", spriteIdx);
                    OutputDebugStringA(debugMsg);
                    return false;
                }
                blockOffsetX = ReadU32(buffer + pos, bigEndian); pos += 4;
                blockOffsetY = ReadU32(buffer + pos, bigEndian); pos += 4;
            }
        }

        uint32_t collWidth = 1, collHeight = 1;
        bool blocksMovement = true, isTrigger = false;
        if (version >= 3) {
            if (pos + 10 > bufferSize) {
                sprintf(debugMsg, "[ParseMultiLevelAtlas] Error reading collider for sprite %d\n", spriteIdx);
                OutputDebugStringA(debugMsg);
                return false;
            }
            collWidth = ReadU32(buffer + pos, bigEndian); pos += 4;
            collHeight = ReadU32(buffer + pos, bigEndian); pos += 4;
            blocksMovement = (buffer[pos] != 0); pos += 1;
            isTrigger = (buffer[pos] != 0); pos += 1;
        }

        // New fields from version >= 6
        std::string spriteName;
        bool flipX = false, flipY = false;
        if (version >= 6) {
            if (pos + 4 > bufferSize) {
                sprintf(debugMsg, "[ParseMultiLevelAtlas] Error reading sprite name for sprite %d\n", spriteIdx);
                OutputDebugStringA(debugMsg);
                return false;
            }
            uint32_t nameLenSprite = ReadU32(buffer + pos, bigEndian); pos += 4;
            if (nameLenSprite > 0) {
                if (pos + nameLenSprite > bufferSize) {
                    sprintf(debugMsg, "[ParseMultiLevelAtlas] Error reading sprite name for sprite %d: insufficient data\n", spriteIdx);
                    OutputDebugStringA(debugMsg);
                    return false;
                }
                spriteName = std::string(reinterpret_cast<char const*>(buffer + pos), nameLenSprite);
                pos += nameLenSprite;
            }

            if (pos + 2 > bufferSize) {
                sprintf(debugMsg, "[ParseMultiLevelAtlas] Error reading sprite transforms for sprite %d\n", spriteIdx);
                OutputDebugStringA(debugMsg);
                return false;
            }
            flipX = (buffer[pos] != 0); pos += 1;
            flipY = (buffer[pos] != 0); pos += 1;
        }

        // Starting from version 7 - read collider offset (signed int32)
        int collOffX = 0, collOffY = 0;
        if (version >= 7) {
            if (pos + 8 > bufferSize) {
                sprintf(debugMsg, "[ParseMultiLevelAtlas] Error reading collider offset for sprite %d\n", spriteIdx);
                OutputDebugStringA(debugMsg);
                return false;
            }
            collOffX = (int)ReadU32(buffer + pos, bigEndian); pos += 4;
            collOffY = (int)ReadU32(buffer + pos, bigEndian); pos += 4;
        }

        // Starting from version 8 - read collision tile mask
        std::vector<std::pair<int,int> > collMask;
        if (version >= 8) {
            if (pos + 4 > bufferSize) {
                sprintf(debugMsg, "[ParseMultiLevelAtlas] Error reading collision mask for sprite %d\n", spriteIdx);
                OutputDebugStringA(debugMsg);
                return false;
            }
            uint32_t maskCount = ReadU32(buffer + pos, bigEndian); pos += 4;
            if (maskCount > 1024) {
                sprintf(debugMsg, "[ParseMultiLevelAtlas] Too many tiles in mask for sprite %d: %d\n", spriteIdx, maskCount);
                OutputDebugStringA(debugMsg);
                return false;
            }
            for (uint32_t m = 0; m < maskCount; m++) {
                if (pos + 8 > bufferSize) {
                    sprintf(debugMsg, "[ParseMultiLevelAtlas] Error reading mask element %d for sprite %d\n", m, spriteIdx);
                    OutputDebugStringA(debugMsg);
                    return false;
                }
                int dx = (int)ReadU32(buffer + pos, bigEndian); pos += 4;
                int dy = (int)ReadU32(buffer + pos, bigEndian); pos += 4;
                collMask.push_back(std::make_pair(dx, dy));
            }
        }

        // Starting from version 9 - read node weight entries
        std::vector<NodeWeightEntry> nodeWeightEntries;
        if (version >= 9) {
            if (pos + 4 > bufferSize) {
                sprintf(debugMsg, "[ParseMultiLevelAtlas] Error reading NodeWeight count for sprite %d\n", spriteIdx);
                OutputDebugStringA(debugMsg);
                return false;
            }
            uint32_t entryCount = ReadU32(buffer + pos, bigEndian); pos += 4;
            if (entryCount > 1024) {
                sprintf(debugMsg, "[ParseMultiLevelAtlas] Too many NodeWeight entries for sprite %d: %d\n", spriteIdx, entryCount);
                OutputDebugStringA(debugMsg);
                return false;
            }
            for (uint32_t ei = 0; ei < entryCount; ei++) {
                if (pos + 9 > bufferSize) {
                    sprintf(debugMsg, "[ParseMultiLevelAtlas] Error reading NodeWeight entry %d for sprite %d\n", ei, spriteIdx);
                    OutputDebugStringA(debugMsg);
                    return false;
                }
                NodeWeightEntry e;
                e.nx = (int)ReadU32(buffer + pos, bigEndian); pos += 4;
                e.ny = (int)ReadU32(buffer + pos, bigEndian); pos += 4;
                e.weight = buffer[pos]; pos += 1;
                nodeWeightEntries.push_back(e);
            }
        }

        // Starting from version 10 - read entranceX, entranceY and isBuilding
        int entranceX = 0, entranceY = 0;
        bool isBuilding = false;
        if (version >= 10) {
            if (pos + 8 > bufferSize) {
                sprintf(debugMsg, "[ParseMultiLevelAtlas] Error reading entrance for sprite %d\n", spriteIdx);
                OutputDebugStringA(debugMsg);
                return false;
            }
            entranceX = (int)ReadU32(buffer + pos, bigEndian); pos += 4;
            entranceY = (int)ReadU32(buffer + pos, bigEndian); pos += 4;
            if (pos >= bufferSize) {
                sprintf(debugMsg, "[ParseMultiLevelAtlas] Error reading isBuilding for sprite %d\n", spriteIdx);
                OutputDebugStringA(debugMsg);
                return false;
            }
            isBuilding = (buffer[pos] != 0); pos += 1;
        }

        if (width == 0 || height == 0 || width > kMaxFrameDim || height > kMaxFrameDim) {
            sprintf(debugMsg, "[ParseMultiLevelAtlas] Skipping sprite %d: invalid dimensions\n", spriteIdx);
            OutputDebugStringA(debugMsg);
            continue;
        }

        // Default pivot 0xFFFF -> center (width/2, height/2)
        if (pivotX == 0xFFFF) pivotX = width / 2;
        if (pivotY == 0xFFFF) pivotY = height / 2;

        sprintf(debugMsg, "[ParseMultiLevelAtlas] Sprite %d: %dx%d, pivot=(%d,%d), UV=(%.3f,%.3f)-(%.3f,%.3f)\n",
                spriteIdx, width, height, pivotX, pivotY, uv_min_x, uv_min_y, uv_max_x, uv_max_y);
        OutputDebugStringA(debugMsg);

        // Create SpriteRegion for Xbox 360 rendering
        SpriteRegion reg;
        reg.name = spriteName;
        reg.width = width;
        reg.height = height;
        reg.pivotX = (float)pivotX;
        reg.pivotY = (float)pivotY;

        // UVs are pre-calculated in C# tool, assign them directly
        reg.u0 = uv_min_x;
        reg.v0 = uv_min_y;
        reg.u1 = uv_max_x;
        reg.v1 = uv_max_y;

        reg.flipX = flipX;
        reg.flipY = flipY;
        reg.collWidth = collWidth;
        reg.collHeight = collHeight;
        reg.collOffX = collOffX;
        reg.collOffY = collOffY;
        reg.blocksMovement = blocksMovement;
        reg.isTrigger = isTrigger;
        reg.collMask = collMask;
        reg.nodeWeightEntries = nodeWeightEntries;
        reg.entranceX = entranceX;
        reg.entranceY = entranceY;
        reg.isBuilding = isBuilding;

        // Add to atlas storage
        atlas->AddRegion(reg);
    }

    // Store groups in atlas
    for (uint32_t i = 0; i < groupCount; i++) {
        atlas->AddGroup(groupNames[i], groupSpriteIndices[i]);
    }

    sprintf(debugMsg, "[ParseMultiLevelAtlas] Successfully processed %d sprites, %d groups\n",
            totalSpriteCount, groupCount);
    OutputDebugStringA(debugMsg);
    OutputDebugStringA("[ParseMultiLevelAtlas] Parsing completed successfully\n");
    return totalSpriteCount > 0;
}

AtlasPtr BinFileManager::GetAtlas(const std::string& name)
{
    std::map<std::string, AtlasPtr>::const_iterator it = m_loadedAtlases.find(name);
    if (it != m_loadedAtlases.end()) {
        return it->second;
    }
    return NULL;
}

bool BinFileManager::HasAtlas(const std::string& name) const
{
    return m_loadedAtlases.find(name) != m_loadedAtlases.end();
}

AtlasPtr BinFileManager::StealAtlas(const std::string& name)
{
    std::map<std::string, AtlasPtr>::iterator it = m_loadedAtlases.find(name);
    if (it == m_loadedAtlases.end()) return NULL;
    AtlasPtr atlas = it->second;
    m_loadedAtlases.erase(it);
    return atlas;
}

bool BinFileManager::ScanForBinFile(const std::wstring& texturePath, const std::wstring& textureName)
{
    std::wstring binPath = texturePath;
    size_t dotPos = binPath.find_last_of(L'.');
    if (dotPos != std::wstring::npos) {
        binPath = binPath.substr(0, dotPos) + L".bin";
    } else {
        binPath += L".bin";
    }

    std::string nameA(textureName.begin(), textureName.end());
    AtlasPtr atlas = LoadAtlas(std::string(binPath.begin(), binPath.end()), nameA);
    if (atlas) {
        return true;
    }

    return false;
}

AtlasPtr BinFileManager::CreateAtlasFromSingleTexture(LPDIRECT3DDEVICE9 pDevice, const char* name, const char* filePath)
{
    char debugMsg[256];
    sprintf(debugMsg, "[BinFileManager] CreateAtlasFromSingleTexture: name=%s, path=%s\n", name, filePath);
    OutputDebugStringA(debugMsg);

    // 1. Check cache
    if (HasAtlas(name)) {
        return GetAtlas(name);
    }

    // 2. Use new TextureLoader instead of pTex->Load
    TextureLoader loader(pDevice);
    LPDIRECT3DTEXTURE9 pD3DTex = nullptr;

    // Convert path to WideString for loader
    std::string pathStr(filePath);
    std::wstring wPath(pathStr.begin(), pathStr.end());

    // Load through our fixed method (from memory, with game:\ support)
    HRESULT hr = loader.Load(wPath.c_str(), &pD3DTex);

    if (FAILED(hr) || !pD3DTex) {
        sprintf(debugMsg, "[BinFileManager] Failed to load texture: %s, hr=0x%08X\n", filePath, hr);
        OutputDebugStringA(debugMsg);
        return NULL;
    }

    // 3. Get texture dimensions
    D3DSURFACE_DESC desc;
    pD3DTex->GetLevelDesc(0, &desc);

    // 4. Create atlas
    AtlasPtr atlas(new SpriteAtlas(name));
    atlas->SetTexture(pD3DTex); // SpriteAtlas should AddRef() internally

    // 5. Create a single region covering the full texture
    SpriteRegion region;
    region.name = "default";
    region.width = desc.Width;
    region.height = desc.Height;
    region.u0 = 0.0f; region.v0 = 0.0f;
    region.u1 = 1.0f; region.v1 = 1.0f;
    region.pivotX = region.width * 0.5f; region.pivotY = region.height * 0.5f;
    region.flipX = false; region.flipY = false;
    region.collWidth = 1;
    region.collHeight = 1;
    region.blocksMovement = true;
    region.isTrigger = false;
    region.isBuilding = false;
    region.nodeWeightEntries.clear(); // Default: no entries

    atlas->AddRegion(region);

    // 6. Register and clean up
    m_loadedAtlases[name] = atlas;
    
    // Release local reference since atlas now owns the texture
    pD3DTex->Release();

    sprintf(debugMsg, "[BinFileManager] Atlas created: %s (%ux%u)\n", name, desc.Width, desc.Height);
    OutputDebugStringA(debugMsg);

    return atlas;
}

bool BinFileManager::ParseAnimationAtlas(BYTE* buffer, DWORD bufferSize, SpriteAtlas* atlas, uint16_t version, bool bigEndian)
{
    OutputDebugStringA("[ParseAnimationAtlas] Starting parsing\n");

    DWORD pos = 5; // Type byte was at index 4

    // 1. Animation Name
    if (pos + 4 > bufferSize) {
        OutputDebugStringA("[ParseAnimationAtlas] Error reading animation name\n");
        return false;
    }
    uint32_t nameLen = ReadU32(buffer + pos, bigEndian); pos += 4;
    if (nameLen == 0 || nameLen > 256 || pos + nameLen > bufferSize) {
        OutputDebugStringA("[ParseAnimationAtlas] Invalid animation name length\n");
        return false;
    }
    std::string animName(reinterpret_cast<char const*>(buffer + pos), nameLen); pos += nameLen;
    char debugMsg[256];
    sprintf(debugMsg, "[ParseAnimationAtlas] Animation name: %s\n", animName.c_str());
    OutputDebugStringA(debugMsg);

    // 2. Playback Metadata
    if (pos + 20 > bufferSize) {
        OutputDebugStringA("[ParseAnimationAtlas] Error reading playback metadata\n");
        return false;
    }
    uint32_t frameRate = ReadU32(buffer + pos, bigEndian); pos += 4;
    bool loop = (buffer[pos++] != 0);
    bool pingPong = (buffer[pos++] != 0);
    float speed = ReadF32(buffer + pos, bigEndian); pos += 4;
    uint32_t startFrame = ReadU32(buffer + pos, bigEndian); pos += 4;
    uint32_t endFrame = ReadU32(buffer + pos, bigEndian); pos += 4;

    sprintf(debugMsg, "[ParseAnimationAtlas] FrameRate=%d, Loop=%d, PingPong=%d, Speed=%.2f, Frames=%d-%d\n",
            frameRate, loop, pingPong, speed, startFrame, endFrame);
    OutputDebugStringA(debugMsg);

    // 3. Frame Count
    if (pos + 4 > bufferSize) {
        OutputDebugStringA("[ParseAnimationAtlas] Error reading frame count\n");
        return false;
    }
    uint32_t spriteCount = ReadU32(buffer + pos, bigEndian); pos += 4;
    if (spriteCount == 0 || spriteCount > kMaxSpriteCount) {
        OutputDebugStringA("[ParseAnimationAtlas] Invalid frame count\n");
        return false;
    }
    sprintf(debugMsg, "[ParseAnimationAtlas] Frame count: %d\n", spriteCount);
    OutputDebugStringA(debugMsg);

    // Create animation object
    SpriteAnimation anim;
    anim.name = animName;
    anim.frameRate = frameRate;
    anim.loop = loop;
    anim.pingPong = pingPong;
    anim.speedMultiplier = speed;
    anim.startFrame = startFrame;
    anim.endFrame = endFrame;
    anim.spriteIndices.reserve(spriteCount);

    // 4. Parse sprite frames
    for (uint32_t i = 0; i < spriteCount; i++) {
        if (pos + 20 > bufferSize) {
            sprintf(debugMsg, "[ParseAnimationAtlas] Error reading frame %d\n", i);
            OutputDebugStringA(debugMsg);
            return false;
        }

        uint32_t x = ReadU32(buffer + pos, bigEndian); pos += 4;
        uint32_t y = ReadU32(buffer + pos, bigEndian); pos += 4;
        uint32_t w = ReadU32(buffer + pos, bigEndian); pos += 4;
        uint32_t h = ReadU32(buffer + pos, bigEndian); pos += 4;

        uint16_t px = ReadU16BE(buffer + pos); pos += 2;
        uint16_t py = ReadU16BE(buffer + pos); pos += 2;

        SpriteRegion reg;
        char frameBuf[16];
        sprintf(frameBuf, "_frame%u", i);
        reg.name = animName + frameBuf;
        reg.width = w;
        reg.height = h;

        if (px == 0xFFFF) px = w / 2;
        if (py == 0xFFFF) py = h / 2;
        reg.pivotX = (float)px;
        reg.pivotY = (float)py;

        if (version >= 2) {
            if (pos + 16 > bufferSize) {
                sprintf(debugMsg, "[ParseAnimationAtlas] Error reading UV for frame %d\n", i);
                OutputDebugStringA(debugMsg);
                return false;
            }
            reg.u0 = ReadF32(buffer + pos, bigEndian); pos += 4;
            reg.v0 = ReadF32(buffer + pos, bigEndian); pos += 4;
            reg.u1 = ReadF32(buffer + pos, bigEndian); pos += 4;
            reg.v1 = ReadF32(buffer + pos, bigEndian); pos += 4;
        } else {
            // Default UV for version 1
            reg.u0 = 0.0f; reg.v0 = 0.0f;
            reg.u1 = 1.0f; reg.v1 = 1.0f;
        }

        reg.flipX = false;
        reg.flipY = false;
        reg.collWidth = 1;
        reg.collHeight = 1;
        reg.blocksMovement = true;
        reg.isTrigger = false;
        reg.isBuilding = false;
        reg.nodeWeightEntries.clear(); // Default: no entries

        // Add region to atlas
        uint32_t regionIndex = atlas->GetRegionCount();
        atlas->AddRegion(reg);
        anim.spriteIndices.push_back(regionIndex);
    }

    // Store animation metadata
    atlas->AddAnimation(anim);

    sprintf(debugMsg, "[ParseAnimationAtlas] Successfully processed %d animation frames\n", spriteCount);
    OutputDebugStringA(debugMsg);
    return true;
}
