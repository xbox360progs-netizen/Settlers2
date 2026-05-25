#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include <string>
#include <map>

// VS2010/Xbox 360 compatibility for uint32_t
typedef unsigned int uint32_t;

// SpriteRegion structure for storing sprite data from atlas
struct SpriteRegion {
    float u0, v0, u1, v1; // UV coordinates for shader
    uint32_t width, height;
    float pivotX, pivotY;
    bool flipX, flipY;
    std::string name;
    uint32_t collWidth;     // collider width in node-tiles (default 1)
    uint32_t collHeight;    // collider height in node-tiles (default 1)
    int collOffX;           // collider X offset from cursor tile (0 = compute from pivot)
    int collOffY;           // collider Y offset from cursor tile (0 = compute from pivot)
    bool blocksMovement;    // whether the object blocks movement (default true)
    bool isTrigger;         // whether it's a trigger (default false)
};

// SpriteAnimation structure for animation metadata
struct SpriteAnimation {
    std::string name;
    int frameRate;
    bool loop;
    bool pingPong;
    float speedMultiplier;
    uint32_t startFrame;
    uint32_t endFrame;
    std::vector<uint32_t> spriteIndices; // Indices into the SpriteAtlas regions
};

// SpriteAtlas class for managing sprite regions and texture
class SpriteAtlas {
public:
    SpriteAtlas();
    SpriteAtlas(const std::string& name);
    ~SpriteAtlas();

    // Get sprite index by name (for one-time lookup at load time)
    uint32_t GetIndex(const char* spriteName) const;

    // Get sprite region by index (for fast per-frame access)
    const SpriteRegion* GetRegion(uint32_t index) const;

    // Add a sprite region to the atlas
    void AddRegion(const SpriteRegion& region);

    // Set the atlas texture (loaded via Texture::LoadXG)
    void SetTexture(LPDIRECT3DTEXTURE9 pTexture);
    // Load atlas texture using XG for optimal performance
    void Load(IDirect3DDevice9* device, const std::string& filename);

    // Get the atlas texture
    LPDIRECT3DTEXTURE9 GetTexture() const { return m_pTexture; }

    // Get texture dimensions for UV calculation
    uint32_t GetTextureWidth() const;
    uint32_t GetTextureHeight() const;

    // Get total number of regions
    uint32_t GetRegionCount() const { return (uint32_t)m_regions.size(); }

    // Animation support
    void AddAnimation(const SpriteAnimation& anim);
    const SpriteAnimation* GetAnimation(const char* animName) const;
    uint32_t GetAnimationCount() const { return (uint32_t)m_animations.size(); }

    // Group support - groups map group name to list of sprite indices
    void AddGroup(const std::string& name, const std::vector<uint32_t>& spriteIndices);
    const std::vector<uint32_t>* GetGroup(const char* name) const;
    std::vector<std::string> GetGroupNames() const;
    uint32_t GetGroupCount() const { return (uint32_t)m_groups.size(); }

private:
    std::map<std::string, uint32_t> m_nameToIndex; // For fast lookup by name
    std::vector<SpriteRegion> m_regions;           // Sprite regions
    std::map<std::string, std::vector<uint32_t> > m_groups; // Group name → sprite indices
    std::map<std::string, SpriteAnimation> m_animations; // Animation metadata
    LPDIRECT3DTEXTURE9 m_pTexture;                // Tiled texture from XG
    uint32_t m_textureWidth;
    uint32_t m_textureHeight;
};
