#include "stdafx.h"
#include "MapSerializer.h"
#include "Map.h"
#include "TileLayer.h"
#include "Components/Building.h"
#include <vector>
#include <cstring>

#pragma pack(push, 1)
struct Header {
    char magic[4];   // "SMAP"
    int version;     // 3 (added Buildings layer)
    int groundW, groundH;
    int otherW, otherH;
};
#pragma pack(pop)

static const int CURRENT_VERSION = 7;  // v7 adds ResourceNode.surveyed

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
            BYTE surv = rn.surveyed ? 1 : 0;
            Append(buffer, &surv, 1);
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
    // Support versions 1-3
    if (hdr.version < 1 || hdr.version > CURRENT_VERSION) return false;

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

    int layersToRead = (hdr.version < 3) ? 7 : World::LayerCount;
    for (int li = 0; li < layersToRead; ++li) {
        int ltInt, fileW, fileH;
        if (!reader.Read(&ltInt, sizeof(ltInt)) || !reader.Read(&fileW, sizeof(fileW)) || !reader.Read(&fileH, sizeof(fileH))) return false;
        World::LayerType lt = static_cast<World::LayerType>(ltInt);
        World::TileLayer* layer = map.GetLayer(lt);
        if (!layer) return false;
        int readW = min(fileW, layer->GetWidth());
        int readH = min(fileH, layer->GetHeight());
        for (int y = 0; y < fileH; ++y) {
            for (int x = 0; x < fileW; ++x) {
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
                if (x < readW && y < readH) {
                    layer->SetTile(x, y, tile);
                }
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

bool MapSerializer::SaveV4(const World::Map& map, const std::string& path, const std::vector<World::FlagData>* flags, const std::vector<World::RoadData>* roads)
{
    std::vector<BYTE> buffer;
    buffer.reserve(1024 * 1024);

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
            BYTE surv = rn.surveyed ? 1 : 0;
            Append(buffer, &surv, 1);
        }
    }

    // V4 flag format: count + array of { x, y, id, type, pendingBuilding, hasBuilding }
    if (flags) {
        int fcount = (int)flags->size();
        Append(buffer, &fcount, sizeof(fcount));
        for (size_t i = 0; i < flags->size(); ++i) {
            const World::FlagData& fd = (*flags)[i];
            Append(buffer, &fd.x, sizeof(fd.x));
            Append(buffer, &fd.y, sizeof(fd.y));
            Append(buffer, &fd.id, sizeof(fd.id));
            int ft = static_cast<int>(fd.type);
            Append(buffer, &ft, sizeof(ft));
            int pb = static_cast<int>(fd.pendingBuilding);
            Append(buffer, &pb, sizeof(pb));
            BYTE hb = fd.hasBuilding ? 1 : 0;
            Append(buffer, &hb, 1);
        }
    } else {
        int fcount = -1;
        Append(buffer, &fcount, sizeof(fcount));
    }

    // Road data: count + array of { id, startFlagId, endFlagId, tileCount, tiles[] }
    if (roads) {
        int rcount = (int)roads->size();
        Append(buffer, &rcount, sizeof(rcount));
        for (size_t i = 0; i < roads->size(); ++i) {
            const World::RoadData& rd = (*roads)[i];
            Append(buffer, &rd.id, sizeof(rd.id));
            Append(buffer, &rd.flagAId, sizeof(rd.flagAId));
            Append(buffer, &rd.flagBId, sizeof(rd.flagBId));
            int tc = (int)rd.tileCount;
            Append(buffer, &tc, sizeof(tc));
            for (int ti = 0; ti < tc; ++ti) {
                Append(buffer, &rd.tiles[ti].x, sizeof(rd.tiles[ti].x));
                Append(buffer, &rd.tiles[ti].y, sizeof(rd.tiles[ti].y));
            }
        }
    } else {
        int rcount = -1;
        Append(buffer, &rcount, sizeof(rcount));
    }

    // Habitat data (version 6+)
    const World::HabitatRegistry& hr = map.GetHabitatRegistry();
    int hcount = (int)hr.GetCount();
    Append(buffer, &hcount, sizeof(hcount));
    for (size_t hi = 0; hi < hr.GetCount(); ++hi) {
        const World::AnimalHabitat* hab = hr.GetByIndex(hi);
        if (!hab) continue;
        Append(buffer, &hab->id, sizeof(hab->id));
        Append(buffer, &hab->center.x, sizeof(hab->center.x));
        Append(buffer, &hab->center.y, sizeof(hab->center.y));
        Append(buffer, &hab->radius, sizeof(hab->radius));
        int ht = static_cast<int>(hab->type);
        Append(buffer, &ht, sizeof(ht));
        Append(buffer, &hab->maxAnimals, sizeof(hab->maxAnimals));
    }

    HANDLE hFile = CreateFileA(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    bool success = WriteFile(hFile, buffer.data(), (DWORD)buffer.size(), &written, NULL) && written == buffer.size();
    CloseHandle(hFile);
    return success;
}

bool MapSerializer::LoadV4(World::Map& map, const std::string& path, std::vector<World::FlagData>* flags, std::vector<World::RoadData>* roads)
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
    if (hdr.version < 1 || hdr.version > CURRENT_VERSION) return false;

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

    int layersToRead = (hdr.version < 3) ? 7 : World::LayerCount;
    for (int li = 0; li < layersToRead; ++li) {
        int ltInt, fileW, fileH;
        if (!reader.Read(&ltInt, sizeof(ltInt)) || !reader.Read(&fileW, sizeof(fileW)) || !reader.Read(&fileH, sizeof(fileH))) return false;
        World::LayerType lt = static_cast<World::LayerType>(ltInt);
        World::TileLayer* layer = map.GetLayer(lt);
        if (!layer) return false;
        int readW = min(fileW, layer->GetWidth());
        int readH = min(fileH, layer->GetHeight());
        for (int y = 0; y < fileH; ++y) {
            for (int x = 0; x < fileW; ++x) {
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
                if (x < readW && y < readH) {
                    layer->SetTile(x, y, tile);
                }
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
        if (hdr.version >= 7) {
            BYTE surv;
            reader.Read(&surv, 1);
            map.GetResourceNode(x, y).surveyed = (surv != 0);
        }
    }

    // V4 flags (neighbor graph reconstructed from road tiles on load;
    // old save files may have neighbor data that we skip)
    if (flags) {
        int fcount;
        if (reader.Read(&fcount, sizeof(fcount))) {
            if (fcount >= 0) {
                flags->clear();
                for (int i = 0; i < fcount; ++i) {
                    World::FlagData fd;
                    memset(&fd, 0, sizeof(fd));
                    reader.Read(&fd.x, sizeof(fd.x));
                    reader.Read(&fd.y, sizeof(fd.y));
                    reader.Read(&fd.id, sizeof(fd.id));
                    int ft;
                    reader.Read(&ft, sizeof(ft));
                    fd.type = static_cast<World::FlagType>(ft);
                    int pb;
                    reader.Read(&pb, sizeof(pb));
                    fd.pendingBuilding = static_cast<World::BuildingType>(pb);
                    BYTE hb;
                    reader.Read(&hb, 1);
                    fd.hasBuilding = (hb != 0);
                    // Skip legacy neighbor data if present (backward compat)
                    int nc;
                    if (reader.Read(&nc, sizeof(nc))) {
                        for (int ni = 0; ni < nc; ++ni) {
                            uint32_t skip;
                            reader.Read(&skip, sizeof(skip));
                        }
                    }
                    flags->push_back(fd);
                }
            }
        }
    }

    // Road data — only in v5+ (present in saves written by new code; v4 won't have it)
    if (hdr.version >= 5 && roads) {
        int rcount;
        if (reader.Read(&rcount, sizeof(rcount)) && rcount >= 0) {
            roads->clear();
            for (int i = 0; i < rcount; ++i) {
                World::RoadData rd;
                if (!reader.Read(&rd.id, sizeof(rd.id))) break;
                if (!reader.Read(&rd.flagAId, sizeof(rd.flagAId))) break;
                if (!reader.Read(&rd.flagBId, sizeof(rd.flagBId))) break;
                int tc;
                if (!reader.Read(&tc, sizeof(tc))) break;
                rd.tileCount = (tc > 0 && tc <= MAX_ROAD_TILES) ? (uint32_t)tc : 0;
                for (uint32_t ti = 0; ti < rd.tileCount; ++ti) {
                    if (!reader.Read(&rd.tiles[ti].x, sizeof(rd.tiles[ti].x))) break;
                    if (!reader.Read(&rd.tiles[ti].y, sizeof(rd.tiles[ti].y))) break;
                }
                roads->push_back(rd);
            }
        }
    }

    // Habitat data — v6+
    if (hdr.version >= 6) {
        int hcount = 0;
        if (reader.Read(&hcount, sizeof(hcount)) && hcount > 0) {
            map.GetHabitatRegistry().Clear();
            for (int hi = 0; hi < hcount; ++hi) {
                World::AnimalHabitat hab;
                reader.Read(&hab.id, sizeof(hab.id));
                reader.Read(&hab.center.x, sizeof(hab.center.x));
                reader.Read(&hab.center.y, sizeof(hab.center.y));
                reader.Read(&hab.radius, sizeof(hab.radius));
                int ht;
                reader.Read(&ht, sizeof(ht));
                hab.type = static_cast<World::AnimalType>(ht);
                reader.Read(&hab.maxAnimals, sizeof(hab.maxAnimals));
                map.GetHabitatRegistry().Restore(hab);
            }
        }
    }

    return true;
}
