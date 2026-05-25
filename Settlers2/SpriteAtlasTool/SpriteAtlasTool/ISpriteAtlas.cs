using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;

namespace SpriteAtlasTool
{
    public class SpriteRegion
    {
        public Rectangle Bounds;                // Для пакинга (в блоке 256×256)
        public Point Pivot;
        public Rectangle DisplayBounds;         // Для отображения в pictureBox1
        public Rectangle OriginalBounds;        // Реальные границы до паддинга
        public Point BlockOffset;               // Смещение внутри блока (например 256x256)
        public PointF UV_Min;                   // Минимальные UV
        public PointF UV_Max;                   // Максимальные UV
        public bool FlipX { get; set; }         // Отразить по горизонтали
        public bool FlipY { get; set; }         // Отразить по вертикали
        public string Name { get; set; }        // Имя для поиска
        public bool HasPivot
        {
            get { return Pivot.X >= 0 && Pivot.Y >= 0; }
        }
        public bool IsPacked { get; set; }
        public SpriteRegion(Rectangle bounds)
        {
            Bounds = bounds;
            OriginalBounds = bounds;
            Pivot = new Point(-1, -1);
            BlockOffset = new Point(0, 0);
            UV_Min = new PointF(0, 0);
            UV_Max = new PointF(1, 1);
            FlipX = false;
            FlipY = false;
            Name = "";
        }

        public override string ToString()
        {
            string baseName = string.Format("Sprite {0},{1} {2}x{3}",
            Bounds.X, Bounds.Y, Bounds.Width, Bounds.Height);

            // Добавляем информацию о трансформации в строковое представление
            if (!string.IsNullOrEmpty(Name))
                baseName = Name;

            if (FlipX && FlipY)
                baseName += " [XY]";
            else if (FlipX)
                baseName += " [X]";
            else if (FlipY)
                baseName += " [Y]";

            return baseName;
        }
        
        public virtual void CalculateUVFromRealCoordinates(int textureWidth, int textureHeight)
    {
        if (textureWidth <= 0 || textureHeight <= 0) return;

        // Рассчитываем базовые UV координаты
        float u0 = (float)OriginalBounds.Left / textureWidth;
        float v0 = (float)OriginalBounds.Top / textureHeight;
        float u1 = (float)OriginalBounds.Right / textureWidth;
        float v1 = (float)OriginalBounds.Bottom / textureHeight;

        // Применяем трансформации зеркалирования
        if (FlipX)
        {
            UV_Min.X = u1;
            UV_Max.X = u0;
        }
        else
        {
            UV_Min.X = u0;
            UV_Max.X = u1;
        }

        if (FlipY)
        {
            UV_Min.Y = v1;
            UV_Max.Y = v0;
        }
        else
        {
            UV_Min.Y = v0;
            UV_Max.Y = v1;
        }

        // Помечаем как упакованный если UV не стандартные
        IsPacked = !(Math.Abs(UV_Min.X) < 0.0001f &&
                     Math.Abs(UV_Min.Y) < 0.0001f &&
                     Math.Abs(UV_Max.X - 1.0f) < 0.0001f &&
                     Math.Abs(UV_Max.Y - 1.0f) < 0.0001f);

        BlockOffset = new Point(0, 0);
    }

    private CollisionInfo _collision;
    public CollisionInfo Collision
    {
        get
        {
            if (_collision == null)
                _collision = new CollisionInfo();
            return _collision;
        }
        set { _collision = value; }
    }
}
    

    public interface ISpriteAtlas
    {
        /// <summary>
        /// Имя атласа
        /// </summary>
        string Name { get; set; }

        /// <summary>
        /// Тип атласа
        /// </summary>
        AtlasType Type { get; }

        /// <summary>
        /// Список спрайтов в атласе
        /// </summary>
        List<SpriteRegion> Sprites { get; }

        /// <summary>
        /// Сохранить атлас в файл
        /// </summary>
        /// <param name="filePath">Путь к файлу</param>
        void SaveToFile(string filePath);

        /// <summary>
        /// Загрузить атлас из файла
        /// </summary>
        /// <param name="filePath">Путь к файлу</param>
        void LoadFromFile(string filePath);

        void PackToFixedSize(int blockSize = 256, int textureWidth = 1280, int textureHeight = 720);

        void CalculateUVFromRealCoordinates(int textureWidth, int textureHeight);
    }

    public enum AtlasType : byte
    {
        /// <summary>
        /// Одиночный спрайт
        /// </summary>
        SingleSprite = 0,

        /// <summary>
        /// Многоуровневый атлас
        /// </summary>
        MultiLevel = 1,

        /// <summary>
        /// Анимационный атлас
        /// </summary>
        Animation = 2
    }

    public class CollisionInfo
{
    public int Width { get; set; }   // Ширина коллайдера в тайлах
    public int Height { get; set; }  // Высота коллайдера в тайлах
    public int OffsetX { get; set; } // Смещение коллайдера по X (тайлы)
    public int OffsetY { get; set; } // Смещение коллайдера по Y (тайлы)
    public bool BlocksMovement { get; set; } // Блокирует ли движение
    public bool IsTrigger { get; set; }     // Триггер (не блокирует, но вызывает событие)
}
}
