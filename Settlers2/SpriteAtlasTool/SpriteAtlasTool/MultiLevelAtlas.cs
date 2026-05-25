using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;
using System.IO;

namespace SpriteAtlasTool
{
    public class MultiLevelAtlas : BaseSpriteAtlas
    {
        private int _spritesPerLevel;

        public override AtlasType Type
        {
            get { return AtlasType.MultiLevel; }
        }

    // Пользовательские группы
    public List<SpriteGroup> Groups { get; private set; }

    // Свойства многоуровневого атласа
    public int SpritesPerLevel
    {
        get { return _spritesPerLevel; }
        set { _spritesPerLevel = value > 0 ? value : 2; }
    }

    public string LevelPrefix { get; set; }
    public bool AutoArrange { get; set; }

    public int LevelCount
    {
        get
        {
            if (Groups.Count == 0)
                return 0;
            return Groups.Count;
        }
    }

    public MultiLevelAtlas()
    {
        Groups = new List<SpriteGroup>();
        _spritesPerLevel = 2;
        LevelPrefix = "Level";
        AutoArrange = false;
        Name = "MultiLevelAtlas"; 
    }

        // Создать новую группу
        public SpriteGroup CreateGroup(string groupName)
        {
            SpriteGroup group = new SpriteGroup(groupName);
            Groups.Add(group);
            return group;
        }

        // Удалить группу
        public void RemoveGroup(SpriteGroup group)
        {
            if (Groups.Contains(group))
            {
                // Просто удаляем группу, спрайты остаются и в основном списке Sprites
                Groups.Remove(group);
            }
        }

        // Добавить спрайт в группу
        public void AddSpriteToGroup(SpriteRegion sprite, SpriteGroup group)
        {
            if (!Sprites.Contains(sprite))
            {
                Sprites.Add(sprite);
            }

            if (!group.Sprites.Contains(sprite))
            {
                group.Sprites.Add(sprite);
            }
        }

        // Удалить спрайт из группы
        public void RemoveSpriteFromGroup(SpriteRegion sprite, SpriteGroup group)
        {
            if (group.Sprites.Contains(sprite))
            {
                group.Sprites.Remove(sprite);

                // Если спрайт не используется в других группах, удаляем его совсем
                bool usedInOtherGroups = false;
                foreach (SpriteGroup g in Groups)
                {
                    if (g != group && g.Sprites.Contains(sprite))
                    {
                        usedInOtherGroups = true;
                        break;
                    }
                }

                if (!usedInOtherGroups)
                {
                    Sprites.Remove(sprite);
                }
            }
        }

        // Получить группу по индексу
        public SpriteGroup GetGroup(int index)
        {
            if (index >= 0 && index < Groups.Count)
            {
                return Groups[index];
            }
            return null;
        }

        // Получить спрайты для последовательной записи (по порядку групп)
        public List<SpriteRegion> GetSpritesForSerialization()
        {
            List<SpriteRegion> result = new List<SpriteRegion>();

            foreach (SpriteGroup group in Groups)
            {
                // Добавляем спрайты группы, но ограничиваем SpritesPerLevel
                int count = Math.Min(group.Sprites.Count, SpritesPerLevel);
                for (int i = 0; i < count; i++)
                {
                    if (!result.Contains(group.Sprites[i]))
                    {
                        result.Add(group.Sprites[i]);
                    }
                }
            }

            // Добавляем оставшиеся спрайты, которые не вошли в группы
            foreach (SpriteRegion sprite in Sprites)
            {
                if (!result.Contains(sprite))
                {
                    result.Add(sprite);
                }
            }

            return result;
        }

        public string GetGroupName(int groupIndex)
        {
            if (groupIndex >= 0 && groupIndex < Groups.Count)
            {
                return Groups[groupIndex].Name;
            }
            return LevelPrefix + "_" + groupIndex.ToString();
        }

        public override void SaveToFile(string filePath)
        {
            try
            {
                using (FileStream fs = new FileStream(filePath, FileMode.Create))
                using (BinaryWriter bw = new BinaryWriter(fs))
                {
                    // Проверяем, есть ли у спрайтов новые функции
                    bool hasTransformFeatures = Sprites.Any(s =>
                        s.FlipX || s.FlipY || !string.IsNullOrEmpty(s.Name));
                    bool hasCollisionOffset = Sprites.Any(s =>
                        s.Collision.OffsetX != 0 || s.Collision.OffsetY != 0);
                    bool hasMask = Sprites.Any(s =>
                        s.Collision.MaskTiles != null && s.Collision.MaskTiles.Count > 0);
                    bool hasAdvancedFeatures = Sprites.Any(s =>
                        s.Collision.Width > 1 ||
                        s.Collision.Height > 1 ||
                        s.Collision.BlocksMovement != true ||
                        s.Collision.IsTrigger != false ||
                        s.IsPacked ||
                        hasTransformFeatures ||
                        hasCollisionOffset);

                    // Новая версия для поддержки трансформаций/имен и смещения коллайдера
                    uint version = hasMask ? 8u :
                                   hasCollisionOffset ? 7u :
                                   (hasTransformFeatures ? 6u : (hasAdvancedFeatures ? 4u : 3u));

                    WriteBigEndian(bw, (ushort)version);
                    WriteBigEndian(bw, (ushort)24);
                    bw.Write((byte)Type);

                    // Метаданные атласа
                    byte[] nameBytes = Encoding.UTF8.GetBytes(Name ?? "MultiLevelAtlas");
                    WriteBigEndian(bw, (uint)nameBytes.Length);
                    bw.Write(nameBytes);

                    // Метаданные MultiLevel
                    byte[] prefixBytes = Encoding.UTF8.GetBytes(LevelPrefix ?? "Level");
                    WriteBigEndian(bw, (uint)prefixBytes.Length);
                    bw.Write(prefixBytes);
                    WriteBigEndian(bw, (uint)SpritesPerLevel);
                    bw.Write(AutoArrange);

                    // Группы
                    WriteBigEndian(bw, (uint)Groups.Count);
                    for (int i = 0; i < Groups.Count; i++)
                    {
                        SpriteGroup group = Groups[i];
                        byte[] groupNameBytes = Encoding.UTF8.GetBytes(group.Name ?? ("Group" + i));
                        WriteBigEndian(bw, (uint)groupNameBytes.Length);
                        bw.Write(groupNameBytes);

                        // Спрайты в группе
                        WriteBigEndian(bw, (uint)group.Sprites.Count);
                        foreach (SpriteRegion sprite in group.Sprites)
                        {
                            int spriteIndex = Sprites.IndexOf(sprite);
                            WriteBigEndian(bw, (uint)spriteIndex);
                        }
                    }

                    // ВАЖНО: Используем Sprites
                    WriteBigEndian(bw, (uint)Sprites.Count);

                    foreach (var sprite in Sprites)
                    {
                        WriteBigEndian(bw, (uint)sprite.OriginalBounds.X);
                        WriteBigEndian(bw, (uint)sprite.OriginalBounds.Y);
                        WriteBigEndian(bw, (uint)sprite.OriginalBounds.Width);
                        WriteBigEndian(bw, (uint)sprite.OriginalBounds.Height);

                        if (sprite.HasPivot)
                        {
                            WriteBigEndian(bw, (ushort)sprite.Pivot.X);
                            WriteBigEndian(bw, (ushort)sprite.Pivot.Y);
                        }
                        else
                        {
                            WriteBigEndian(bw, (ushort)0xFFFF);
                            WriteBigEndian(bw, (ushort)0xFFFF);
                        }

                        WriteBigEndian(bw, sprite.UV_Min.X);
                        WriteBigEndian(bw, sprite.UV_Min.Y);
                        WriteBigEndian(bw, sprite.UV_Max.X);
                        WriteBigEndian(bw, sprite.UV_Max.Y);

                        // Начиная с версии 4 - сохраняем IsPacked явно
                        if (version >= 4)
                        {
                            bw.Write(sprite.IsPacked);
                            if (sprite.IsPacked)
                            {
                                WriteBigEndian(bw, (uint)sprite.BlockOffset.X);
                                WriteBigEndian(bw, (uint)sprite.BlockOffset.Y);
                            }
                        }

                        // Начиная с версии 5 - сохраняем коллайдер
                        if (version >= 3)
                        {
                            WriteBigEndian(bw, (uint)sprite.Collision.Width);
                            WriteBigEndian(bw, (uint)sprite.Collision.Height);
                            bw.Write(sprite.Collision.BlocksMovement);
                            bw.Write(sprite.Collision.IsTrigger);
                        }

                        // Начиная с версии 6 - сохраняем имена и трансформации
                        if (version >= 6)
                        {
                            // Сохраняем имя спрайта
                            byte[] spriteNameBytes = Encoding.UTF8.GetBytes(sprite.Name ?? "");
                            WriteBigEndian(bw, (uint)spriteNameBytes.Length);
                            bw.Write(spriteNameBytes);

                            // Сохраняем трансформации
                            bw.Write(sprite.FlipX);
                            bw.Write(sprite.FlipY);
                        }

                        // Начиная с версии 7 - сохраняем смещение коллайдера
                        if (version >= 7)
                        {
                            WriteBigEndian(bw, (uint)(int)sprite.Collision.OffsetX);
                            WriteBigEndian(bw, (uint)(int)sprite.Collision.OffsetY);
                        }

                        // Начиная с версии 8 - сохраняем маску тайлов коллизии
                        if (version >= 8)
                        {
                            var mask = sprite.Collision.MaskTiles;
                            uint maskCount = (mask != null) ? (uint)mask.Count : 0u;
                            WriteBigEndian(bw, maskCount);
                            if (mask != null)
                            {
                                foreach (Point p in mask)
                                {
                                    WriteBigEndian(bw, (uint)(int)p.X);
                                    WriteBigEndian(bw, (uint)(int)p.Y);
                                }
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                throw new Exception("Ошибка сохранения MultiLevelAtlas: " + ex.Message);
            }
        }

        public override void LoadFromFile(string filePath)
        {
            try
            {
                Sprites.Clear();
                Groups.Clear();

                using (FileStream fs = new FileStream(filePath, FileMode.Open))
                using (BinaryReader br = new BinaryReader(fs))
                {
                    // Читаем заголовок так же, как пишем
                    ushort version = ReadBigEndianUShort(br);    // 2 байта
                    ushort headerSize = ReadBigEndianUShort(br); // 2 байта
                    byte typeByte = br.ReadByte();               // 1 байт

                    // Читаем метаданные атласа
                    uint nameLength = ReadBigEndian(br);         // uint - 4 байта
                    byte[] nameBytes = br.ReadBytes((int)nameLength);
                    Name = Encoding.UTF8.GetString(nameBytes);

                    // Читаем метаданные MultiLevel
                    uint prefixLength = ReadBigEndian(br);       // uint - 4 байта
                    byte[] prefixBytes = br.ReadBytes((int)prefixLength);
                    LevelPrefix = Encoding.UTF8.GetString(prefixBytes);
                    uint spritesPerLevel = ReadBigEndian(br);    // uint - 4 байта
                    SpritesPerLevel = (int)spritesPerLevel;
                    AutoArrange = br.ReadBoolean();              // 1 байт

                    // Читаем группы
                    uint groupCount = ReadBigEndian(br);         // uint - 4 байта
                    List<List<uint>> groupSpriteIndices = new List<List<uint>>();

                    for (int i = 0; i < groupCount; i++)
                    {
                        uint groupNameLength = ReadBigEndian(br); // uint - 4 байта
                        byte[] groupNameBytes = br.ReadBytes((int)groupNameLength);
                        string groupName = Encoding.UTF8.GetString(groupNameBytes);

                        SpriteGroup group = new SpriteGroup(groupName);
                        Groups.Add(group);

                        // Читаем индексы спрайтов в группе
                        uint spriteCountInGroup = ReadBigEndian(br); // uint - 4 байта
                        List<uint> spriteIndices = new List<uint>();

                        for (int j = 0; j < spriteCountInGroup; j++)
                        {
                            uint spriteIndex = ReadBigEndian(br);    // uint - 4 байта
                            spriteIndices.Add(spriteIndex);
                        }

                        groupSpriteIndices.Add(spriteIndices);
                    }

                    // Читаем все спрайты
                    uint spriteCount = ReadBigEndian(br);        // uint - 4 байта
                    List<SpriteRegion> loadedSprites = new List<SpriteRegion>();

                    for (int i = 0; i < spriteCount; i++)
                    {
                        uint x = ReadBigEndian(br);              // uint - 4 байта
                        uint y = ReadBigEndian(br);              // uint - 4 байта
                        uint width = ReadBigEndian(br);          // uint - 4 байта
                        uint height = ReadBigEndian(br);         // uint - 4 байта

                        Rectangle originalBounds = new Rectangle((int)x, (int)y, (int)width, (int)height);
                        Rectangle bounds = originalBounds;

                        ushort pivotX = ReadBigEndianUShort(br); // ushort - 2 байта
                        ushort pivotY = ReadBigEndianUShort(br); // ushort - 2 байта

                        Point pivot = (pivotX != 0xFFFF && pivotY != 0xFFFF)
                            ? new Point(pivotX, pivotY)
                            : new Point(-1, -1);

                        PointF uvMin = new PointF(0, 0);
                        PointF uvMax = new PointF(1, 1);

                        if (version >= 2)
                        {
                            uvMin = new PointF(ReadBigEndianFloat(br), ReadBigEndianFloat(br));  // 4+4 байта
                            uvMax = new PointF(ReadBigEndianFloat(br), ReadBigEndianFloat(br));  // 4+4 байта
                        }

                        var sprite = new SpriteRegion(bounds)
                        {
                            OriginalBounds = originalBounds,
                            Pivot = pivot,
                            UV_Min = uvMin,
                            UV_Max = uvMax
                        };

                        // Начиная с версии 4 - читаем IsPacked явно
                        if (version >= 4)
                        {
                            sprite.IsPacked = br.ReadBoolean();
                            if (sprite.IsPacked)
                            {
                                uint blockOffsetX = ReadBigEndian(br);
                                uint blockOffsetY = ReadBigEndian(br);
                                sprite.BlockOffset = new Point((int)blockOffsetX, (int)blockOffsetY);
                            }
                        }
                        else
                        {
                            // Для старых версий - определяем по UV (с риском ошибки)
                            bool hasUvData =
                                Math.Abs(uvMin.X) > 0.0001f || Math.Abs(uvMin.Y) > 0.0001f ||
                                Math.Abs(uvMax.X - 1.0f) > 0.0001f || Math.Abs(uvMax.Y - 1.0f) > 0.0001f;

                            if (hasUvData)
                            {
                                sprite.IsPacked = true;
                                float uvWidth = uvMax.X - uvMin.X;
                                float uvHeight = uvMax.Y - uvMin.Y;
                                if (uvWidth > 0.0001f && uvHeight > 0.0001f)
                                {
                                    float blockWidth = originalBounds.Width / uvWidth;
                                    float blockHeight = originalBounds.Height / uvHeight;
                                    sprite.BlockOffset = new Point(
                                        (int)Math.Round(uvMin.X * blockWidth),
                                        (int)Math.Round(uvMin.Y * blockHeight));
                                }
                            }
                        }

                        // Загружаем коллайдер если версия >= 3
                        if (version >= 3)
                        {
                            uint collWidth = ReadBigEndian(br);
                            uint collHeight = ReadBigEndian(br);
                            bool blocks = br.ReadBoolean();
                            bool trigger = br.ReadBoolean();

                            sprite.Collision = new CollisionInfo
                            {
                                Width = (int)collWidth,
                                Height = (int)collHeight,
                                BlocksMovement = blocks,
                                IsTrigger = trigger
                            };
                        }

                        // Начиная с версии 6 - читаем имена и трансформации
                        if (version >= 6)
                        {
                            // Читаем имя спрайта
                            uint nameLengthSprite = ReadBigEndian(br);
                            if (nameLengthSprite > 0)
                            {
                                byte[] nameBytesSprite = br.ReadBytes((int)nameLengthSprite);
                                sprite.Name = Encoding.UTF8.GetString(nameBytesSprite);
                            }

                            // Читаем трансформации
                            sprite.FlipX = br.ReadBoolean();
                            sprite.FlipY = br.ReadBoolean();
                        }

                        // Начиная с версии 7 - читаем смещение коллайдера
                        if (version >= 7 && sprite.Collision != null)
                        {
                            sprite.Collision.OffsetX = (int)ReadBigEndian(br);
                            sprite.Collision.OffsetY = (int)ReadBigEndian(br);
                        }

                        // Начиная с версии 8 - читаем маску тайлов коллизии
                        if (version >= 8 && sprite.Collision != null)
                        {
                            uint maskCount = ReadBigEndian(br);
                            sprite.Collision.MaskTiles.Clear();
                            for (uint m = 0; m < maskCount; m++)
                            {
                                int dx = (int)ReadBigEndian(br);
                                int dy = (int)ReadBigEndian(br);
                                sprite.Collision.MaskTiles.Add(new Point(dx, dy));
                            }
                        }

                        sprite.DisplayBounds = originalBounds;
                        Sprites.Add(sprite);
                        loadedSprites.Add(sprite);
                    }

                    // Восстанавливаем связи групп и спрайтов
                    for (int i = 0; i < Groups.Count && i < groupSpriteIndices.Count; i++)
                    {
                        SpriteGroup group = Groups[i];
                        List<uint> indices = groupSpriteIndices[i];

                        foreach (uint spriteIndex in indices)
                        {
                            if (spriteIndex < loadedSprites.Count)
                            {
                                group.Sprites.Add(loadedSprites[(int)spriteIndex]);
                            }
                        }
                    }
                }

                // Автоматически генерируем имена для спрайтов, если они не были загружены
                AutoGenerateSpriteNamesAfterLoad();
            }
            catch (Exception ex)
            {
                throw new Exception("Ошибка загрузки MultiLevelAtlas: " + ex.Message);
            }
        }
        private void AutoGenerateSpriteNamesAfterLoad()
        {
            HashSet<string> usedNames = new HashSet<string>();

            // Сначала собираем все существующие имена
            foreach (SpriteRegion sprite in Sprites)
            {
                if (!string.IsNullOrEmpty(sprite.Name))
                {
                    usedNames.Add(sprite.Name.ToLower());
                }
            }

            // Генерируем имена для спрайтов без имен
            for (int i = 0; i < Sprites.Count; i++)
            {
                SpriteRegion sprite = Sprites[i];
                if (string.IsNullOrEmpty(sprite.Name))
                {
                    string baseName = "Sprite_" + (i + 1).ToString("D3");
                    string uniqueName = baseName;
                    int counter = 1;

                    while (usedNames.Contains(uniqueName.ToLower()))
                    {
                        uniqueName = baseName + "_" + counter;
                        counter++;
                    }

                    sprite.Name = uniqueName;
                    usedNames.Add(uniqueName.ToLower());
                }
            }
        }
        
    }

    // Класс для представления группы спрайтов
    public class SpriteGroup
    {
        public string Name { get; set; }
        public List<SpriteRegion> Sprites { get; private set; }

        public SpriteGroup(string name)
        {
            Name = name ?? "Group";
            Sprites = new List<SpriteRegion>();
        }

        public override string ToString()
        {
            return Name + " (" + Sprites.Count + " sprites)";
        }
    }
}
