using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Drawing;

namespace SpriteAtlasTool
{
    public abstract class BaseSpriteAtlas : ISpriteAtlas
    {
        public string Name { get; set; }
        public abstract AtlasType Type { get; }
        public List<SpriteRegion> Sprites { get; private set; }

        protected BaseSpriteAtlas()
        {
            Sprites = new List<SpriteRegion>();
            Name = "Atlas";
        }

        public abstract void SaveToFile(string filePath);
        public abstract void LoadFromFile(string filePath);

        /// <summary>
        /// Проверка существования файла
        /// </summary>
        /// <param name="filePath">Путь к файлу</param>
        /// <returns>True если файл существует</returns>
        protected bool FileExists(string filePath)
        {
            return !string.IsNullOrEmpty(filePath) && File.Exists(filePath);
        }

        /// <summary>
        /// Валидация пути к файлу
        /// </summary>
        /// <param name="filePath">Путь к файлу</param>
        protected void ValidateFilePath(string filePath)
        {
            if (string.IsNullOrEmpty(filePath))
                throw new ArgumentException("Путь к файлу не может быть пустым");

            string directory = Path.GetDirectoryName(filePath);
            if (!string.IsNullOrEmpty(directory) && !Directory.Exists(directory))
                throw new DirectoryNotFoundException("Директория не существует: " + directory);
        }

        /// <summary>
        /// Получить количество спрайтов
        /// </summary>
        public int SpriteCount
        {
            get { return Sprites != null ? Sprites.Count : 0; }
        }

        /// <summary>
        /// Очистить все спрайты
        /// </summary>
        public void ClearSprites()
        {
            if (Sprites != null)
                Sprites.Clear();
        }

        /// <summary>
        /// Добавить спрайт
        /// </summary>
        /// <param name="sprite">Спрайт для добавления</param>
        public void AddSprite(SpriteRegion sprite)
        {
            if (sprite != null && Sprites != null)
                Sprites.Add(sprite);
        }

        /// <summary>
        /// Удалить спрайт
        /// </summary>
        /// <param name="sprite">Спрайт для удаления</param>
        public void RemoveSprite(SpriteRegion sprite)
        {
            if (sprite != null && Sprites != null)
                Sprites.Remove(sprite);
        }

        public virtual void PackToFixedSize(int blockSize = 256, int textureWidth = 1280, int textureHeight = 720)
        {
            foreach (var sprite in Sprites)
            {
                sprite.OriginalBounds = sprite.Bounds;
                sprite.IsPacked = true;

                // Центрируем в блоке (только для пакинга)
                int padX = (blockSize - sprite.OriginalBounds.Width) / 2;
                int padY = (blockSize - sprite.OriginalBounds.Height) / 2;

                // Bounds используются ТОЛЬКО для пакинга
                sprite.Bounds = new Rectangle(padX, padY, sprite.OriginalBounds.Width, sprite.OriginalBounds.Height);
                sprite.BlockOffset = new Point(padX, padY);

                // UV относительно БЛОКА
                float uMin = (float)padX / blockSize;
                float vMin = (float)padY / blockSize;
                float uMax = (float)(padX + sprite.OriginalBounds.Width) / blockSize;
                float vMax = (float)(padY + sprite.OriginalBounds.Height) / blockSize;

                sprite.UV_Min = new PointF(uMin, vMin);
                sprite.UV_Max = new PointF(uMax, vMax);

                // Применяем трансформации зеркалирования к UV
                if (sprite.FlipX)
                {
                    float temp = sprite.UV_Min.X;
                    sprite.UV_Min.X = sprite.UV_Max.X;
                    sprite.UV_Max.X = temp;
                }

                if (sprite.FlipY)
                {
                    float temp = sprite.UV_Min.Y;
                    sprite.UV_Min.Y = sprite.UV_Max.Y;
                    sprite.UV_Max.Y = temp;
                }

                // Корректируем pivot
                if (sprite.HasPivot)
                {
                    int correctedPivotX = Math.Min(sprite.Pivot.X, sprite.OriginalBounds.Width - 1);
                    int correctedPivotY = Math.Min(sprite.Pivot.Y, sprite.OriginalBounds.Height - 1);
                    sprite.Pivot = new Point(correctedPivotX, correctedPivotY);
                }
            }
        }

        public virtual void CalculateUVFromRealCoordinates(int textureWidth, int textureHeight)
        {
            if (textureWidth <= 0 || textureHeight <= 0) return;

            foreach (var sprite in Sprites)
            {
                // Рассчитываем UV координаты относительно всей текстуры
                sprite.UV_Min = new PointF(
                    (float)sprite.OriginalBounds.X / textureWidth,
                    (float)sprite.OriginalBounds.Y / textureHeight
                );

                sprite.UV_Max = new PointF(
                    (float)(sprite.OriginalBounds.X + sprite.OriginalBounds.Width) / textureWidth,
                    (float)(sprite.OriginalBounds.Y + sprite.OriginalBounds.Height) / textureHeight
                );

                // Применяем трансформации зеркалирования
                if (sprite.FlipX)
                {
                    // Меняем местами UV по X
                    float temp = sprite.UV_Min.X;
                    sprite.UV_Min.X = sprite.UV_Max.X;
                    sprite.UV_Max.X = temp;
                }

                if (sprite.FlipY)
                {
                    // Меняем местами UV по Y
                    float temp = sprite.UV_Min.Y;
                    sprite.UV_Min.Y = sprite.UV_Max.Y;
                    sprite.UV_Max.Y = temp;
                }

                // Помечаем как упакованный если UV не стандартные или есть трансформации
                sprite.IsPacked = !(Math.Abs(sprite.UV_Min.X) < 0.0001f &&
                                   Math.Abs(sprite.UV_Min.Y) < 0.0001f &&
                                   Math.Abs(sprite.UV_Max.X - 1.0f) < 0.0001f &&
                                   Math.Abs(sprite.UV_Max.Y - 1.0f) < 0.0001f) ||
                                 sprite.FlipX || sprite.FlipY;

                // BlockOffset для совместимости
                sprite.BlockOffset = new Point(0, 0);
            }
        }

        protected void WriteBigEndian(BinaryWriter bw, ushort value)
        {
            byte[] bytes = BitConverter.GetBytes(value);
            if (BitConverter.IsLittleEndian)
                Array.Reverse(bytes);
            bw.Write(bytes);
        }

        protected void WriteBigEndian(BinaryWriter bw, float value)
        {
            byte[] bytes = BitConverter.GetBytes(value);
            if (BitConverter.IsLittleEndian)
                Array.Reverse(bytes);
            bw.Write(bytes);
        }

        protected void WriteBigEndian(BinaryWriter bw, uint value)
        {
            byte[] bytes = BitConverter.GetBytes(value);
            if (BitConverter.IsLittleEndian)
                Array.Reverse(bytes);
            bw.Write(bytes);
        }

        protected uint ReadBigEndian(BinaryReader br)
        {
            byte[] bytes = br.ReadBytes(4);
            if (BitConverter.IsLittleEndian)
                Array.Reverse(bytes);
            return BitConverter.ToUInt32(bytes, 0);
        }
        protected float ReadBigEndianFloat(BinaryReader br)
        {
            byte[] bytes = br.ReadBytes(4);
            if (BitConverter.IsLittleEndian)
                Array.Reverse(bytes);
            return BitConverter.ToSingle(bytes, 0);
        }
        protected ushort ReadBigEndianUShort(BinaryReader br)
        {
            byte[] bytes = br.ReadBytes(2); // Только 2 байта!
            if (BitConverter.IsLittleEndian)
                Array.Reverse(bytes);
            return BitConverter.ToUInt16(bytes, 0);
        }
    }
}
