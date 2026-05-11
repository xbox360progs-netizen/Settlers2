#include "StdAfx.h"
#include "TextureLoader.h"
#include "TextureRegistry.h"
#include <stdio.h>
#ifdef _XBOX
#include <xgraphics.h>
#endif

TextureLoader::TextureLoader(IDirect3DDevice9* device)
    : m_device(device)
{
}

TextureLoader::~TextureLoader()
{
}

HRESULT TextureLoader::Load(LPCWSTR filename, LPDIRECT3DTEXTURE9* texture)
{
    if (!filename || !texture)
        return E_INVALIDARG;

    *texture = nullptr;
    
    // Convert wide string to ANSI for logging
    char pathA[512];
    WideCharToMultiByte(CP_ACP, 0, filename, -1, pathA, 512, NULL, NULL);
    
    // Используем CreateFile для всех путей (работает и для game:\)
    HANDLE hFile = CreateFileA(
        pathA,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        OutputDebugStringA("[TextureLoader] Failed to open file: ");
        OutputDebugStringA(pathA);
        OutputDebugStringA("\n");
        return E_FAIL;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE || fileSize == 0)
    {
        CloseHandle(hFile);
        return E_FAIL;
    }

    // --- читаем файл в память ---
    BYTE* buffer = (BYTE*)malloc(fileSize);
    if (!buffer)
    {
        CloseHandle(hFile);
        return E_OUTOFMEMORY;
    }

    DWORD bytesRead = 0;
    BOOL ok = ReadFile(hFile, buffer, fileSize, &bytesRead, NULL);
    CloseHandle(hFile);

    if (!ok || bytesRead != fileSize)
    {
        free(buffer);
        return E_FAIL;
    }

    // --- узнаём оригинальные размеры, чтобы не допадить до степени 2 ---
    D3DXIMAGE_INFO info;
    ZeroMemory(&info, sizeof(info));
    HRESULT hrInfo = D3DXGetImageInfoFromFileInMemory(buffer, fileSize, &info);
    if (FAILED(hrInfo))
    {
        char debugInfo[256];
        sprintf(debugInfo, "[TextureLoader] D3DXGetImageInfoFromFileInMemory failed hr=0x%08X for %s\n",
                (unsigned)hrInfo, pathA);
        OutputDebugStringA(debugInfo);
    }

    // --- создаём текстуру из памяти, стараясь сохранить исходные размеры ---
    // Xbox 360: Use D3DPOOL_DEFAULT with D3DUSAGE_WRITEONLY for static textures in fast video memory
    #ifdef _XBOX
    DWORD usage = D3DUSAGE_WRITEONLY;
    D3DPOOL pool = D3DPOOL_DEFAULT;
    #else
    DWORD usage = 0;
    D3DPOOL pool = D3DPOOL_MANAGED;
    #endif
    
    HRESULT hr = D3DXCreateTextureFromFileInMemoryEx(
        m_device,
        buffer,
        fileSize,
        SUCCEEDED(hrInfo) ? info.Width  : D3DX_DEFAULT_NONPOW2,
        SUCCEEDED(hrInfo) ? info.Height : D3DX_DEFAULT_NONPOW2,
        1,                       // без автогенерации мипов для 2D
        usage,
        D3DFMT_A8R8G8B8,
        pool,
        D3DX_FILTER_LINEAR,
        D3DX_FILTER_LINEAR,
        0,
        SUCCEEDED(hrInfo) ? &info : NULL,
        NULL,
        texture
    );

    // Фолбэк для устройств, которые не поддерживают NPOT (на всякий случай)
    if (FAILED(hr))
    {
        hr = D3DXCreateTextureFromFileInMemory(
            m_device,
            buffer,
            fileSize,
            texture
        );
    }

    free(buffer);

    if (FAILED(hr))
    {
        char debugLoad[256];
        sprintf(debugLoad, "[TextureLoader] Texture create failed hr=0x%08X for %s\n",
                (unsigned)hr, pathA);
        OutputDebugStringA(debugLoad);
    }
    else if (SUCCEEDED(hrInfo))
    {
        char debugOk[256];
        sprintf(debugOk, "[TextureLoader] Loaded %s size=%ux%u\n",
                pathA, (unsigned)info.Width, (unsigned)info.Height);
        OutputDebugStringA(debugOk);
    }

    return hr;
}


HRESULT TextureLoader::LoadFromIni(const wchar_t* iniPath, const wchar_t* section)
{
    if (!iniPath || !section || !m_device) return E_FAIL;
    // Minimal INI parser: [section] followed by lines key=value
    FILE* f = _wfopen(iniPath, L"r");
    if (!f) return E_FAIL;

    bool inSection = false;
    wchar_t line[1024];
    while (fgetws(line, 1024, f)) {
        // remove CR/LF
        std::wstring wline(line);
        if (!wline.empty() && (wline.back() == L'\n' || wline.back() == L'\r')) wline.pop_back();
        // strip spaces
        // handle section header
        if (!wline.empty() && wline[0] == L'[') {
            size_t end = wline.find(L']');
            if (end != std::wstring::npos) {
                std::wstring sec = wline.substr(1, end-1);
                inSection = (sec == section);
            }
            continue;
        }
        if (!inSection) continue;
        // skip comments
        if (wline.size() && (wline[0] == L'#' || wline[0] == L';')) continue;
        size_t eq = wline.find(L'=');
        if (eq == std::wstring::npos) continue;
        std::wstring keyw = wline.substr(0, eq);
        std::wstring valw = wline.substr(eq+1);
        // trim helper
        struct TrimHelper {
            static void trim(std::wstring &s) { 
                while(!s.empty() && iswspace(s.front())) s.erase(s.begin()); 
                while(!s.empty() && iswspace(s.back())) s.pop_back(); 
            }
        };
        TrimHelper::trim(keyw); TrimHelper::trim(valw);
        if (keyw.empty() || valw.empty()) continue;
        std::string key(keyw.begin(), keyw.end());
        // Build full path with game:\ prefix
        std::wstring fullPathW = L"game:\\Media\\Textures\\" + valw;
        LPDIRECT3DTEXTURE9 tex = nullptr;
        // Use the fixed Load method instead of D3DXCreateTextureFromFileW
        HRESULT hr = this->Load(fullPathW.c_str(), &tex);
        if (SUCCEEDED(hr) && tex) {
            TextureRegistry::instance().registerTexture(key, tex);
            char debugOk[256];
            sprintf(debugOk, "[TextureLoader] IniLoad SUCCESS: %s\n", key.c_str());
            OutputDebugStringA(debugOk);
        } else {
            char debugFail[256];
            sprintf(debugFail, "[TextureLoader] IniLoad FAILED (0x%08X) for key: %S\n", (unsigned)hr, keyw.c_str());
            OutputDebugStringA(debugFail);
        }
    }
    fclose(f);
    return S_OK;
}

void TextureLoader::GetEntries(std::vector<std::string>& entries) const {
    entries.clear();
    for (std::map<std::string, LPDIRECT3DTEXTURE9>::const_iterator it = m_loaded.begin(); it != m_loaded.end(); ++it) {
        entries.push_back(it->first);
    }
}

void TextureLoader::SetDevice(IDirect3DDevice9* device)
{
    m_device = device;
}

IDirect3DDevice9* TextureLoader::GetDevice() const
{
    return m_device;
}

#ifdef _XBOX
HRESULT TextureLoader::LoadAtlas(LPCWSTR filename, LPDIRECT3DTEXTURE9* texture)
{
    HRESULT hr = this->Load(filename, texture);
    
    if (FAILED(hr)) {
        OutputDebugStringA("[TextureLoader] LoadAtlas FAILED using standard Load\n");
    } else {
        OutputDebugStringA("[TextureLoader] LoadAtlas SUCCESS\n");
    }
    
    return hr;
}
#endif // _XBOX

