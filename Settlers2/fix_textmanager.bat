@echo off
echo Fixing TextManager.cpp...

:: Create a temporary PowerShell script
echo $content = Get-Content -Path "Graphics\TextManager.cpp" -Raw > temp_fix.ps1
echo $content = $content -replace "    float penX = x;", "    penX = x;" >> temp_fix.ps1
echo $content = $content -replace "    float penY = y;", "    penY = y;" >> temp_fix.ps1
echo $content = $content -replace "    int shaderID = isUI \? SHADER_SPRITE : SHADER_WORLD;", "    shaderID = isUI ? SHADER_SPRITE : SHADER_WORLD;" >> temp_fix.ps1
echo $content = $content -replace "glyph = chars\[c\]\.glyph", "glyph = chars[c]" >> temp_fix.ps1
echo $content | Out-File -FilePath "Graphics\TextManager.cpp" -Encoding UTF8 >> temp_fix.ps1

:: Run the PowerShell script
powershell -ExecutionPolicy Bypass -File temp_fix.ps1

:: Clean up
del temp_fix.ps1

echo TextManager.cpp has been fixed
pause
