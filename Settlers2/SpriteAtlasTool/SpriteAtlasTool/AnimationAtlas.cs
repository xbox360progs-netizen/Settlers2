using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;
using System.IO;

namespace SpriteAtlasTool
{
    public class AnimationAtlas : BaseSpriteAtlas
    {

        public override AtlasType Type
        {
            get { return AtlasType.Animation; }
        }

        // Свойства анимации
        public int FrameRate { get; set; }
        public bool Loop { get; set; }
        public string AnimationName { get; set; }
        public int StartFrame { get; set; }
        public int EndFrame { get; set; }

        // Дополнительные параметры
        public bool PingPong { get; set; }
        public float SpeedMultiplier { get; set; }

        public AnimationAtlas()
        {
            FrameRate = 30;
            Loop = true;
            AnimationName = "Animation";
            StartFrame = 0;
            EndFrame = 0;
            PingPong = false;
            SpeedMultiplier = 1.0f;
        }

        public int TotalFrames
        {
            get { return Sprites.Count; }
        }

        public int DurationMs
        {
            get { return TotalFrames * (1000 / Math.Max(1, FrameRate)); }
        }

        public void SetFrameRange(int start, int end)
        {
            if (start >= 0 && start < Sprites.Count && end >= start && end < Sprites.Count)
            {
                StartFrame = start;
                EndFrame = end;
            }
            else
            {
                StartFrame = 0;
                EndFrame = Math.Max(0, Sprites.Count - 1);
            }
        }

        public SpriteRegion GetFrame(int frameIndex)
        {
            if (frameIndex >= 0 && frameIndex < Sprites.Count)
            {
                return Sprites[frameIndex];
            }
            return null;
        }

        public List<SpriteRegion> GetAnimationFrames()
        {
            if (EndFrame >= StartFrame)
            {
                return Sprites.GetRange(StartFrame, EndFrame - StartFrame + 1);
            }
            return new List<SpriteRegion>();
        }

        public override void SaveToFile(string filePath)
        {
            try
            {
                using (FileStream fs = new FileStream(filePath, FileMode.Create))
                using (BinaryWriter bw = new BinaryWriter(fs))
                {
                    WriteBigEndian(bw, 2); // версия
                    WriteBigEndian(bw, 20); // размер заголовка
                    bw.Write((byte)Type); // AtlasType.Animation

                    byte[] animNameBytes = Encoding.UTF8.GetBytes(AnimationName);
                    WriteBigEndian(bw, (uint)animNameBytes.Length);
                    bw.Write(animNameBytes);

                    WriteBigEndian(bw, (uint)FrameRate);
                    bw.Write(Loop);
                    bw.Write(PingPong);
                    bw.Write(SpeedMultiplier);
                    WriteBigEndian(bw, (uint)StartFrame);
                    WriteBigEndian(bw, (uint)EndFrame);

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
                    }
                }
            }
            catch (Exception ex)
            {
                throw new Exception("Ошибка сохранения AnimationAtlas: " + ex.Message);
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
                    // Читаем заголовок так же, как пишем
                    ushort version = ReadBigEndianUShort(br);    // 2 байта
                    ushort headerSize = ReadBigEndianUShort(br); // 2 байта  
                    byte typeByte = br.ReadByte();               // 1 байт

                    if (typeByte != (byte)AtlasType.Animation)
                        throw new Exception("Неверный тип атласа");

                    uint animNameLength = ReadBigEndian(br);     // uint - 4 байта (это правильно)
                    byte[] animNameBytes = br.ReadBytes((int)animNameLength);
                    AnimationName = Encoding.UTF8.GetString(animNameBytes);

                    uint frameRate = ReadBigEndian(br);          // uint - 4 байта
                    FrameRate = (int)frameRate;
                    Loop = br.ReadBoolean();                     // 1 байт
                    PingPong = br.ReadBoolean();                 // 1 байт
                    SpeedMultiplier = br.ReadSingle();           // 4 байта (float)
                    StartFrame = (int)ReadBigEndian(br);         // uint - 4 байта
                    EndFrame = (int)ReadBigEndian(br);           // uint - 4 байта

                    uint spriteCount = ReadBigEndian(br);        // uint - 4 байта

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

                    if (StartFrame >= Sprites.Count)
                        StartFrame = 0;
                    if (EndFrame >= Sprites.Count || EndFrame < StartFrame)
                        EndFrame = Math.Max(0, Sprites.Count - 1);
                }
            }
            catch (Exception ex)
            {
                throw new Exception("Ошибка загрузки AnimationAtlas: " + ex.Message);
            }
        }
    }
}
