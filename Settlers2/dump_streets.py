import struct, os

def read_be_u32(d, p): return struct.unpack('>I', d[p:p+4])[0]
def read_be_u16(d, p): return struct.unpack('>H', d[p:p+2])[0]

base = r'D:\development\Settlers2\Settlers2\Release\Media\Textures'
path = os.path.join(base, 'UI', 'Streets.bin')
with open(path, 'rb') as f:
    data = f.read()

pos = 0
ver = read_be_u16(data, pos); pos += 2
hs = read_be_u16(data, pos); pos += 2
at = data[pos]; pos += 1
print(f'Ver={ver} hdr={hs} type={at}')

nl = read_be_u32(data, pos); pos += 4
aname = data[pos:pos+nl].decode(); pos += nl
print(f'Atlas: {aname}')

pl = read_be_u32(data, pos); pos += 4
pref = data[pos:pos+pl].decode(); pos += pl
print(f'Prefix: {pref}')

spl = read_be_u32(data, pos); pos += 4
aa = data[pos]; pos += 1
print(f'SPL={spl} AA={aa}')

gc = read_be_u32(data, pos); pos += 4
print(f'Groups: {gc}')
for g in range(gc):
    gl = read_be_u32(data, pos); pos += 4
    gn = data[pos:pos+gl].decode(); pos += gl
    sc = read_be_u32(data, pos); pos += 4
    idxs = []
    for si in range(sc):
        idxs.append(read_be_u32(data, pos)); pos += 4
    print(f'  [{g}] "{gn}": {idxs}')

ts = read_be_u32(data, pos); pos += 4
print(f'Total sprites: {ts}')

for si in range(ts):
    s = pos
    x = struct.unpack('>i', data[pos:pos+4])[0]; pos += 4
    y = struct.unpack('>i', data[pos:pos+4])[0]; pos += 4
    w = struct.unpack('>i', data[pos:pos+4])[0]; pos += 4
    h = struct.unpack('>i', data[pos:pos+4])[0]; pos += 4
    
    valid = x>=0 and y>=0 and w>0 and h>0 and w<10000 and h<10000
    
    px = read_be_u16(data, pos); pos += 2
    py = read_be_u16(data, pos); pos += 2
    
    if ver >= 2:
        for _ in range(4):
            struct.unpack('>f', data[pos:pos+4])[0]; pos += 4
    
    if ver >= 4:
        ip = data[pos]; pos += 1
        if ip:
            pos += 8
    
    if ver >= 3:
        pos += 10
    
    name = ''
    if ver >= 6:
        nl2 = read_be_u32(data, pos); pos += 4
        if nl2 > 0 and nl2 < 256:
            name = data[pos:pos+nl2].decode(errors='replace'); pos += nl2
        pos += 2
    
    if valid and name:
        print(f'  [{si}] {name} ({x},{y} {w}x{h})')
