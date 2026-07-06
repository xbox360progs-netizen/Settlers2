#pragma once

namespace Platform {

    // Platform-independent filesystem paths.
    //
    // On Xbox 360: GetMediaPath() returns "game:\\Media\\"
    // On Win32:    GetMediaPath() returns "Media\\"  (relative to working directory)

    // Root path for game assets (textures, shaders, configs, etc.).
    // Includes trailing separator.
    const char* GetMediaPath();

    // Root path for saved games / user data.
    const char* GetSavePath();

} // namespace Platform