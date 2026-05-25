using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;
using System.IO;
using System.Windows.Forms;

namespace SpriteAtlasTool
{
    public class SingleSpriteAtlas : BaseSpriteAtlas
    {
        public Point Hotspot { get; set; }
        public string SpriteName { get; set; }

        public override AtlasType Type
        {
            get { return AtlasType.SingleSprite; }
        }

        public SingleSpriteAtlas()
        {
            Hotspot = new Point(0, 0);
            SpriteName = "Sprite";
        }

        public SpriteRegion GetSprite()
        {
            return Sprites.Count > 0 ? Sprites[0] : null;
        }

        public void SetSprite(SpriteRegion sprite)
        {
            Sprites.Clear();
            if (sprite != null)
            {
                Sprites.Add(sprite);
            }
        }

        public override void SaveToFile(string filePath)
        {
            try
            {
                using (FileStream fs = new FileStream(filePath, FileMode.Create))
                using (BinaryWriter bw = new BinaryWriter(fs))
                {
                    // ВСЕГДА используем версию 3 если есть коллайдеры ИЛИ пивот
                    bool hasAdvancedFeatures =
                        Sprites.Any(s => s.Collision.Width > 0 || s.Collision.Height > 0);
                    bool hasCollisionOffset = Sprites.Any(s =>
                        s.Collision.OffsetX != 0 || s.Collision.OffsetY != 0);
                    bool hasMask = Sprites.Any(s =>
                        s.Collision.MaskTiles != null && s.Collision.MaskTiles.Count > 0);
                    bool hasNodeWeight = Sprites.Any(s => s.NodeWeights.Entries.Count > 0);

                    uint version = hasNodeWeight ? 9u :
                                   hasMask ? 8u :
                                   hasCollisionOffset ? 7u : (hasAdvancedFeatures ? 3u : 2u);

                    WriteBigEndian(bw, version);
                    WriteBigEndian(bw, 16u);
                    bw.Write((byte)Type);

                    byte[] nameBytes = Encoding.UTF8.GetBytes(SpriteName);
                    WriteBigEndian(bw, (uint)nameBytes.Length);
                    bw.Write(nameBytes);

                    WriteBigEndian(bw, (uint)Hotspot.X);
                    WriteBigEndian(bw, (uint)Hotspot.Y);

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

                        // Если версия 3 - сохраняем коллайдер
                        if (version >= 3)
                        {
                            WriteBigEndian(bw, (uint)sprite.Collision.Width);
                            WriteBigEndian(bw, (uint)sprite.Collision.Height);
                            bw.Write(sprite.Collision.BlocksMovement);
                            bw.Write(sprite.Collision.IsTrigger);
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

                        // Начиная с версии 9 - сохраняем веса узлов (NodeWeight entries)
                        if (version >= 9)
                        {
                            var entries = sprite.NodeWeights.Entries;
                            WriteBigEndian(bw, (uint)entries.Count);
                            foreach (NodeWeightEntry e in entries)
                            {
                                WriteBigEndian(bw, (uint)e.NX);
                                WriteBigEndian(bw, (uint)e.NY);
                                bw.Write(e.Weight);
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                throw new Exception("Ошибка сохранения: " + ex.Message);
            }
        }


        public override void LoadFromFile(string filePath)
        {
            try
            {
                Sprites.Clear();

                using (FileStream fs = new FileStream(filePath, FileMode.Open))
                using (BinaryReader br = new BinaryReader(fs))
                {
                    uint version = ReadBigEndian(br);
                    uint headerSize = ReadBigEndian(br);
                    AtlasType type = (AtlasType)br.ReadByte();

                    if (type != AtlasType.SingleSprite)
                    {
                        throw new Exception("Неверный тип атласа");
                    }

                    uint nameLength = ReadBigEndian(br);
                    byte[] nameBytes = br.ReadBytes((int)nameLength);
                    SpriteName = Encoding.UTF8.GetString(nameBytes);

                    uint hotspotX = ReadBigEndian(br);
                    uint hotspotY = ReadBigEndian(br);
                    Hotspot = new Point((int)hotspotX, (int)hotspotY);

                    uint spriteCount = ReadBigEndian(br);

                    for (int i = 0; i < spriteCount; i++)
                    {
                        uint x = ReadBigEndian(br);
                        uint y = ReadBigEndian(br);
                        uint width = ReadBigEndian(br);
                        uint height = ReadBigEndian(br);

                        Rectangle originalBounds = new Rectangle((int)x, (int)y, (int)width, (int)height);
                        Rectangle bounds = originalBounds;

                        ushort pivotX = ReadBigEndianUShort(br);
                        ushort pivotY = ReadBigEndianUShort(br);

                        Point pivot = (pivotX != 0xFFFF && pivotY != 0xFFFF)
                            ? new Point(pivotX, pivotY)
                            : new Point(-1, -1);
/*
                        string loadDebug = string.Format("Загружен пивот: {0},{1} (raw: {2},{3})",
    pivot.X, pivot.Y, pivotX, pivotY);
                        MessageBox.Show(loadDebug, "Отладка LoadFromFile", MessageBoxButtons.OK, MessageBoxIcon.Information);
*/
                        PointF uvMin = new PointF(0, 0);
                        PointF uvMax = new PointF(1, 1);


                        if (version >= 2)
                        {
                            uvMin = new PointF(ReadBigEndianFloat(br), ReadBigEndianFloat(br));
                            uvMax = new PointF(ReadBigEndianFloat(br), ReadBigEndianFloat(br));
                        }

                        var sprite = new SpriteRegion(bounds)
                        {
                            OriginalBounds = originalBounds,
                            Pivot = pivot,
                            UV_Min = uvMin,
                            UV_Max = uvMax
                        };

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

                        // Начиная с версии 9 - читаем веса узлов (NodeWeight entries)
                        if (version >= 9)
                        {
                            uint entryCount = ReadBigEndian(br);
                            sprite.NodeWeights.Entries.Clear();
                            for (uint ei = 0; ei < entryCount; ei++)
                            {
                                int nx = (int)ReadBigEndian(br);
                                int ny = (int)ReadBigEndian(br);
                                byte w = br.ReadByte();
                                sprite.NodeWeights.Entries.Add(new NodeWeightEntry(nx, ny, w));
                            }
                        }

                        sprite.DisplayBounds = originalBounds;
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

                        Sprites.Add(sprite);
                    }
                }
            }
            catch (Exception ex)
            {
                throw new Exception("Ошибка загрузки SingleSpriteAtlas: " + ex.Message);
            }
        }


    }
}
