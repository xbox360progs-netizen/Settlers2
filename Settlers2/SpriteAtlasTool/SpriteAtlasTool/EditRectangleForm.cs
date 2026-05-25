using System;
using System.Drawing;
using System.Windows.Forms;

namespace SpriteAtlasTool
{
    public partial class EditRectangleForm : Form
    {
        private Rectangle originalRectangle;
        public Rectangle ResultRectangle { get; private set; }
        public bool LiveUpdateEnabled { get; set; }

        // Событие для уведомления об изменениях
        public event Action<Rectangle> RectangleChanged;

        public EditRectangleForm(Rectangle initial)
        {
            InitializeComponent();

            // Инициализация свойства вручную
            LiveUpdateEnabled = false;

            // Сохраняем оригинальные значения
            originalRectangle = initial;

            // Установка диапазонов
            SetupNumericUpDown(numericUpDownX, 0, 10000, initial.X);
            SetupNumericUpDown(numericUpDownY, 0, 10000, initial.Y);
            SetupNumericUpDown(numericUpDownWidth, 1, 10000, initial.Width);
            SetupNumericUpDown(numericUpDownHeight, 1, 10000, initial.Height);

            // Подписываемся на изменения
            numericUpDownX.ValueChanged += NumericValueChanged;
            numericUpDownY.ValueChanged += NumericValueChanged;
            numericUpDownWidth.ValueChanged += NumericValueChanged;
            numericUpDownHeight.ValueChanged += NumericValueChanged;
        }

        private void SetupNumericUpDown(NumericUpDown control, decimal min, decimal max, decimal value)
        {
            control.Minimum = min;
            control.Maximum = max;
            if (value >= min && value <= max)
                control.Value = value;
            else
                control.Value = min;
        }

        private void NumericValueChanged(object sender, EventArgs e)
        {
            if (LiveUpdateEnabled)
            {
                Rectangle newRect = new Rectangle(
                    (int)numericUpDownX.Value,
                    (int)numericUpDownY.Value,
                    (int)numericUpDownWidth.Value,
                    (int)numericUpDownHeight.Value
                );

                // Используем старый синтаксис вместо ?.
                if (RectangleChanged != null)
                {
                    RectangleChanged(newRect);
                }
            }
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            ResultRectangle = new Rectangle(
                (int)numericUpDownX.Value,
                (int)numericUpDownY.Value,
                (int)numericUpDownWidth.Value,
                (int)numericUpDownHeight.Value
            );
            DialogResult = DialogResult.OK;
            Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            // Восстанавливаем оригинальные значения
            numericUpDownX.Value = originalRectangle.X;
            numericUpDownY.Value = originalRectangle.Y;
            numericUpDownWidth.Value = originalRectangle.Width;
            numericUpDownHeight.Value = originalRectangle.Height;

            // Уведомляем об отмене (старый синтаксис)
            if (RectangleChanged != null)
            {
                RectangleChanged(originalRectangle);
            }

            DialogResult = DialogResult.Cancel;
            Close();
        }

        // Метод для обновления значений извне (если нужно)
        public void UpdateValues(Rectangle rect)
        {
            numericUpDownX.Value = rect.X;
            numericUpDownY.Value = rect.Y;
            numericUpDownWidth.Value = rect.Width;
            numericUpDownHeight.Value = rect.Height;
        }
    }
}