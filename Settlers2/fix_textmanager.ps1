# PowerShell script to fix TextManager.cpp
$content = Get-Content -Path "Graphics\TextManager.cpp" -Raw

# Fix variable redefinition (lines 189-192)
$content = $content -replace "    float penX = x;", "    penX = x;"
$content = $content -replace "    float penY = y;", "    penY = y;"
$content = $content -replace "    int shaderID = isUI \? SHADER_SPRITE : SHADER_WORLD;", "    shaderID = isUI ? SHADER_SPRITE : SHADER_WORLD;"

# Add baseDepth variable after shaderID line
$content = $content -replace "(    shaderID = isUI \? SHADER_SPRITE : SHADER_WORLD;)", "`$1`n    float baseDepth = depth; // Added this so C2065 disappears"

# Fix FontChar access issue (line 240)
$content = $content -replace "glyph = chars\[c\]\.glyph", "glyph = chars[c]"

# Write back to file
$content | Out-File -FilePath "Graphics\TextManager.cpp" -Encoding UTF8
Write-Host "TextManager.cpp has been fixed"
