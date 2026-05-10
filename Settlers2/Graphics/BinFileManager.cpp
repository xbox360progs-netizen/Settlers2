// BinFileManager.cpp
#include "stdafx.h"
#include "BinFileManager.h"
#include "SpriteAtlas.h"
#include "Texture.h"
#include "TextureLoader.h"
#include <fstream>
#include <xtl.h>
#include <cstring>

// Вспомогательные функции для чтения с конвертацией endianness
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
// Универсальные чтения с учётом выбранного порядка байт
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
        return (v == 1 || v == 2 || v == 3 || v == 4 || v == 5 || v == 6) && h == 24;
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
    // Проверяем, загружен ли уже такой атлас
    if (HasAtlas(name)) {
        return GetAtlas(name);
    }; 
    
    // Конвертируем путь в wide string для WinAPI
    std::wstring wBinPath(binFilePath.begin(), binFilePath.end());
    char pathA[512];
    WideCharToMultiByte(CP_ACP, 0, wBinPath.c_str(), -1, pathA, 512, NULL, NULL);
    
    // Открываем .bin файл
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
        return NULL;
    }
    
    // Получаем размер файла
    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == 0 || fileSize > kMaxFileSize) { // Защита от огромных файлов
        CloseHandle(hFile);
        return NULL;
    }
    
    // Читаем файл в память
    BYTE* buffer = new BYTE[fileSize];
    DWORD bytesRead = 0;
    if (!ReadFile(hFile, buffer, fileSize, &bytesRead, NULL) || bytesRead != fileSize) {
        delete[] buffer;
        CloseHandle(hFile);
        return NULL;
    }
    
    CloseHandle(hFile);
    
    // Создаем атлас
    AtlasPtr atlas(new SpriteAtlas(name));
    
    // Парсим данные из .bin файла
    if (ParseBinFile(buffer, fileSize, atlas.get())) {
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
        if (FAILED(loader.Load(wPngPath.c_str(), &pD3DTex))) {
            OutputDebugStringA("[BinFileManager] Failed to load PNG texture for atlas\n");
            delete[] buffer;
            return NULL;
        }
        
        // Set texture to atlas
        atlas->SetTexture(pD3DTex);
        
        // Release local reference since atlas owns the texture now
        pD3DTex->Release();
        
        m_loadedAtlases[name] = atlas;
        delete[] buffer;
        return atlas;
    }
    
    delete[] buffer;
//    delete atlas;
    return NULL;
}

bool BinFileManager::ParseBinFile(BYTE* buffer, DWORD bufferSize, SpriteAtlas* atlas)
{
    if (bufferSize < 5) {
        return false;
    }

    // Читаем первый байт типа
    uint8_t atlasType = buffer[4]; // 5-й байт

    char debugHeader[256];
    sprintf(debugHeader, "[ParseBinFile] Raw type: %d\n", atlasType);
    OutputDebugStringA(debugHeader);

    // Поддерживаем MultiLevelAtlas (type 1) и AnimationAtlas (type 2)
    if (atlasType == 1) {
        // Версия и headerSize по 2 байта
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
            if ((versionBE == 3 || versionBE == 4 || versionBE == 5 || versionBE == 6) && headerSizeBE == 24) {
                bigEndian = true;
                OutputDebugStringA("[ParseBinFile] Принудительно выбран BigEndian для MultiLevel\n");
            } else {
                OutputDebugStringA("[ParseBinFile] Неверный заголовок MultiLevel файла!\n");
                return false;
            }
        }

        uint16_t version    = bigEndian ? versionBE    : versionLE;
        uint16_t headerSize = bigEndian ? headerSizeBE : headerSizeLE;

        sprintf(debugHeader, "[ParseBinFile] Определена версия MultiLevel: %d, endian: %s\n",
                version, bigEndian ? "big" : "little");
        OutputDebugStringA(debugHeader);

        OutputDebugStringA("[ParseBinFile] Начинаем парсинг MultiLevel\n");
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
            OutputDebugStringA("[ParseBinFile] Принудительно выбран BigEndian для AnimationAtlas\n");
        }

        uint16_t version = bigEndian ? versionBE : versionLE;

        sprintf(debugHeader, "[ParseBinFile] Определена версия AnimationAtlas: %d, endian: %s\n",
                version, bigEndian ? "big" : "little");
        OutputDebugStringA(debugHeader);

        OutputDebugStringA("[ParseBinFile] Начинаем парсинг AnimationAtlas\n");
        return ParseAnimationAtlas(buffer, bufferSize, atlas, version, bigEndian);
    }
    else {
        char unknownType[128];
        sprintf(unknownType, "[ParseBinFile] Неизвестный тип атласа: %d (поддерживаются MultiLevelAtlas type 1 и AnimationAtlas type 2)\n", atlasType);
        OutputDebugStringA(unknownType);
        return false;
    }
}

bool BinFileManager::ParseMultiLevelAtlas(BYTE* buffer, DWORD bufferSize, SpriteAtlas* atlas, uint16_t version, bool bigEndian)
{
    OutputDebugStringA("[ParseMultiLevelAtlas] Начало парсинга\n");
    
    uint32_t nameLen = ReadU32(buffer + 5, bigEndian);
    uint32_t nameStart = 9;
    if (nameLen == 0 || nameLen > 256 || nameStart + nameLen > bufferSize) {
        OutputDebugStringA("[ParseMultiLevelAtlas] Ошибка чтения имени атласа\n");
        return false;
    }

    DWORD pos = nameStart + nameLen;
    std::string atlasName(reinterpret_cast<char const*>(buffer + nameStart), nameLen);
    char debugMsg[256];
    sprintf(debugMsg, "[ParseMultiLevelAtlas] Имя атласа: %s\n", atlasName.c_str());
    OutputDebugStringA(debugMsg);

    uint32_t prefixLen = ReadU32(buffer + pos, bigEndian); pos += 4;
    if (prefixLen == 0 || prefixLen > 256 || pos + prefixLen > bufferSize) {
        OutputDebugStringA("[ParseMultiLevelAtlas] Ошибка чтения префикса\n");
        return false;
    }
    std::string prefix(reinterpret_cast<char const*>(buffer + pos), prefixLen);
    pos += prefixLen;
    sprintf(debugMsg, "[ParseMultiLevelAtlas] Префикс: %s\n", prefix.c_str());
    OutputDebugStringA(debugMsg);

    uint32_t spritesPerLevel = ReadU32(buffer + pos, bigEndian); pos += 4;
    bool autoArrange = (buffer[pos] != 0); pos += 1;
    sprintf(debugMsg, "[ParseMultiLevelAtlas] SpritesPerLevel: %d, AutoArrange: %s\n",
            spritesPerLevel, autoArrange ? "true" : "false");
    OutputDebugStringA(debugMsg);

    uint32_t groupCount = ReadU32(buffer + pos, bigEndian); pos += 4;
    sprintf(debugMsg, "[ParseMultiLevelAtlas] Количество групп: %d\n", groupCount);
    OutputDebugStringA(debugMsg);
    
    if (groupCount > 512) {
        OutputDebugStringA("[ParseMultiLevelAtlas] Слишком много групп\n");
        return false;
    }
    
    // Читаем группы и собираем имена
    std::vector<std::string> groupNames;
    std::vector<std::vector<uint32_t>> groupSpriteIndices;
    
    for (uint32_t i = 0; i < groupCount; i++) {
        if (pos + 4 > bufferSize) {
            sprintf(debugMsg, "[ParseMultiLevelAtlas] Ошибка в группе %d: недостаточно данных\n", i);
            OutputDebugStringA(debugMsg);
            return false;
        }
        uint32_t groupNameLen = ReadU32(buffer + pos, bigEndian); pos += 4;
        if (groupNameLen > 256 || pos + groupNameLen > bufferSize) {
            sprintf(debugMsg, "[ParseMultiLevelAtlas] Ошибка в группе %d: неверная длина имени\n", i);
            OutputDebugStringA(debugMsg);
            return false;
        }
        
        std::string groupName(reinterpret_cast<char const*>(buffer + pos), groupNameLen);
        pos += groupNameLen;
        groupNames.push_back(groupName);
        
        if (pos + 4 > bufferSize) {
            sprintf(debugMsg, "[ParseMultiLevelAtlas] Ошибка в группе %d: нет данных о спрайтах\n", i);
            OutputDebugStringA(debugMsg);
            return false;
        }
        uint32_t spriteCount = ReadU32(buffer + pos, bigEndian); pos += 4;
        if (spriteCount > kMaxSpriteCount) {
            sprintf(debugMsg, "[ParseMultiLevelAtlas] Ошибка в группе %d: слишком много спрайтов\n", i);
            OutputDebugStringA(debugMsg);
            return false;
        }
        
        std::vector<uint32_t> spriteIndices;
        for (uint32_t j = 0; j < spriteCount; j++) {
            if (pos + 4 > bufferSize) {
                sprintf(debugMsg, "[ParseMultiLevelAtlas] Ошибка в группе %d, спрайт %d: недостаточно данных\n", i, j);
                OutputDebugStringA(debugMsg);
                return false;
            }
            uint32_t spriteIndex = ReadU32(buffer + pos, bigEndian); pos += 4;
            spriteIndices.push_back(spriteIndex);
        }
        groupSpriteIndices.push_back(spriteIndices);
        
        sprintf(debugMsg, "[ParseMultiLevelAtlas] Группа %d: %s (%d спрайтов)\n", i, groupName.c_str(), spriteCount);
        OutputDebugStringA(debugMsg);
    }

    if (pos + 4 > bufferSize) {
        OutputDebugStringA("[ParseMultiLevelAtlas] Ошибка чтения общего количества спрайтов\n");
        return false;
    }
    uint32_t totalSpriteCount = ReadU32(buffer + pos, bigEndian); pos += 4;
    sprintf(debugMsg, "[ParseMultiLevelAtlas] Всего спрайтов: %d\n", totalSpriteCount);
    OutputDebugStringA(debugMsg);
    
    if (totalSpriteCount == 0 || totalSpriteCount > kMaxSpriteCount) {
        OutputDebugStringA("[ParseMultiLevelAtlas] Неверное количество спрайтов\n");
        return false;
    }

    // Теперь читаем сами спрайты и создаем SpriteRegion
    for (uint32_t spriteIdx = 0; spriteIdx < totalSpriteCount; spriteIdx++) {
        if (pos + 20 > bufferSize) {
            sprintf(debugMsg, "[ParseMultiLevelAtlas] Ошибка чтения спрайта %d: недостаточно данных\n", spriteIdx);
            OutputDebugStringA(debugMsg);
            return false;
        }

        uint32_t x = ReadU32(buffer + pos, bigEndian); pos += 4;
        uint32_t y = ReadU32(buffer + pos, bigEndian); pos += 4;
        uint32_t width = ReadU32(buffer + pos, bigEndian); pos += 4;
        uint32_t height = ReadU32(buffer + pos, bigEndian); pos += 4;

        // Пивоты в BigEndian (как в C# коде)
        uint16_t pivotX = ReadU16BE(buffer + pos); pos += 2;
        uint16_t pivotY = ReadU16BE(buffer + pos); pos += 2;

        float uv_min_x = 0.0f, uv_min_y = 0.0f;
        float uv_max_x = 1.0f, uv_max_y = 1.0f;
        if (version >= 2) {
            if (pos + 16 > bufferSize) {
                sprintf(debugMsg, "[ParseMultiLevelAtlas] Ошибка чтения UV спрайта %d\n", spriteIdx);
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
                sprintf(debugMsg, "[ParseMultiLevelAtlas] Ошибка чтения IsPacked спрайта %d\n", spriteIdx);
                OutputDebugStringA(debugMsg);
                return false;
            }
            isPacked = (buffer[pos] != 0); pos += 1;
            if (isPacked) {
                if (pos + 8 > bufferSize) {
                    sprintf(debugMsg, "[ParseMultiLevelAtlas] Ошибка чтения BlockOffset спрайта %d\n", spriteIdx);
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
                sprintf(debugMsg, "[ParseMultiLevelAtlas] Ошибка чтения коллайдера спрайта %d\n", spriteIdx);
                OutputDebugStringA(debugMsg);
                return false;
            }
            collWidth = ReadU32(buffer + pos, bigEndian); pos += 4;
            collHeight = ReadU32(buffer + pos, bigEndian); pos += 4;
            blocksMovement = (buffer[pos] != 0); pos += 1;
            isTrigger = (buffer[pos] != 0); pos += 1;
        }

        // Новые поля из version >= 6
        std::string spriteName;
        bool flipX = false, flipY = false;
        if (version >= 6) {
            if (pos + 4 > bufferSize) {
                sprintf(debugMsg, "[ParseMultiLevelAtlas] Ошибка чтения имени спрайта %d\n", spriteIdx);
                OutputDebugStringA(debugMsg);
                return false;
            }
            uint32_t nameLenSprite = ReadU32(buffer + pos, bigEndian); pos += 4;
            if (nameLenSprite > 0) {
                if (pos + nameLenSprite > bufferSize) {
                    sprintf(debugMsg, "[ParseMultiLevelAtlas] Ошибка чтения имени спрайта %d: недостаточно данных\n", spriteIdx);
                    OutputDebugStringA(debugMsg);
                    return false;
                }
                spriteName = std::string(reinterpret_cast<char const*>(buffer + pos), nameLenSprite);
                pos += nameLenSprite;
            }

            if (pos + 2 > bufferSize) {
                sprintf(debugMsg, "[ParseMultiLevelAtlas] Ошибка чтения трансформаций спрайта %d\n", spriteIdx);
                OutputDebugStringA(debugMsg);
                return false;
            }
            flipX = (buffer[pos] != 0); pos += 1;
            flipY = (buffer[pos] != 0); pos += 1;
        }

        if (width == 0 || height == 0 || width > kMaxFrameDim || height > kMaxFrameDim) {
            sprintf(debugMsg, "[ParseMultiLevelAtlas] Пропущен спрайт %d: неверные размеры\n", spriteIdx);
            OutputDebugStringA(debugMsg);
            continue;
        }

        if (pivotX == 0xFFFF) pivotX = width / 2;
        if (pivotY == 0xFFFF) pivotY = height / 2;

        sprintf(debugMsg, "[ParseMultiLevelAtlas] Спрайт %d: %dx%d, pivot=(%d,%d), UV=(%.3f,%.3f)-(%.3f,%.3f)\n",
                spriteIdx, width, height, pivotX, pivotY, uv_min_x, uv_min_y, uv_max_x, uv_max_y);
        OutputDebugStringA(debugMsg);

        // Create SpriteRegion for Xbox 360 rendering
        SpriteRegion reg;
        reg.name = spriteName;
        reg.width = width;
        reg.height = height;

        // UVs are pre-calculated in C# tool, assign them directly
        reg.u0 = uv_min_x;
        reg.v0 = uv_min_y;
        reg.u1 = uv_max_x;
        reg.v1 = uv_max_y;

        // Convert pivot 0xFFFF to default (0,0)
        if (pivotX == 0xFFFF || pivotY == 0xFFFF) {
            reg.pivotX = 0.0f;
            reg.pivotY = 0.0f;
        } else {
            reg.pivotX = (float)pivotX;
            reg.pivotY = (float)pivotY;
        }

        reg.flipX = flipX;
        reg.flipY = flipY;

        // Add to atlas storage
        atlas->AddRegion(reg);
    }

    sprintf(debugMsg, "[ParseMultiLevelAtlas] Успешно обработано %d спрайтов\n", totalSpriteCount);
    OutputDebugStringA(debugMsg);
    OutputDebugStringA("[ParseMultiLevelAtlas] Парсинг завершен успешно\n");
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

    // 1. Проверяем кэш
    if (HasAtlas(name)) {
        return GetAtlas(name);
    }

    // 2. Используем новый TextureLoader вместо pTex->Load
    TextureLoader loader(pDevice);
    LPDIRECT3DTEXTURE9 pD3DTex = nullptr;

    // Конвертируем путь в WideString для лоадера
    std::string pathStr(filePath);
    std::wstring wPath(pathStr.begin(), pathStr.end());

    // Загружаем через наш исправленный метод (из памяти, с поддержкой game:\)
    HRESULT hr = loader.Load(wPath.c_str(), &pD3DTex);

    if (FAILED(hr) || !pD3DTex) {
        sprintf(debugMsg, "[BinFileManager] Failed to load texture: %s, hr=0x%08X\n", filePath, hr);
        OutputDebugStringA(debugMsg);
        return NULL;
    }

    // 3. Получаем размеры текстуры
    D3DSURFACE_DESC desc;
    pD3DTex->GetLevelDesc(0, &desc);

    // 4. Создаем атлас
    AtlasPtr atlas(new SpriteAtlas(name));
    atlas->SetTexture(pD3DTex); // SpriteAtlas должен сделать AddRef() внутри

    // 5. Создаем регион на всю текстуру
    SpriteRegion region;
    region.name = "default";
    region.width = (float)desc.Width;
    region.height = (float)desc.Height;
    region.u0 = 0.0f; region.v0 = 0.0f;
    region.u1 = 1.0f; region.v1 = 1.0f;
    region.pivotX = 0.0f; region.pivotY = 0.0f;
    region.flipX = false; region.flipY = false;

    atlas->AddRegion(region);

    // 6. Регистрируем и чистим за собой
    m_loadedAtlases[name] = atlas;
    
    // Освобождаем локальную ссылку, так как атлас теперь владеет текстурой
    pD3DTex->Release();

    sprintf(debugMsg, "[BinFileManager] Atlas created: %s (%ux%u)\n", name, desc.Width, desc.Height);
    OutputDebugStringA(debugMsg);

    return atlas;
}

bool BinFileManager::ParseAnimationAtlas(BYTE* buffer, DWORD bufferSize, SpriteAtlas* atlas, uint16_t version, bool bigEndian)
{
    OutputDebugStringA("[ParseAnimationAtlas] Начало парсинга\n");

    DWORD pos = 5; // Type byte was at index 4

    // 1. Animation Name
    if (pos + 4 > bufferSize) {
        OutputDebugStringA("[ParseAnimationAtlas] Ошибка чтения имени анимации\n");
        return false;
    }
    uint32_t nameLen = ReadU32(buffer + pos, bigEndian); pos += 4;
    if (nameLen == 0 || nameLen > 256 || pos + nameLen > bufferSize) {
        OutputDebugStringA("[ParseAnimationAtlas] Неверная длина имени анимации\n");
        return false;
    }
    std::string animName(reinterpret_cast<char const*>(buffer + pos), nameLen); pos += nameLen;
    char debugMsg[256];
    sprintf(debugMsg, "[ParseAnimationAtlas] Имя анимации: %s\n", animName.c_str());
    OutputDebugStringA(debugMsg);

    // 2. Playback Metadata
    if (pos + 20 > bufferSize) {
        OutputDebugStringA("[ParseAnimationAtlas] Ошибка чтения метаданных воспроизведения\n");
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
        OutputDebugStringA("[ParseAnimationAtlas] Ошибка чтения количества кадров\n");
        return false;
    }
    uint32_t spriteCount = ReadU32(buffer + pos, bigEndian); pos += 4;
    if (spriteCount == 0 || spriteCount > kMaxSpriteCount) {
        OutputDebugStringA("[ParseAnimationAtlas] Неверное количество кадров\n");
        return false;
    }
    sprintf(debugMsg, "[ParseAnimationAtlas] Количество кадров: %d\n", spriteCount);
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
            sprintf(debugMsg, "[ParseAnimationAtlas] Ошибка чтения кадра %d\n", i);
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

        if (px == 0xFFFF || py == 0xFFFF) {
            reg.pivotX = 0.0f;
            reg.pivotY = 0.0f;
        } else {
            reg.pivotX = (float)px;
            reg.pivotY = (float)py;
        }

        if (version >= 2) {
            if (pos + 16 > bufferSize) {
                sprintf(debugMsg, "[ParseAnimationAtlas] Ошибка чтения UV кадра %d\n", i);
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

        // Add region to atlas
        uint32_t regionIndex = atlas->GetRegionCount();
        atlas->AddRegion(reg);
        anim.spriteIndices.push_back(regionIndex);
    }

    // Store animation metadata
    atlas->AddAnimation(anim);

    sprintf(debugMsg, "[ParseAnimationAtlas] Успешно обработано %d кадров анимации\n", spriteCount);
    OutputDebugStringA(debugMsg);
    return true;
}
