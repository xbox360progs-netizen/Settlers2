#include "stdafx.h"
#include "MapSerializer.h"
#include "Map.h"
#include "TileLayer.h"
#include <vector>
#include <cstring>

#pragma pack(push, 1)
struct Header {
    char magic[4];   // "SMAP"
    int version;     // 2 (added road flags support)
    int groundW, groundH;
    int otherW, otherH;
};
#pragma pack(pop)

static const int CURRENT_VERSION = 2;

// Вспомогательные функции для буферизации
static void Append(std::vector<BYTE>& buf, const void* data, size_t size) {
    size_t oldSize = buf.size();
    buf.resize(oldSize + size);
    memcpy(buf.data() + oldSize, data, size);
}

// Потоковое чтение из буфера
struct BufferReader {
    const BYTE* data;
    size_t size;
    size_t pos;
    BufferReader(const std::vector<BYTE>& buf) : data(buf.data()), size(buf.size()), pos(0) {}
    bool Read(void* dest, size_t sz) {
        if (pos + sz > size) return false;
        memcpy(dest, data + pos, sz);
        pos += sz;
        return true;
    }
};

bool MapSerializer::Save(const World::Map& map, const std::string& path, const std::vector<std::pair<int,int>>* flags)
{
    std::vector<BYTE> buffer;
    buffer.reserve(1024 * 1024); // Резерв 1МБ

    int groundW = map.GetWidth();
    int groundH = map.GetHeight();
    int otherW = groundW * 2;
    int otherH = groundH * 4;

    Header hdr;
    memcpy(hdr.magic, "SMAP", 4);
    hdr.version = CURRENT_VERSION;
    hdr.groundW = groundW;
    hdr.groundH = groundH;
    hdr.otherW = otherW;
    hdr.otherH = otherH;
    Append(buffer, &hdr, sizeof(hdr));

    for (int li = 0; li < World::LayerCount; ++li) {
        World::LayerType lt = static_cast<World::LayerType>(li);
        const World::TileLayer* layer = map.GetLayer(lt);
        if (!layer) {
            int zero = 0;
            Append(buffer, &zero, sizeof(zero));
            Append(buffer, &zero, sizeof(zero));
            Append(buffer, &zero, sizeof(zero));
            continue;
        }
        int ltInt = static_cast<int>(lt);
        int w = layer->GetWidth();
        int h = layer->GetHeight();
        Append(buffer, &ltInt, sizeof(ltInt));
        Append(buffer, &w, sizeof(w));
        Append(buffer, &h, sizeof(h));

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const World::Tile& tile = layer->GetTile(x, y);
                int type = static_cast<int>(tile.type);
                Append(buffer, &type, sizeof(type));
                Append(buffer, &tile.x, sizeof(tile.x));
                Append(buffer, &tile.y, sizeof(tile.y));
                Append(buffer, &tile.height, sizeof(tile.height));
                Append(buffer, &tile.uvOffset, sizeof(tile.uvOffset));
                BYTE wb = tile.walkable ? 1 : 0;
                BYTE bb = tile.buildable ? 1 : 0;
                Append(buffer, &wb, 1);
                Append(buffer, &bb, 1);
                Append(buffer, &tile.regionIndex, sizeof(tile.regionIndex));
                Append(buffer, &tile.u0, sizeof(tile.u0));
                Append(buffer, &tile.v0, sizeof(tile.v0));
                Append(buffer, &tile.u1, sizeof(tile.u1));
                Append(buffer, &tile.v1, sizeof(tile.v1));
                int nameLen = static_cast<int>(tile.atlasName.length());
                Append(buffer, &nameLen, sizeof(nameLen));
                if (nameLen > 0)
                    Append(buffer, tile.atlasName.c_str(), nameLen);
            }
        }
    }

    int rcount = otherW * otherH;
    Append(buffer, &rcount, sizeof(rcount));
    for (int y = 0; y < otherH; ++y) {
        for (int x = 0; x < otherW; ++x) {
            const World::ResourceNode& rn = map.GetResourceNode(x, y);
            Append(buffer, &rn.weight, sizeof(rn.weight));
            int rt = static_cast<int>(rn.type);
            Append(buffer, &rt, sizeof(rt));
            Append(buffer, &rn.amount, sizeof(rn.amount));
            BYTE vis = rn.isVisible ? 1 : 0;
            Append(buffer, &vis, 1);
        }
    }

    // Road flags
    if (flags) {
        int fcount = (int)flags->size();
        Append(buffer, &fcount, sizeof(fcount));
        for (size_t i = 0; i < flags->size(); ++i) {
            Append(buffer, &(*flags)[i].first, sizeof(int));
            Append(buffer, &(*flags)[i].second, sizeof(int));
        }
    } else {
        int fcount = -1; // no flags section
        Append(buffer, &fcount, sizeof(fcount));
    }

    HANDLE hFile = CreateFileA(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    bool success = WriteFile(hFile, buffer.data(), (DWORD)buffer.size(), &written, NULL) && written == buffer.size();
    CloseHandle(hFile);
    return success;
}

bool MapSerializer::Load(World::Map& map, const std::string& path, std::vector<std::pair<int,int>>* flags)
{
    HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD fileSize = GetFileSize(hFile, NULL);
    std::vector<BYTE> buffer(fileSize);
    DWORD read = 0;
    ReadFile(hFile, buffer.data(), fileSize, &read, NULL);
    CloseHandle(hFile);

    BufferReader reader(buffer);
    Header hdr;
    if (!reader.Read(&hdr, sizeof(hdr))) return false;
    if (memcmp(hdr.magic, "SMAP", 4) != 0) return false;
    // Support version 1 (no flags) and version 2 (with flags)
    if (hdr.version != 1 && hdr.version != CURRENT_VERSION) return false;

    map.Clear();
    map.InitializeWeights(World::Weight_Land);
    map.ClearResources();

    for (int li = 0; li < World::LayerCount; ++li) {
        World::LayerType lt = static_cast<World::LayerType>(li);
        World::TileLayer* layer = map.GetLayer(lt);
        if (!layer) continue;
        for (int y = 0; y < layer->GetHeight(); ++y)
            for (int x = 0; x < layer->GetWidth(); ++x)
                layer->SetTile(x, y, World::Tile());
    }

    for (int li = 0; li < World::LayerCount; ++li) {
        int ltInt, fileW, fileH;
        if (!reader.Read(&ltInt, sizeof(ltInt)) || !reader.Read(&fileW, sizeof(fileW)) || !reader.Read(&fileH, sizeof(fileH))) return false;
        World::LayerType lt = static_cast<World::LayerType>(ltInt);
        World::TileLayer* layer = map.GetLayer(lt);
        if (!layer) return false;
        int readW = min(fileW, layer->GetWidth());
        int readH = min(fileH, layer->GetHeight());
        for (int y = 0; y < readH; ++y) {
            for (int x = 0; x < readW; ++x) {
                World::Tile tile;
                int type;
                reader.Read(&type, sizeof(type));
                tile.type = static_cast<World::TileType>(type);
                reader.Read(&tile.x, sizeof(tile.x));
                reader.Read(&tile.y, sizeof(tile.y));
                reader.Read(&tile.height, sizeof(tile.height));
                reader.Read(&tile.uvOffset, sizeof(tile.uvOffset));
                BYTE wb, bb;
                reader.Read(&wb, 1);
                reader.Read(&bb, 1);
                tile.walkable = (wb != 0);
                tile.buildable = (bb != 0);
                reader.Read(&tile.regionIndex, sizeof(tile.regionIndex));
                reader.Read(&tile.u0, sizeof(tile.u0));
                reader.Read(&tile.v0, sizeof(tile.v0));
                reader.Read(&tile.u1, sizeof(tile.u1));
                reader.Read(&tile.v1, sizeof(tile.v1));
                int nameLen;
                reader.Read(&nameLen, sizeof(nameLen));
                if (nameLen > 0) {
                    std::vector<char> buf(nameLen + 1, 0);
                    reader.Read(buf.data(), nameLen);
                    tile.atlasName = buf.data();
                }
                layer->SetTile(x, y, tile);
            }
        }
    }

    int fileRcount;
    if (!reader.Read(&fileRcount, sizeof(fileRcount))) return false;
    int maxRes = hdr.otherW * hdr.otherH;
    int readCount = min(fileRcount, maxRes);
    for (int i = 0; i < readCount; ++i) {
        int y = i / hdr.otherW;
        int x = i % hdr.otherW;
        BYTE weight;
        int rt, amount;
        BYTE vis;
        reader.Read(&weight, 1);
        reader.Read(&rt, sizeof(rt));
        reader.Read(&amount, sizeof(amount));
        reader.Read(&vis, 1);
        map.SetResourceNode(x, y, static_cast<World::ResourceType>(rt), amount, vis != 0);
        map.SetNodeWeight(x, y, weight);
    }

    // Road flags (version 2+)
    if (hdr.version >= 2 && flags) {
        int fcount;
        if (reader.Read(&fcount, sizeof(fcount))) {
            if (fcount >= 0) {
                flags->clear();
                for (int i = 0; i < fcount; ++i) {
                    int fx, fy;
                    if (!reader.Read(&fx, sizeof(fx)) || !reader.Read(&fy, sizeof(fy))) break;
                    flags->push_back(std::make_pair(fx, fy));
                }
            }
        }
    }
    return true;
}
