#pragma once

#include <string>
#include <vector>
#include <map>
#include <d3d9.h>
#include <d3dx9.h>

// Forward declaration for compatibility with TextureRegistry
class TextureLoader {
public:
    TextureLoader(IDirect3DDevice9* device = nullptr);
    ~TextureLoader();

	HRESULT Load(LPCWSTR filename, LPDIRECT3DTEXTURE9* texture);
#ifdef _XBOX
    HRESULT LoadAtlas(LPCWSTR filename, LPDIRECT3DTEXTURE9* texture);
#endif
    // Load a texture from disk given a wide-path, return HRESULT
    // Load textures listed in an INI file under a section; (basic parsing)
    HRESULT LoadFromIni(const wchar_t* iniPath, const wchar_t* section);
    // Enumerate loaded entries (name -> path mapping if needed)
    void GetEntries(std::vector<std::string>& entries) const;
    void SetDevice(IDirect3DDevice9* device);
    IDirect3DDevice9* GetDevice() const;
private:
    IDirect3DDevice9* m_device;
    std::map<std::string, LPDIRECT3DTEXTURE9> m_loaded;
};
