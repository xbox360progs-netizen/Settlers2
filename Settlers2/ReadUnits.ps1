function R4($br) { $b = $br.ReadBytes(4); [array]::Reverse($b); return [System.BitConverter]::ToUInt32($b,0) }
function R2($br) { $b = $br.ReadBytes(2); [array]::Reverse($b); return [System.BitConverter]::ToUInt16($b,0) }
function RF($br) { $b = $br.ReadBytes(4); [array]::Reverse($b); return [System.BitConverter]::ToSingle($b,0) }

$path = "D:\development\Settlers2\Settlers2\Release\Media\Textures\AtlasTextures\Units.bin"
$fs = [System.IO.File]::OpenRead($path)
$br = New-Object System.IO.BinaryReader($fs)
$ver = R2 $br; $hs = R2 $br; $type = $br.ReadByte()
Write-Host ("Version=" + $ver + " HeaderSize=" + $hs + " Type=" + $type)
$nl = R4 $br; $name = [Text.Encoding]::UTF8.GetString($br.ReadBytes($nl))
Write-Host ("Name=" + $name)
if ($type -eq 1) {
    $pl = R4 $br; $prefix = [Text.Encoding]::UTF8.GetString($br.ReadBytes($pl))
    $spl = R4 $br; $aa = $br.ReadBoolean()
    Write-Host ("Prefix=" + $prefix)
    $gc = R4 $br; Write-Host ("Groups=" + $gc)
    for ($gi=0; $gi -lt $gc; $gi++) {
        $gnl = R4 $br; $gn = [Text.Encoding]::UTF8.GetString($br.ReadBytes($gnl))
        $scig = R4 $br; Write-Host ("  Group " + $gn + " : " + $scig)
        for ($j=0; $j -lt $scig; $j++) { $si = R4 $br }
    }
    $sc = R4 $br; Write-Host ("Sprites=" + $sc)
    for ($i=0; $i -lt $sc; $i++) {
        $sx = R4 $br; $sy = R4 $br; $sw = R4 $br; $sh = R4 $br
        $px = R2 $br; $py = R2 $br
        $line = ("  " + $i + ": rect=(" + $sx + "," + $sy + " " + $sw + "x" + $sh + ")")
        if ($ver -ge 2) { $u = RF $br; $v = RF $br; $u2 = RF $br; $v2 = RF $br }
        if ($ver -ge 4) { $ip = $br.ReadBoolean(); if ($ip) { $bx = R4 $br; $by = R4 $br } }
        if ($ver -ge 3) { $cw = R4 $br; $ch = R4 $br; $bl = $br.ReadBoolean(); $tr = $br.ReadBoolean() }
        if ($ver -ge 6) {
            $snl = R4 $br; $sn = ""
            if ($snl -gt 0) { $sn = [Text.Encoding]::UTF8.GetString($br.ReadBytes($snl)) }
            $fx = $br.ReadBoolean(); $fy = $br.ReadBoolean()
            Write-Host ($line + " Name='" + $sn + "'")
        } else { Write-Host $line }
    }
}
$br.Close(); $fs.Close()
