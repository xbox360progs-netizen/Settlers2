import struct
import sys
import os

def read_be_u32(data, pos):
    return struct.unpack('>I', data[pos:pos+4])[0]

def read_be_u16(data, pos):
    return struct.unpack('>H', data[pos:pos+2])[0]

def read_string(data, pos, length):
    return data[pos:pos+length].decode('utf-8', errors='replace')

def dump_bin(filepath):
    with open(filepath, 'rb') as f:
        data = f.read()
    
    pos = 0
    version = read_be_u16(data, pos)
    pos += 2
    header_size = read_be_u16(data, pos)
    pos += 2
    atlas_type = data[pos]
    pos += 1
    
    print(f"File: {os.path.basename(filepath)}")
    print(f"  Version: {version}, HeaderSize: {header_size}, Type: {atlas_type}")
    
    # Atlas name
    name_len = read_be_u32(data, pos)
    pos += 4
    atlas_name = read_string(data, pos, name_len)
    pos += name_len
    print(f"  Atlas name: {atlas_name}")
    
    # Prefix
    prefix_len = read_be_u32(data, pos)
    pos += 4
    prefix = read_string(data, pos, prefix_len)
    pos += prefix_len
    print(f"  Prefix: '{prefix}'")
    
    sprites_per_level = read_be_u32(data, pos)
    pos += 4
    auto_arrange = data[pos]
    pos += 1
    print(f"  SpritesPerLevel: {sprites_per_level}, AutoArrange: {auto_arrange}")
    
    # Groups
    group_count = read_be_u32(data, pos)
    pos += 4
    print(f"  Groups ({group_count}):")
    for g in range(group_count):
        glen = read_be_u32(data, pos)
        pos += 4
        gname = read_string(data, pos, glen)
        pos += glen
        scount = read_be_u32(data, pos)
        pos += 4
        indices = []
        for si in range(scount):
            idx = read_be_u32(data, pos)
            pos += 4
            indices.append(idx)
        print(f"    [{g}] '{gname}': {scount} sprites -> {indices}")
    
    # Total sprite count
    total_sprites = read_be_u32(data, pos)
    pos += 4
    print(f"  Total sprites: {total_sprites}")
    print(f"  Sprite data starts at offset: {pos}")
    
    # Read sprites
    names = []
    for si in range(total_sprites):
        start = pos
        
        # x, y, width, height (16 bytes) — stored as signed int32 in v10+
        x = struct.unpack('>i', data[pos:pos+4])[0]; pos += 4
        y = struct.unpack('>i', data[pos:pos+4])[0]; pos += 4
        w = struct.unpack('>i', data[pos:pos+4])[0]; pos += 4
        h = struct.unpack('>i', data[pos:pos+4])[0]; pos += 4
        
        valid = (x >= 0 and y >= 0 and w > 0 and h > 0 and w < 10000 and h < 10000)
        
        # pivotX, pivotY (4 bytes)
        px = read_be_u16(data, pos); pos += 2
        py = read_be_u16(data, pos); pos += 2
        
        if version >= 2:
            # UV (16 bytes)
            uv_min_x = struct.unpack('>f', data[pos:pos+4])[0]; pos += 4
            uv_min_y = struct.unpack('>f', data[pos:pos+4])[0]; pos += 4
            uv_max_x = struct.unpack('>f', data[pos:pos+4])[0]; pos += 4
            uv_max_y = struct.unpack('>f', data[pos:pos+4])[0]; pos += 4
        
        if version >= 4:
            is_packed = data[pos]; pos += 1
            if is_packed:
                block_off_x = read_be_u32(data, pos); pos += 4
                block_off_y = read_be_u32(data, pos); pos += 4
        
        if version >= 3:
            coll_w = read_be_u32(data, pos); pos += 4
            coll_h = read_be_u32(data, pos); pos += 4
            blocks_mov = data[pos]; pos += 1
            is_trigger = data[pos]; pos += 1
        
        name = ""
        if version >= 6:
            name_len = read_be_u32(data, pos); pos += 4
            if name_len > 0 and name_len < 256:
                name = read_string(data, pos, name_len)
                pos += name_len
                flip_x = data[pos]; pos += 1
            flip_y = data[pos]; pos += 1
        
        if version >= 7:
            coll_off_x = read_be_u32(data, pos); pos += 4
            coll_off_y = read_be_u32(data, pos); pos += 4
        
        if version >= 8:
            mask_count = read_be_u32(data, pos); pos += 4
            for mi in range(min(mask_count, 1024)):
                dx = struct.unpack('>i', data[pos:pos+4])[0]; pos += 4
                dy = struct.unpack('>i', data[pos:pos+4])[0]; pos += 4
        
        if version >= 9:
            entry_count = read_be_u32(data, pos); pos += 4
            for ei in range(min(entry_count, 1024)):
                nx = struct.unpack('>i', data[pos:pos+4])[0]; pos += 4
                ny = struct.unpack('>i', data[pos:pos+4])[0]; pos += 4
                weight = data[pos]; pos += 1
        
        if version >= 10:
            entrance_x = struct.unpack('>i', data[pos:pos+4])[0]; pos += 4
            entrance_y = struct.unpack('>i', data[pos:pos+4])[0]; pos += 4
            is_building = data[pos]; pos += 1
        
        if not valid:
            print(f"    [{si:3d}] (invalid: x={x} y={y} w={w} h={h})")
        elif name:
            names.append(name)
            extra = ""
            if version >= 10:
                extra = f" entrance=({entrance_x},{entrance_y}) isBuilding={is_building}"
            print(f"    [{si:3d}] x={x:5d} y={y:5d} w={w:3d} h={h:3d}  '{name}'{extra}")
        else:
            print(f"    [{si:3d}] x={x:5d} y={y:5d} w={w:3d} h={h:3d}  (no name)")
    
    return names

if __name__ == '__main__':
    base = r"D:\development\Settlers2\Settlers2\Release\Media\Textures"
    for subdir, fname in [
        (r"UI", "Icon.bin"),
        (r"UI", "UI.bin"),
        (r"AtlasTextures", "Buildings.bin"),
        (r"AtlasTextures", "Units.bin"),
    ]:
        path = os.path.join(base, subdir, fname)
        if os.path.exists(path):
            print(f"\n{'='*60}")
            dump_bin(path)
        else:
            print(f"\n{'='*60}")
            print(f"NOT FOUND: {path}")
