using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;
using System.Drawing.Drawing2D;

namespace SpriteAtlasTool
{
    public partial class Form1 : Form
    {
        private Bitmap loadedImage;
        private ISpriteAtlas currentAtlas;
        private SpriteRegion selectedSprite = null;
        private Rectangle tempRect;
        private bool isDrawing = false;
        private bool isMoving = false;
        private Point moveOffset;
        private Point resizeStartPoint;
        private Rectangle originalBounds;
        private string loadedImagePath = "";
        private enum ResizeMode { None, Left, Right, Top, Bottom, TopLeft, TopRight, BottomLeft, BottomRight }
        private ResizeMode currentResizeMode = ResizeMode.None;
        private EditRectangleForm editForm = null;

        private float zoomFactor = 1.0f;
        private PointF offset = new PointF(0, 0);
        private const float zoomStep = 0.1f;

        private int currentAnimationFrame = 0;

        private Point previewMousePos = new Point(-1, -1);

        private float previewZoomFactor = 1.0f;
        private const float previewZoomStep = 0.1f;

        public Form1()
        {
            InitializeComponent();
            this.lstSprites.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.lstSprites_MouseDoubleClick);
            // Синхронизация меню с чекбоксами
            showGuidesToolStripMenuItem.Text = chkShowGuides.Checked ? "Hide Guides" : "Show Guides";
            fillUVAreaToolStripMenuItem.Text = chkFillUVArea.Checked ? "Don't Fill UV Area" : "Fill UV Area";
            showPivotGuidesToolStripMenuItem.Text = chkShowPivotGuides.Checked ? "Hide Pivot Guides" : "Show Pivot Guides";
            animationTimer.Interval = 100; // 10 FPS по умолчанию
            btnPlayAnimation.Visible = false;
            btnStopAnimation.Visible = false;
            btnStopAnimation.Enabled = false;
            btnCenterPivotAnimation.Visible = false;
            pictureBox1.SizeMode = PictureBoxSizeMode.Normal;
            pictureBox1.Cursor = Cursors.Cross;
            previewBox.SizeMode = PictureBoxSizeMode.Normal;
            previewBox.Dock = DockStyle.Fill;
            btnMoveUp.Visible = false;
            btnMoveDown.Visible = false;
            labelSpritesPerLevel.Visible = false;
            numericUpDownSpritesPerLevel.Visible = false;
            previewBox.MouseMove += previewBox_MouseMove;
            previewBox.Paint += previewBox_Paint;
            CreateNewAtlas(AtlasType.SingleSprite);
            UpdateUI();
        }

        private void btnPlayAnimation_Click(object sender, EventArgs e)
        {
            if (currentAtlas != null && currentAtlas.Type == AtlasType.Animation &&
                currentAtlas.Sprites.Count > 0)
            {
                AnimationAtlas animAtlas = currentAtlas as AnimationAtlas;
                if (animAtlas != null)
                {
                    // Устанавливаем FPS из настроек атласа
                    if (animAtlas.FrameRate > 0)
                    {
                        animationTimer.Interval = 1000 / animAtlas.FrameRate;
                    }

                    currentAnimationFrame = 0;
                    animationTimer.Start();
                    btnPlayAnimation.Enabled = false;
                    btnStopAnimation.Enabled = true;

                    // Выбираем первый спрайт
                    selectedSprite = animAtlas.Sprites[0];
                    lstSprites.SelectedIndex = 0;
                    UpdatePreview();
                    pictureBox1.Invalidate();
                }
            }
        }

        private void btnStopAnimation_Click(object sender, EventArgs e)
        {
            StopAnimation();
        }

        private void StopAnimation()
        {
            animationTimer.Stop();
            currentAnimationFrame = 0;
            btnPlayAnimation.Enabled = true;
            btnStopAnimation.Enabled = false;

            // Возвращаем отображение первого спрайта
            if (currentAtlas != null && currentAtlas.Sprites.Count > 0)
            {
                selectedSprite = currentAtlas.Sprites[0];
                if (lstSprites.Items.Count > 0)
                {
                    lstSprites.SelectedIndex = 0;
                }
                UpdatePreview();
                pictureBox1.Invalidate();
            }
        }

        private void animationTimer_Tick(object sender, EventArgs e)
        {
            if (currentAtlas != null && currentAtlas.Type == AtlasType.Animation)
            {
                AnimationAtlas animAtlas = currentAtlas as AnimationAtlas;
                if (animAtlas != null && animAtlas.Sprites.Count > 0)
                {
                    // Показываем текущий кадр
                    selectedSprite = animAtlas.Sprites[currentAnimationFrame];

                    // Обновляем выделение в списке
                    if (currentAnimationFrame < lstSprites.Items.Count)
                    {
                        lstSprites.SelectedIndex = currentAnimationFrame;
                    }

                    UpdatePreview();
                    pictureBox1.Invalidate();

                    // Переходим к следующему кадру
                    currentAnimationFrame++;

                    // Циклическое проигрывание
                    if (currentAnimationFrame >= animAtlas.Sprites.Count)
                    {
                        if (animAtlas.Loop)
                        {
                            currentAnimationFrame = 0; // Начинаем сначала
                        }
                        else
                        {
                            StopAnimation(); // Останавливаем, если не зациклено
                        }
                    }
                }
            }
        }

        private void btnAddGroup_Click(object sender, EventArgs e)
        {
            if (currentAtlas != null && currentAtlas.Type == AtlasType.MultiLevel)
            {
                MultiLevelAtlas multiAtlas = currentAtlas as MultiLevelAtlas;
                if (multiAtlas != null)
                {
                    // Создаем новую группу по имени из текстбокса
                    string groupName = txtGroupName.Text;
                    if (string.IsNullOrEmpty(groupName))
                        groupName = "Group" + multiAtlas.Groups.Count;

                    SpriteGroup group = multiAtlas.CreateGroup(groupName);
                    RefreshGroupsList();
                    RefreshSpriteList(); // ← ДОБАВИТЬ ЭТО
                    pictureBox1.Invalidate();
                }
            }
            else
            {
                MessageBox.Show("Выберите Multi-Level тип атласа!");
            }
        }

        private void btnAddToGroup_Click(object sender, EventArgs e)
        {
            // Русская версия отладки
            string debugInfo = "Атлас загружен: " + (currentAtlas != null ? "ДА" : "НЕТ") +
                              ", Тип: " + (currentAtlas != null ? currentAtlas.Type.ToString() : "НЕТ") +
                              ", Спрайт выбран: " + (selectedSprite != null ? "ДА" : "НЕТ") +
                              ", Группа выбрана: " + (lstGroups.SelectedItem != null ? "ДА" : "НЕТ");

            MessageBox.Show(debugInfo);

            if (currentAtlas != null && currentAtlas.Type == AtlasType.MultiLevel &&
                selectedSprite != null && lstGroups.SelectedItem != null)
            {
                MultiLevelAtlas multiAtlas = currentAtlas as MultiLevelAtlas;
                if (multiAtlas != null)
                {
                    SpriteGroup group = lstGroups.SelectedItem as SpriteGroup;
                    if (group != null)
                    {
                        multiAtlas.AddSpriteToGroup(selectedSprite, group);
                        RefreshGroupsList();
                        RefreshSpriteList();
                        pictureBox1.Invalidate();
                    }
                }
            }
            else
            {
                MessageBox.Show("Нужно: 1) Выбрать тип Multi-Level 2) Выбрать спрайт 3) Выбрать группу");
            }
        }

        private void btnRemoveGroup_Click(object sender, EventArgs e)
        {
            if (currentAtlas != null && currentAtlas.Type == AtlasType.MultiLevel)
            {
                MultiLevelAtlas multiAtlas = currentAtlas as MultiLevelAtlas;
                if (multiAtlas != null && lstGroups.SelectedItem != null)
                {
                    SpriteGroup group = lstGroups.SelectedItem as SpriteGroup;
                    if (group != null)
                    {
                        multiAtlas.RemoveGroup(group);
                        RefreshGroupsList();
                        pictureBox1.Invalidate();
                    }
                }
            }
        }

        private void lstGroups_SelectedIndexChanged(object sender, EventArgs e)
        {
            pictureBox1.Invalidate();
        }

        private void RefreshGroupsList()
        {
            lstGroups.Items.Clear();
            if (currentAtlas != null && currentAtlas.Type == AtlasType.MultiLevel)
            {
                MultiLevelAtlas multiAtlas = currentAtlas as MultiLevelAtlas;
                if (multiAtlas != null)
                {
                    foreach (SpriteGroup group in multiAtlas.Groups)
                    {
                        lstGroups.Items.Add(group);
                    }
                }
            }
        }

        private void comboBoxAtlasType_SelectedIndexChanged(object sender, EventArgs e)
        {
            AtlasType selectedType = (AtlasType)comboBoxAtlasType.SelectedIndex;

            // Всегда показываем GroupBox
            groupBoxAtlasType.Visible = true;

            // Показываем/скрываем элементы для MultiLevel
            if (selectedType == AtlasType.MultiLevel)
            {
                labelSpritesPerLevel.Visible = true;
                numericUpDownSpritesPerLevel.Visible = true;
                lstGroups.Visible = true;
                txtGroupName.Visible = true;
                btnAddGroup.Visible = true;
                btnRemoveGroup.Visible = true;
                btnAddToGroup.Visible = true;
                btnMoveUp.Visible = false;
                btnMoveDown.Visible = false;
                // Скрываем элементы анимации
                btnPlayAnimation.Visible = false;
                btnStopAnimation.Visible = false;
            }
            else if (selectedType == AtlasType.Animation)
            {
                // Скрываем элементы MultiLevel
                labelSpritesPerLevel.Visible = false;
                numericUpDownSpritesPerLevel.Visible = false;
                lstGroups.Visible = false;
                txtGroupName.Visible = false;
                btnAddGroup.Visible = false;
                btnRemoveGroup.Visible = false;
                btnAddToGroup.Visible = false;
                btnMoveUp.Visible = true;
                btnMoveDown.Visible = true;
                // Показываем элементы анимации
                btnPlayAnimation.Visible = true;
                btnStopAnimation.Visible = true;
                btnCenterPivotAnimation.Visible = true;

                btnPlayAnimation.BringToFront();
                btnStopAnimation.BringToFront();
            }
            else
            {
                // Single Sprite - скрываем все дополнительные элементы
                labelSpritesPerLevel.Visible = false;
                numericUpDownSpritesPerLevel.Visible = false;
                lstGroups.Visible = false;
                txtGroupName.Visible = false;
                btnAddGroup.Visible = false;
                btnRemoveGroup.Visible = false;
                btnAddToGroup.Visible = false;
                btnPlayAnimation.Visible = false;
                btnStopAnimation.Visible = false;
                btnCenterPivotAnimation.Visible = false;
                btnMoveUp.Visible = false;
                btnMoveDown.Visible = false;
            }
            UpdateUI();
            // Создаем новый атлас выбранного типа
            CreateNewAtlas(selectedType);
            RefreshSpriteList();
        }


        private void numericUpDownSpritesPerLevel_ValueChanged(object sender, EventArgs e)
        {
            if (currentAtlas != null && currentAtlas.Type == AtlasType.MultiLevel)
            {
                MultiLevelAtlas multiAtlas = currentAtlas as MultiLevelAtlas;
                if (multiAtlas != null)
                {
                    multiAtlas.SpritesPerLevel = (int)numericUpDownSpritesPerLevel.Value;
                }
            }
        }

        private void CreateNewAtlas(AtlasType type)
        {
            switch (type)
            {
                case AtlasType.SingleSprite:
                    currentAtlas = new SingleSpriteAtlas();
                    break;
                case AtlasType.MultiLevel:
                    MultiLevelAtlas multiAtlas = new MultiLevelAtlas();
                    multiAtlas.SpritesPerLevel = (int)numericUpDownSpritesPerLevel.Value;
                    currentAtlas = multiAtlas;
                    break;
                case AtlasType.Animation:
                    currentAtlas = new AnimationAtlas();
                    break;
            }

            // Очищаем список спрайтов
            if (currentAtlas != null)
            {
                currentAtlas.Sprites.Clear();
            }

            selectedSprite = null;
            pictureBox1.Invalidate();
            RefreshSpriteList();
            RefreshGroupsList();
        }

        private void RefreshSpriteList()
        {
            int currentIndex = lstSprites.SelectedIndex;
            lstSprites.Items.Clear();

            if (currentAtlas != null)
            {
                AutoGenerateSpriteNames();
                if (currentAtlas.Type == AtlasType.MultiLevel)
                {
                    MultiLevelAtlas multiAtlas = currentAtlas as MultiLevelAtlas;
                    // Для MultiLevel отображаем спрайты с названиями групп и детальной информацией
                    for (int i = 0; i < currentAtlas.Sprites.Count; i++)
                    {
                        SpriteRegion sprite = currentAtlas.Sprites[i];
                        string displayName = sprite.Name;

                        if (sprite.FlipX && sprite.FlipY)
                    displayName += " [XY]";
                else if (sprite.FlipX)
                    displayName += " [X]";
                else if (sprite.FlipY)
                    displayName += " [Y]";

                        // Добавляем координаты и размеры
                        displayName += string.Format(" [{0},{1} {2}x{3}]",
                            sprite.DisplayBounds.X, sprite.DisplayBounds.Y,
                            sprite.DisplayBounds.Width, sprite.DisplayBounds.Height);

                        // Добавляем пивот если есть
                        if (sprite.HasPivot)
                        {
                            displayName += string.Format(" P({0},{1})", sprite.Pivot.X, sprite.Pivot.Y);
                        }

                        lstSprites.Items.Add(displayName);
                    }
                }
                else
                {
                    // Для других типов - обычное отображение с деталями
                    foreach (SpriteRegion sprite in currentAtlas.Sprites)
                    {
                        string displayName = sprite.Name;

                        if (sprite.FlipX && sprite.FlipY)
                    displayName += " [XY]";
                else if (sprite.FlipX)
                    displayName += " [X]";
                else if (sprite.FlipY)
                    displayName += " [Y]";

                        // Добавляем координаты и размеры
                        displayName += string.Format(" [{0},{1} {2}x{3}]",
                            sprite.DisplayBounds.X, sprite.DisplayBounds.Y,
                            sprite.DisplayBounds.Width, sprite.DisplayBounds.Height);

                        // Добавляем пивот если есть
                        if (sprite.HasPivot)
                        {
                            displayName += string.Format(" P({0},{1})", sprite.Pivot.X, sprite.Pivot.Y);
                        }

                        lstSprites.Items.Add(displayName);
                    }
                }
            }

            // Восстанавливаем выделение или выбираем первый
            if (currentAtlas != null && currentAtlas.Sprites.Count > 0)
            {
                if (currentIndex >= 0 && currentIndex < currentAtlas.Sprites.Count)
                {
                    lstSprites.SelectedIndex = currentIndex;
                }
                else
                {
                    lstSprites.SelectedIndex = 0;
                    selectedSprite = currentAtlas.Sprites[0];
                }
                UpdatePreview();
            }
            else
            {
                selectedSprite = null;
                previewBox.Image = null;
                if (previewBox != null)
                    previewBox.Invalidate();
            }
            if (pictureBox1 != null)
                pictureBox1.Invalidate();
            RefreshGroupsList();
        }

        private AtlasType DetectAtlasTypeFromFile(string filePath)
        {
            try
            {
                using (FileStream fs = new FileStream(filePath, FileMode.Open))
                using (BinaryReader br = new BinaryReader(fs))
                {
                    if (fs.Length < 5) // Минимум 2+2+1 байт
                    {
                        MessageBox.Show("File too small: " + fs.Length.ToString());
                        return AtlasType.SingleSprite;
                    }

                    // Читаем версию (ushort - 2 байта)
                    ushort version = ReadBigEndianUShort(br);

                    // Читаем размер заголовка (ushort - 2 байта)
                    ushort headerSize = ReadBigEndianUShort(br);

                    // Читаем тип атласа (byte - 1 байт)
                    byte typeByte = br.ReadByte();

                    if (typeByte <= 2)
                    {
                        AtlasType detectedType = (AtlasType)typeByte;
                        return detectedType;
                    }
                    else
                    {
                        MessageBox.Show("Invalid type byte: " + typeByte.ToString());
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error in detection: " + ex.Message);
            }

            return AtlasType.SingleSprite;
        }

        private void DrawGroupIndicators(PaintEventArgs e)
        {
            if (currentAtlas != null && currentAtlas.Type == AtlasType.MultiLevel)
            {
                MultiLevelAtlas multiAtlas = currentAtlas as MultiLevelAtlas;
                if (multiAtlas != null)
                {
                    // Отрисовка групп
                    for (int groupIndex = 0; groupIndex < multiAtlas.Groups.Count; groupIndex++)
                    {
                        SpriteGroup group = multiAtlas.Groups[groupIndex];

                        foreach (SpriteRegion sprite in group.Sprites)
                        {
                            // Рисуем имя группы над спрайтом (черным цветом и внутри прямоугольника)
                            using (Font font = new Font("Arial", 8))
                            using (Brush brush = new SolidBrush(Color.Black)) // ← Черный цвет
                            using (StringFormat format = new StringFormat())
                            {
                                format.Alignment = StringAlignment.Center; // Центрирование по горизонтали

                                string groupText = sprite.Name ?? "";
                                if (string.IsNullOrEmpty(groupText))
                                {
                                    groupText = group.Name;
                                    int spriteIndexInGroup = group.Sprites.IndexOf(sprite);
                                    if (spriteIndexInGroup >= 0)
                                    {
                                        groupText += "_" + (spriteIndexInGroup + 1).ToString("D2");
                                    }
                                }

                                // Позиционируем текст внутри прямоугольника спрайта
                                float textX = (sprite.DisplayBounds.X + sprite.DisplayBounds.Width / 2) * zoomFactor;
                                float textY = (sprite.DisplayBounds.Y + 5) * zoomFactor; // 5 пикселей от верха

                                // Рисуем фон для текста чтобы он был лучше виден
                                SizeF textSize = e.Graphics.MeasureString(groupText, font);
                                RectangleF textBackground = new RectangleF(
                                    textX - textSize.Width / 2 - 2,
                                    textY - 2,
                                    textSize.Width + 4,
                                    textSize.Height + 2
                                );

                                using (Brush bgBrush = new SolidBrush(Color.FromArgb(180, Color.Yellow)))
                                {
                                    e.Graphics.FillRectangle(bgBrush, textBackground);
                                }

                                e.Graphics.DrawString(groupText, font, brush, textX, textY, format);
                            }
                        }
                    }
                }
            }
        }

        private void btnCenterPivot_Click(object sender, EventArgs e)
        {
            if (selectedSprite != null)
            {
                // Центр спрайта
                int centerX = selectedSprite.Bounds.Width / 2;
                int centerY = selectedSprite.Bounds.Height / 2;
                selectedSprite.Pivot = new Point(centerX, centerY);
                pictureBox1.Invalidate();
                UpdatePreview();
                lblInfo.Text = string.Format("Pivot centered: {0}, {1}", centerX, centerY);
            }
            else
            {
                MessageBox.Show("Select a sprite first.");
            }
        }

        private void btnSaveDefaultBin_Click(object sender, EventArgs e)
        {
            if (loadedImage == null || currentAtlas == null || currentAtlas.Sprites.Count == 0)
            {
                MessageBox.Show("No sprites defined or no image loaded.");
                return;
            }

            if (string.IsNullOrEmpty(loadedImagePath))
            {
                MessageBox.Show("Original image path is not available.");
                return;
            }

            try
            {
                string directory = Path.GetDirectoryName(loadedImagePath);
                string fileNameWithoutExtension = Path.GetFileNameWithoutExtension(loadedImagePath);

                // Проверим, что имя файла допустимо
                foreach (char c in Path.GetInvalidFileNameChars())
                {
                    fileNameWithoutExtension = fileNameWithoutExtension.Replace(c, '_');
                }

                string defaultBinPath = Path.Combine(directory, fileNameWithoutExtension + ".bin");

                // Проверим, что путь допустим
                if (string.IsNullOrEmpty(defaultBinPath) || defaultBinPath.Length > 260)
                {
                    MessageBox.Show("Generated path is invalid or too long.");
                    return;
                }

                currentAtlas.SaveToFile(defaultBinPath);
                MessageBox.Show(string.Format("Saved as {0}\nGenerated {1} sprites", defaultBinPath, currentAtlas.Sprites.Count));
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error saving file:\n" + ex.Message);
            }
        }

        private void buttonLoadBin_Click(object sender, EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Filter = "Binary Files (*.bin)|*.bin";
            if (ofd.ShowDialog() == DialogResult.OK)
            {
                try
                {
                    // Здесь нужно определить тип атласа по файлу
                    AtlasType detectedType = DetectAtlasTypeFromFile(ofd.FileName);

                    // Пересоздаем атлас правильного типа
                    CreateNewAtlas(detectedType);

                    // Устанавливаем правильный тип в комбо боксе
                    comboBoxAtlasType.SelectedIndex = (int)detectedType;

                    // Загружаем данные
                    currentAtlas.LoadFromFile(ofd.FileName);

                    if (currentAtlas != null)
                    {
                        foreach (var sprite in currentAtlas.Sprites)
                        {
                            if (sprite.DisplayBounds.Width == 0 || sprite.DisplayBounds.Height == 0)
                            {
                                sprite.DisplayBounds = sprite.OriginalBounds;
                            }
                        }
                    }

                    RefreshSpriteList();

                    pictureBox1.Invalidate();

                    lblInfo.Text = string.Format("Loaded {0} sprites from .bin", currentAtlas.Sprites.Count);
                    MessageBox.Show(string.Format("Successfully loaded {0} sprites", currentAtlas.Sprites.Count));
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Error loading .bin file:\n" + ex.Message);
                }
            }
        }

        private void buttonLoad_Click(object sender, EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Filter = "Image Files|*.png;*.jpg;*.bmp";
            if (ofd.ShowDialog() == DialogResult.OK)
            {
                try
                {
                    loadedImage = new Bitmap(ofd.FileName);
                    // Рисуем изображение вручную в обработчике Paint, чтобы не было двойного масштабирования
                    pictureBox1.Size = new Size((int)(loadedImage.Width * zoomFactor), (int)(loadedImage.Height * zoomFactor));
                    if (currentAtlas != null)
                    {
                        currentAtlas.Sprites.Clear();
                    }
                    lstSprites.Items.Clear();
                    lblInfo.Text = string.Format("Loaded: {0}x{1}", loadedImage.Width, loadedImage.Height);
                    previewBox.Image = null;

                    // Сохраняем путь к изображению
                    loadedImagePath = ofd.FileName;
                    lblFileName.Text = Path.GetFileName(loadedImagePath);
                    pictureBox1.Invalidate();
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Error loading image: " + ex.Message);
                }
            }
        }

        private void pictureBox1_MouseDown(object sender, MouseEventArgs e)
        {
            // Корректируем координаты с учетом зума
            Point correctedPoint = CorrectMousePosition(e.Location);

            if (currentAtlas == null) return;

            if (selectedSprite != null)
            {
                Rectangle bounds = selectedSprite.Bounds;
                int margin = 5;

                // Корректируем координаты для проверки
                Rectangle left = new Rectangle(bounds.Left - margin, bounds.Top, margin * 2, bounds.Height);
                Rectangle right = new Rectangle(bounds.Right - margin, bounds.Top, margin * 2, bounds.Height);
                Rectangle top = new Rectangle(bounds.Left, bounds.Top - margin, bounds.Width, margin * 2);
                Rectangle bottom = new Rectangle(bounds.Left, bounds.Bottom - margin, bounds.Width, margin * 2);

                if (left.Contains(correctedPoint) && top.Contains(correctedPoint)) currentResizeMode = ResizeMode.TopLeft;
                else if (right.Contains(correctedPoint) && top.Contains(correctedPoint)) currentResizeMode = ResizeMode.TopRight;
                else if (left.Contains(correctedPoint) && bottom.Contains(correctedPoint)) currentResizeMode = ResizeMode.BottomLeft;
                else if (right.Contains(correctedPoint) && bottom.Contains(correctedPoint)) currentResizeMode = ResizeMode.BottomRight;
                else if (left.Contains(correctedPoint)) currentResizeMode = ResizeMode.Left;
                else if (right.Contains(correctedPoint)) currentResizeMode = ResizeMode.Right;
                else if (top.Contains(correctedPoint)) currentResizeMode = ResizeMode.Top;
                else if (bottom.Contains(correctedPoint)) currentResizeMode = ResizeMode.Bottom;
                else if (bounds.Contains(correctedPoint))
                {
                    currentResizeMode = ResizeMode.None;
                    isMoving = true;
                    moveOffset = new Point(correctedPoint.X - bounds.X, correctedPoint.Y - bounds.Y);
                    return;
                }

                if (currentResizeMode != ResizeMode.None)
                {
                    resizeStartPoint = correctedPoint;
                    originalBounds = selectedSprite.Bounds;
                    return;
                }
            }

            isDrawing = true;
            tempRect = new Rectangle(correctedPoint.X, correctedPoint.Y, 0, 0);
        }

        private void pictureBox1_MouseMove(object sender, MouseEventArgs e)
        {
            // Корректируем координаты с учетом зума
            Point correctedPoint = CorrectMousePosition(e.Location);

            if (currentAtlas == null) return;

            if (isMoving && selectedSprite != null)
            {
                selectedSprite.Bounds = new Rectangle(
                    correctedPoint.X - moveOffset.X,
                    correctedPoint.Y - moveOffset.Y,
                    selectedSprite.Bounds.Width,
                    selectedSprite.Bounds.Height
                );
                selectedSprite.DisplayBounds = selectedSprite.Bounds;
                selectedSprite.OriginalBounds = new Rectangle(
            selectedSprite.Bounds.X,
            selectedSprite.Bounds.Y,
            selectedSprite.Bounds.Width,
            selectedSprite.Bounds.Height
        );
                pictureBox1.Invalidate();
                UpdatePreview();
                previewBox.Invalidate();
                UpdateSpriteListItem(selectedSprite);
                RefreshSpriteList();
                return;
            }

            if (currentResizeMode != ResizeMode.None && selectedSprite != null)
            {
                Rectangle r = originalBounds;
                int dx = correctedPoint.X - resizeStartPoint.X;
                int dy = correctedPoint.Y - resizeStartPoint.Y;

                switch (currentResizeMode)
                {
                    case ResizeMode.Left:
                        r.X += dx;
                        r.Width -= dx;
                        break;
                    case ResizeMode.Right:
                        r.Width += dx;
                        break;
                    case ResizeMode.Top:
                        r.Y += dy;
                        r.Height -= dy;
                        break;
                    case ResizeMode.Bottom:
                        r.Height += dy;
                        break;
                    case ResizeMode.TopLeft:
                        r.X += dx;
                        r.Y += dy;
                        r.Width -= dx;
                        r.Height -= dy;
                        break;
                    case ResizeMode.TopRight:
                        r.Y += dy;
                        r.Width += dx;
                        r.Height -= dy;
                        break;
                    case ResizeMode.BottomLeft:
                        r.X += dx;
                        r.Width -= dx;
                        r.Height += dy;
                        break;
                    case ResizeMode.BottomRight:
                        r.Width += dx;
                        r.Height += dy;
                        break;
                }

                if (r.Width > 5 && r.Height > 5)
                {
                    selectedSprite.Bounds = r;
                    selectedSprite.DisplayBounds = r;
                    pictureBox1.Invalidate();
                    UpdatePreview();
                    previewBox.Invalidate();
                    UpdateSpriteListItem(selectedSprite);
                    RefreshSpriteList();
                }
                return;
            }

            if (isDrawing)
            {
                tempRect.Width = correctedPoint.X - tempRect.X;
                tempRect.Height = correctedPoint.Y - tempRect.Y;
                pictureBox1.Invalidate();
                previewBox.Invalidate();
            }
        }

        private void pictureBox1_MouseUp(object sender, MouseEventArgs e)
        {
            // Корректируем координаты с учетом зума
            Point correctedPoint = CorrectMousePosition(e.Location);

            if (isMoving)
            {
                isMoving = false;
                return;
            }

            if (currentResizeMode != ResizeMode.None)
            {
                currentResizeMode = ResizeMode.None;
                return;
            }

            if (isDrawing && currentAtlas != null)
            {
                isDrawing = false;
                if (Math.Abs(tempRect.Width) > 5 && Math.Abs(tempRect.Height) > 5)
                {
                    int x = Math.Min(tempRect.X, tempRect.Right);
                    int y = Math.Min(tempRect.Y, tempRect.Bottom);
                    int w = Math.Abs(tempRect.Width);
                    int h = Math.Abs(tempRect.Height);
                    Rectangle rect = new Rectangle(x, y, w, h);

                    SpriteRegion sprite = new SpriteRegion(rect);
                    sprite.DisplayBounds = rect;
                    sprite.OriginalBounds = rect;
                    currentAtlas.Sprites.Add(sprite);
                    RefreshSpriteList();
                    UpdatePreview();
                    previewBox.Invalidate();
                }
                pictureBox1.Invalidate();
            }
        }

        protected override void OnMouseWheel(MouseEventArgs e)
        {
            if (Control.ModifierKeys == Keys.Control)
            {
                Point clientPos = pictureBox1.PointToClient(Cursor.Position);
                if (pictureBox1.ClientRectangle.Contains(clientPos))
                {
                    if (e.Delta > 0)
                        btnZoomIn_Click(null, e);
                    else if (e.Delta < 0)
                        btnZoomOut_Click(null, e);
                    ((HandledMouseEventArgs)e).Handled = true;
                    return;
                }
            }
            base.OnMouseWheel(e);
        }

        private void pictureBox1_MouseClick(object sender, MouseEventArgs e)
        {
            // Корректируем координаты с учетом зума
            Point correctedPoint = CorrectMousePosition(e.Location);

            if (e.Button == MouseButtons.Right)
            {
                // Правая кнопка - выбираем спрайт под курсором
                if (currentAtlas != null)
                {
                    bool found = false;
                    foreach (SpriteRegion sprite in currentAtlas.Sprites)
                    {
                        if (sprite.Bounds.Contains(correctedPoint))
                        {
                            selectedSprite = sprite;
                            int index = currentAtlas.Sprites.IndexOf(sprite);
                            if (index >= 0)
                                lstSprites.SelectedIndex = index;
                            pictureBox1.Invalidate();
                            previewBox.Invalidate();
                            UpdatePreview();
                            lblInfo.Text = string.Format("Selected: {0}", sprite.Name ?? sprite.ToString());
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        selectedSprite = null;
                        pictureBox1.Invalidate();
                        previewBox.Image = null;
                    }
                }
                return;
            }

            // Левая кнопка - установка pivot
            if (selectedSprite != null)
            {
                if (selectedSprite.Bounds.Contains(correctedPoint))
                {
                    int localX = correctedPoint.X - selectedSprite.Bounds.X;
                    int localY = correctedPoint.Y - selectedSprite.Bounds.Y;

                    // Проверка границ
                    localX = Math.Max(0, Math.Min(localX, selectedSprite.Bounds.Width - 1));
                    localY = Math.Max(0, Math.Min(localY, selectedSprite.Bounds.Height - 1));

                    selectedSprite.Pivot = new Point(localX, localY);
                    pictureBox1.Invalidate();
                    previewBox.Invalidate();
                    lblInfo.Text = string.Format("Pivot set: {0}, {1}", localX, localY);
                    RefreshSpriteList();
                    UpdatePreview();
                    previewBox.Invalidate();
                }
            }
            else
            {
                if (currentAtlas != null)
                {
                    bool spriteSelected = false;
                    foreach (SpriteRegion sprite in currentAtlas.Sprites)
                    {
                        if (sprite.Bounds.Contains(correctedPoint))
                        {
                            selectedSprite = sprite;
                            lstSprites.SelectedItem = sprite;
                            int localX = correctedPoint.X - sprite.Bounds.X;
                            int localY = correctedPoint.Y - sprite.Bounds.Y;
                            sprite.Pivot = new Point(localX, localY);
                            pictureBox1.Invalidate();
                            lblInfo.Text = string.Format("Pivot set: {0}, {1}", localX, localY);
                            RefreshSpriteList();
                            UpdatePreview();
                            spriteSelected = true;
                            break;
                        }
                    }

                    // Если ни один спрайт не выбран, сбрасываем выделение
                    if (!spriteSelected)
                    {
                        selectedSprite = null;
                        pictureBox1.Invalidate(); // Перерисовываем для обновления цвета
                        previewBox.Image = null;
                    }
                }
            }
        }

        private Point CorrectMousePosition(Point mousePoint)
        {
            if (zoomFactor != 1.0f && loadedImage != null)
            {
                // Корректируем координаты с учетом зума
                float correctedX = mousePoint.X / zoomFactor;
                float correctedY = mousePoint.Y / zoomFactor;
                return new Point((int)correctedX, (int)correctedY);
            }
            return mousePoint;
        }

        private void EditForm_RectangleChanged(Rectangle newRect)
        {
            if (selectedSprite != null)
            {
                selectedSprite.Bounds = newRect;
                selectedSprite.DisplayBounds = newRect;
                pictureBox1.Invalidate();
                UpdatePreview();
                previewBox.Invalidate();

                //чтобы убедиться, что метод вызывается:
                // lblInfo.Text = string.Format("Rect changed: {0},{1} {2}x{3}", 
                //     newRect.X, newRect.Y, newRect.Width, newRect.Height);
            }
        }

        private void btnEditRect_Click(object sender, EventArgs e)
        {
            if (selectedSprite != null)
            {
                // Создаём форму редактирования
                editForm = new EditRectangleForm(selectedSprite.Bounds);
                editForm.LiveUpdateEnabled = true;

                // Убедитесь, что подписка есть:
                editForm.RectangleChanged += EditForm_RectangleChanged;

                // Показываем как модальное окно
                if (editForm.ShowDialog() == DialogResult.OK)
                {
                    selectedSprite.Bounds = editForm.ResultRectangle;
                    selectedSprite.OriginalBounds = editForm.ResultRectangle;
                    selectedSprite.DisplayBounds = editForm.ResultRectangle;
                    pictureBox1.Invalidate();
                    UpdatePreview();
                    RefreshSpriteList();
                    previewBox.Invalidate();
                }
                else
                {
                    // Отмена — восстановили прямоугольник
                    selectedSprite.DisplayBounds = selectedSprite.Bounds;
                    pictureBox1.Invalidate();
                    UpdatePreview();
                    RefreshSpriteList();
                    previewBox.Invalidate();
                }

                editForm = null;
            }
            else
            {
                MessageBox.Show("Select a sprite first.");
            }
        }

        private void pictureBox1_Paint(object sender, PaintEventArgs e)
        {
            if (loadedImage != null)
            {
                // Настройки рендеринга для стабильного пиксель-арта
                e.Graphics.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.NearestNeighbor;
                e.Graphics.PixelOffsetMode = System.Drawing.Drawing2D.PixelOffsetMode.Half;
                e.Graphics.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.None;

                // При зуме рисуем изображение соответствующего размера
                Rectangle srcRect = new Rectangle(0, 0, loadedImage.Width, loadedImage.Height);
                Rectangle destRect = new Rectangle(0, 0,
                    (int)(loadedImage.Width * zoomFactor),
                    (int)(loadedImage.Height * zoomFactor));
                e.Graphics.DrawImage(loadedImage, destRect, srcRect, GraphicsUnit.Pixel);
            }

            if (currentAtlas != null)
            {
                // Отладка: показываем количество спрайтов
                // using (Brush debugBrush = new SolidBrush(Color.White))
                // using (Font debugFont = new Font("Arial", 12))
                // {
                //     e.Graphics.DrawString("Sprites: " + currentAtlas.Sprites.Count, debugFont, debugBrush, 10, 10);
                // }

                foreach (SpriteRegion sprite in currentAtlas.Sprites)
                {
                    // Отладка: показываем координаты каждого спрайта
                    // Console.WriteLine($"Sprite: {sprite.DisplayBounds.X}, {sprite.DisplayBounds.Y}");

                    // Используем DisplayBounds для отображения
                    Rectangle scaledBounds = new Rectangle(
                        (int)(sprite.DisplayBounds.X * zoomFactor),
                        (int)(sprite.DisplayBounds.Y * zoomFactor),
                        (int)(sprite.DisplayBounds.Width * zoomFactor),
                        (int)(sprite.DisplayBounds.Height * zoomFactor)
                    );

                    // Выделенный спрайт - красный, остальные - синие
                    using (Pen pen = sprite == selectedSprite ? new Pen(Color.Red, 3.0f) : new Pen(Color.Blue, 2.0f))
                    {
                        e.Graphics.DrawRectangle(pen, scaledBounds);
                    }

                    // Pivot точка
                    if (sprite.HasPivot)
                    {
                        using (Brush brush = new SolidBrush(Color.Red))
                        {
                            float px = (sprite.DisplayBounds.X + sprite.Pivot.X) * zoomFactor;
                            float py = (sprite.DisplayBounds.Y + sprite.Pivot.Y) * zoomFactor;
                            float size = 4.0f;
                            e.Graphics.FillEllipse(brush, px - size / 2, py - size / 2, size, size);
                        }
                    }
                }
            }

            if (isDrawing)
            {
                using (Pen pen = new Pen(Color.Green, 1.0f))
                {
                    pen.DashStyle = System.Drawing.Drawing2D.DashStyle.Dash;
                    Rectangle scaledRect = new Rectangle(
                        (int)(tempRect.X * zoomFactor),
                        (int)(tempRect.Y * zoomFactor),
                        (int)(tempRect.Width * zoomFactor),
                        (int)(tempRect.Height * zoomFactor)
                    );
                    e.Graphics.DrawRectangle(pen, scaledRect);
                }
            }
            DrawGroupIndicators(e);

            if (currentAtlas != null && currentAtlas.Type == AtlasType.Animation &&
                selectedSprite != null && selectedSprite.HasPivot)
            {
                Point pivot = selectedSprite.Pivot;
                Point spriteLocation = selectedSprite.Bounds.Location;

                float centerX = (spriteLocation.X + pivot.X) * zoomFactor;
                float centerY = (spriteLocation.Y + pivot.Y) * zoomFactor;

                // Рисуем крестик в центре pivot точки
                using (Pen centerPen = new Pen(Color.Magenta, 2.0f))
                {
                    // Вертикальная линия
                    e.Graphics.DrawLine(centerPen, centerX, centerY - 10, centerX, centerY + 10);
                    // Горизонтальная линия
                    e.Graphics.DrawLine(centerPen, centerX - 10, centerY, centerX + 10, centerY);
                }
            }
        }

        private void UpdateUI()
        {
            bool hasSprite = selectedSprite != null;
            bool isSingleSprite = currentAtlas != null && currentAtlas.Type == AtlasType.SingleSprite;
            bool isMultiLevel = currentAtlas != null && currentAtlas.Type == AtlasType.MultiLevel;

            // Кнопка редактирования коллайдера для SingleSprite и MultiLevel
            btnEditCollision.Visible = hasSprite && (isSingleSprite || isMultiLevel);
            btnEditCollision.Enabled = hasSprite && (isSingleSprite || isMultiLevel);

            // Кнопка редактирования NodeWeight для SingleSprite и MultiLevel
            btnEditNodeWeight.Visible = hasSprite && (isSingleSprite || isMultiLevel);
            btnEditNodeWeight.Enabled = hasSprite && (isSingleSprite || isMultiLevel);

            // btnEditCollision.Visible = hasSprite && isSingleSprite;
        }

        private void lstSprites_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (lstSprites.SelectedIndex >= 0 && lstSprites.SelectedIndex < currentAtlas.Sprites.Count)
            {
                selectedSprite = currentAtlas.Sprites[lstSprites.SelectedIndex];
                UpdatePreview();
                pictureBox1.Invalidate();
                previewBox.Invalidate();
                lblInfo.Text = string.Format("Selected sprite: {0}", selectedSprite.ToString());
            }
            else
            {
                selectedSprite = null;
                previewBox.Image = null;
                pictureBox1.Invalidate();
                previewBox.Invalidate();
                lblInfo.Text = "No sprite selected";
            }
            UpdateUI();
        }

        private void UpdatePreview()
        {
            if (selectedSprite != null && loadedImage != null)
            {
                Rectangle sourceBounds;

                // Определяем, какие границы использовать для отображения
                if (selectedSprite.IsPacked)
                {
                    sourceBounds = selectedSprite.OriginalBounds;
                }
                else
                {
                    sourceBounds = selectedSprite.Bounds;
                }

                if (sourceBounds.Width > 0 && sourceBounds.Height > 0)
                {
                    // Создаём bitmap размером preview box
                    int canvasWidth = Math.Max(1, previewBox.Width);
                    int canvasHeight = Math.Max(1, previewBox.Height);

                    Bitmap bmp = new Bitmap(canvasWidth, canvasHeight);
                    using (Graphics g = Graphics.FromImage(bmp))
                    {
                        // Заливаем фон
                        g.Clear(Color.FromArgb(40, 40, 40));

                        // Настройки рендеринга
                        g.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.NearestNeighbor;
                        g.PixelOffsetMode = System.Drawing.Drawing2D.PixelOffsetMode.Half;
                        g.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.None;

                        // Центрируем спрайт в preview
                        float scaleX = (float)(canvasWidth - 20) / sourceBounds.Width;
                        float scaleY = (float)(canvasHeight - 20) / sourceBounds.Height;
                        float scale = Math.Min(scaleX, scaleY);
                        if (scale > 5.0f) scale = 5.0f; // Ограничение масштаба

                        float spriteWidth = sourceBounds.Width * scale;
                        float spriteHeight = sourceBounds.Height * scale;
                        float spritePosX = (canvasWidth - spriteWidth) / 2f;
                        float spritePosY = (canvasHeight - spriteHeight) / 2f;

                        // Рисуем спрайт с учетом трансформаций
                        if (selectedSprite.FlipX || selectedSprite.FlipY)
                        {
                            // Сохраняем текущее состояние графики
                            System.Drawing.Drawing2D.Matrix oldMatrix = g.Transform.Clone();

                            try
                            {
                                // Устанавливаем точку трансформации в центр спрайта
                                g.TranslateTransform(spritePosX + spriteWidth / 2, spritePosY + spriteHeight / 2);

                                // Применяем зеркалирование
                                float flipX = selectedSprite.FlipX ? -1f : 1f;
                                float flipY = selectedSprite.FlipY ? -1f : 1f;
                                g.ScaleTransform(flipX, flipY);

                                // Возвращаем обратно
                                g.TranslateTransform(-(spritePosX + spriteWidth / 2), -(spritePosY + spriteHeight / 2));

                                // Рисуем спрайт
                                g.DrawImage(loadedImage,
                                    new RectangleF(spritePosX, spritePosY, spriteWidth, spriteHeight),
                                    sourceBounds,
                                    GraphicsUnit.Pixel);

                                // Восстанавливаем оригинальное состояние
                                g.Transform = oldMatrix;
                            }
                            catch
                            {
                                // В случае ошибки восстанавливаем матрицу и рисуем без трансформации
                                g.Transform = oldMatrix;
                                g.DrawImage(loadedImage,
                                    new RectangleF(spritePosX, spritePosY, spriteWidth, spriteHeight),
                                    sourceBounds,
                                    GraphicsUnit.Pixel);
                            }
                        }
                        else
                        {
                            // Без трансформаций рисуем напрямую
                            g.DrawImage(loadedImage,
                                new RectangleF(spritePosX, spritePosY, spriteWidth, spriteHeight),
                                sourceBounds,
                                GraphicsUnit.Pixel);
                        }

                        // Если есть pivot - рисуем маркер
                        if (selectedSprite.HasPivot && chkShowPivotGuides.Checked)
                        {
                            using (Pen pivotPen = new Pen(Color.Red, 1.0f))
                            using (Brush pivotBrush = new SolidBrush(Color.Red))
                            {
                                // Вычисляем позицию pivot с учетом масштаба и трансформаций
                                float pivotX = spritePosX;
                                float pivotY = spritePosY;

                                if (selectedSprite.FlipX)
                                {
                                    pivotX += (sourceBounds.Width - selectedSprite.Pivot.X) * scale;
                                }
                                else
                                {
                                    pivotX += selectedSprite.Pivot.X * scale;
                                }

                                if (selectedSprite.FlipY)
                                {
                                    pivotY += (sourceBounds.Height - selectedSprite.Pivot.Y) * scale;
                                }
                                else
                                {
                                    pivotY += selectedSprite.Pivot.Y * scale;
                                }

                                // Рисуем крестик
                                g.DrawLine(pivotPen, pivotX - 5, pivotY, pivotX + 5, pivotY);
                                g.DrawLine(pivotPen, pivotX, pivotY - 5, pivotX, pivotY + 5);

                                // Рисуем точку
                                g.FillEllipse(pivotBrush, pivotX - 2, pivotY - 2, 4, 4);
                            }
                        }

                        // Информация о трансформациях
                        using (Font font = new Font("Arial", 8))
                        using (Brush textBrush = new SolidBrush(Color.Lime))
                        {
                            string transformInfo = "";
                            if (selectedSprite.FlipX && selectedSprite.FlipY)
                                transformInfo = "Transform: XY";
                            else if (selectedSprite.FlipX)
                                transformInfo = "Transform: X";
                            else if (selectedSprite.FlipY)
                                transformInfo = "Transform: Y";
                            else
                                transformInfo = "Transform: None";

                            string info = string.Format("Size: {0}×{1}\n{2}",
                                sourceBounds.Width,
                                sourceBounds.Height,
                                transformInfo);

                            g.DrawString(info, font, textBrush, 10, 10);
                        }
                    }

                    // Освобождаем старое изображение
                    if (previewBox.Image != null)
                    {
                        previewBox.Image.Dispose();
                    }
                    previewBox.Image = bmp;
                }
                else
                {
                    previewBox.Image = null;
                }
            }
            else
            {
                previewBox.Image = null;
            }

            // Принудительно обновляем preview box
            previewBox.Invalidate();
        }

        private void btnAddRect_Click(object sender, EventArgs e)
        {
            lblInfo.Text = "Draw rectangle on image";
        }

        private void btnDeleteRect_Click(object sender, EventArgs e)
        {
            if (selectedSprite != null && currentAtlas != null)
            {
                currentAtlas.Sprites.Remove(selectedSprite);
                RefreshSpriteList();
                selectedSprite = null;
                pictureBox1.Invalidate();
                previewBox.Image = null;
            }
        }

        private void btnZoomIn_Click(object sender, EventArgs e)
        {
            zoomFactor += zoomStep;
            ApplyZoom();
        }

        private void btnZoomOut_Click(object sender, EventArgs e)
        {
            if (zoomFactor > zoomStep)
            {
                zoomFactor -= zoomStep;
                ApplyZoom();
            }
        }

        private void btnZoomReset_Click(object sender, EventArgs e)
        {
            zoomFactor = 1.0f;
            ApplyZoom();
        }

        private void ApplyZoom()
        {
            if (loadedImage != null)
            {
                pictureBox1.Size = new Size((int)(loadedImage.Width * zoomFactor), (int)(loadedImage.Height * zoomFactor));
                lblInfo.Text = string.Format("Zoom: {0:P0}", zoomFactor);
                pictureBox1.Invalidate();
            }
        }

        private void btnFillRect_Click(object sender, EventArgs e)
        {
            if (loadedImage != null && currentAtlas != null)
            {
                Rectangle fullImageRect = new Rectangle(0, 0, loadedImage.Width, loadedImage.Height);
                SpriteRegion sprite = new SpriteRegion(fullImageRect);
                currentAtlas.Sprites.Add(sprite);
                RefreshSpriteList();
                pictureBox1.Invalidate();
                lblInfo.Text = "Full image rectangle added.";
            }
            else
            {
                MessageBox.Show("Load an image first.");
            }
        }

        private void buttonSave_Click(object sender, EventArgs e)
        {
            if (loadedImage == null || currentAtlas == null || currentAtlas.Sprites.Count == 0)
            {
                MessageBox.Show("No sprites defined or no image loaded.");
                return;
            }

            SaveFileDialog sfd = new SaveFileDialog();
            sfd.Filter = "Binary Files (*.bin)|*.bin";
            if (sfd.ShowDialog() == DialogResult.OK)
            {
                try
                {
                    currentAtlas.SaveToFile(sfd.FileName);
                    MessageBox.Show("Saved successfully!");
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Error saving file:\n" + ex.Message);
                }
            }
        }

        private void CenterAnimationOnPivot()
        {
            if (currentAtlas != null && currentAtlas.Type == AtlasType.Animation &&
                currentAtlas.Sprites.Count > 0 && selectedSprite != null)
            {
                Point pivot = selectedSprite.Pivot;
                Point spriteLocation = selectedSprite.Bounds.Location;

                int offsetX = (pictureBox1.Width / 2) - (pivot.X + spriteLocation.X);
                int offsetY = (pictureBox1.Height / 2) - (pivot.Y + spriteLocation.Y);


                lblInfo.Text = string.Format("Pivot centered at: {0}, {1}",
                    pivot.X + spriteLocation.X, pivot.Y + spriteLocation.Y);

                pictureBox1.Invalidate();
            }
        }

        private void btnCenterPivotAnimation_Click(object sender, EventArgs e)
        {
            if (currentAtlas != null && currentAtlas.Type == AtlasType.Animation &&
                selectedSprite != null && selectedSprite.HasPivot)
            {
                Point pivot = selectedSprite.Pivot;
                Point spriteLocation = selectedSprite.Bounds.Location;

                lblInfo.Text = string.Format("Animation centered on pivot: {0}, {1}",
                    spriteLocation.X + pivot.X, spriteLocation.Y + pivot.Y);

                // Здесь можно добавить логику для реального центрирования
                // Например, прокрутку ScrollPanel или изменение offset
                pictureBox1.Invalidate(); // Перерисовываем для показа маркера
            }
            else
            {
                MessageBox.Show("Select a sprite with pivot point first!");
            }
        }

        private void previewBox_Paint(object sender, PaintEventArgs e)
        {
            if (chkShowGuides.Checked && selectedSprite != null && previewBox.Image != null)
            {
                bool hasUvData =
                    Math.Abs(selectedSprite.UV_Min.X) > 0.0001f ||
                    Math.Abs(selectedSprite.UV_Min.Y) > 0.0001f ||
                    Math.Abs(selectedSprite.UV_Max.X - 1.0f) > 0.0001f ||
                    Math.Abs(selectedSprite.UV_Max.Y - 1.0f) > 0.0001f;

                Rectangle sourceBounds = selectedSprite.IsPacked ? selectedSprite.OriginalBounds : selectedSprite.Bounds;

                float blockWidth = sourceBounds.Width;
                float blockHeight = sourceBounds.Height;
                if (hasUvData)
                {
                    float uvWidth = selectedSprite.UV_Max.X - selectedSprite.UV_Min.X;
                    float uvHeight = selectedSprite.UV_Max.Y - selectedSprite.UV_Min.Y;

                    if (uvWidth > 0.0001f)
                        blockWidth = selectedSprite.OriginalBounds.Width / uvWidth;
                    if (uvHeight > 0.0001f)
                        blockHeight = selectedSprite.OriginalBounds.Height / uvHeight;
                }

                float padX = hasUvData ? selectedSprite.UV_Min.X * blockWidth : 0f;
                float padY = hasUvData ? selectedSprite.UV_Min.Y * blockHeight : 0f;

                float scaledBlockWidth = blockWidth * previewZoomFactor;
                float scaledBlockHeight = blockHeight * previewZoomFactor;
                float blockPosX = (previewBox.Width - scaledBlockWidth) / 2f;
                float blockPosY = (previewBox.Height - scaledBlockHeight) / 2f;

                // === Направляющие (красные) ===
                if (chkShowPivotGuides.Checked)
                {
                    // Центр блока
                    float centerX = blockPosX + scaledBlockWidth / 2f;
                    float centerY = blockPosY + scaledBlockHeight / 2f;

                    // Если есть pivot - центрируем по нему относительно спрайта в блоке
                    if (selectedSprite.HasPivot)
                    {
                        centerX = blockPosX + (padX + selectedSprite.Pivot.X) * previewZoomFactor;
                        centerY = blockPosY + (padY + selectedSprite.Pivot.Y) * previewZoomFactor;
                    }

                    // Рисуем направляющие через центр
                    using (Pen guidePen = new Pen(Color.FromArgb(128, Color.Red), 1.0f))
                    {
                        e.Graphics.DrawLine(guidePen, centerX, 0, centerX, previewBox.Height);
                        e.Graphics.DrawLine(guidePen, 0, centerY, previewBox.Width, centerY);
                    }

                    using (Pen centerPen = new Pen(Color.Red, 2.0f))
                    using (Brush centerBrush = new SolidBrush(Color.FromArgb(128, Color.Red)))
                    {
                        e.Graphics.FillEllipse(centerBrush, centerX - 3, centerY - 3, 6, 6);
                        e.Graphics.DrawEllipse(centerPen, centerX - 3, centerY - 3, 6, 6);
                    }
                }
            }
        }

        private void btnCenterSprite_Click(object sender, EventArgs e)
        {
            if (selectedSprite != null)
            {
                // Центрируем pivot точку спрайта
                int centerX = selectedSprite.Bounds.Width / 2;
                int centerY = selectedSprite.Bounds.Height / 2;
                selectedSprite.Pivot = new Point(centerX, centerY);
                pictureBox1.Invalidate();
                UpdatePreview(); // Перерисовываем previewBox с новыми направляющими
                lblInfo.Text = string.Format("Sprite centered: {0}, {1}", centerX, centerY);
            }
            else
            {
                MessageBox.Show("Select a sprite first!");
            }
        }

        private void previewBox_MouseMove(object sender, MouseEventArgs e)
        {
            if (selectedSprite != null && selectedSprite.Bounds.Width > 0 && selectedSprite.Bounds.Height > 0)
            {
                // Используем ту же геометрию, что и в UpdatePreview
                bool hasUvData =
                    Math.Abs(selectedSprite.UV_Min.X) > 0.0001f ||
                    Math.Abs(selectedSprite.UV_Min.Y) > 0.0001f ||
                    Math.Abs(selectedSprite.UV_Max.X - 1.0f) > 0.0001f ||
                    Math.Abs(selectedSprite.UV_Max.Y - 1.0f) > 0.0001f;

                Rectangle sourceBounds = selectedSprite.IsPacked ? selectedSprite.OriginalBounds : selectedSprite.Bounds;

                float blockWidth = sourceBounds.Width;
                float blockHeight = sourceBounds.Height;
                if (hasUvData)
                {
                    float uvWidth = selectedSprite.UV_Max.X - selectedSprite.UV_Min.X;
                    float uvHeight = selectedSprite.UV_Max.Y - selectedSprite.UV_Min.Y;

                    if (uvWidth > 0.0001f)
                        blockWidth = selectedSprite.OriginalBounds.Width / uvWidth;
                    if (uvHeight > 0.0001f)
                        blockHeight = selectedSprite.OriginalBounds.Height / uvHeight;
                }

                float padX = hasUvData ? selectedSprite.UV_Min.X * blockWidth : 0f;
                float padY = hasUvData ? selectedSprite.UV_Min.Y * blockHeight : 0f;

                float scaledBlockWidth = blockWidth * previewZoomFactor;
                float scaledBlockHeight = blockHeight * previewZoomFactor;
                float blockPosX = (previewBox.Width - scaledBlockWidth) / 2f;
                float blockPosY = (previewBox.Height - scaledBlockHeight) / 2f;

                // Координаты внутри блока
                float xInBlock = (e.X - blockPosX) / previewZoomFactor;
                float yInBlock = (e.Y - blockPosY) / previewZoomFactor;

                int spriteX = (int)Math.Round(xInBlock - padX);
                int spriteY = (int)Math.Round(yInBlock - padY);

                previewMousePos = new Point(spriteX, spriteY);
                previewBox.Invalidate(); // Перерисовываем для обновления координат
            }
        }

        private void chkShowGuides_CheckedChanged(object sender, EventArgs e)
        {
            showGuidesToolStripMenuItem.Text = chkShowGuides.Checked ? "Hide Guides" : "Show Guides";
            previewBox.Invalidate(); // Перерисовываем при изменении состояния чекбокса
        }

        private void btnMoveUp_Click(object sender, EventArgs e)
        {
            if (currentAtlas != null && currentAtlas.Type == AtlasType.Animation &&
                lstSprites.SelectedItem != null && lstSprites.SelectedIndex > 0)
            {
                int selectedIndex = lstSprites.SelectedIndex;
                SpriteRegion sprite = currentAtlas.Sprites[selectedIndex];

                // Меняем местами с предыдущим
                currentAtlas.Sprites.RemoveAt(selectedIndex);
                currentAtlas.Sprites.Insert(selectedIndex - 1, sprite);

                // Обновляем список
                RefreshSpriteList();
                lstSprites.SelectedIndex = selectedIndex - 1;
            }
        }

        private void btnMoveDown_Click(object sender, EventArgs e)
        {
            if (currentAtlas != null && currentAtlas.Type == AtlasType.Animation &&
                lstSprites.SelectedItem != null && lstSprites.SelectedIndex < lstSprites.Items.Count - 1)
            {
                int selectedIndex = lstSprites.SelectedIndex;
                SpriteRegion sprite = currentAtlas.Sprites[selectedIndex];

                // Меняем местами со следующим
                currentAtlas.Sprites.RemoveAt(selectedIndex);
                currentAtlas.Sprites.Insert(selectedIndex + 1, sprite);

                // Обновляем список
                RefreshSpriteList();
                lstSprites.SelectedIndex = selectedIndex + 1;
            }
        }

        private void btnAutoPack_Click(object sender, EventArgs e)
        {
            if (currentAtlas != null && loadedImage != null)
            {
                // Просто рассчитываем UV координаты на основе реальных координат
                currentAtlas.CalculateUVFromRealCoordinates(loadedImage.Width, loadedImage.Height);

                UpdatePreview();
                pictureBox1.Invalidate();
                previewBox.Invalidate();
                lblInfo.Text = "UV координаты рассчитаны";
            }
            else
            {
                MessageBox.Show("Загрузите изображение и создайте атлас.");
            }
        }

        private void btnExportUV_Click(object sender, EventArgs e)
        {
            if (currentAtlas == null || currentAtlas.Sprites.Count == 0)
            {
                MessageBox.Show("Нет спрайтов для экспорта.");
                return;
            }

            SaveFileDialog sfd = new SaveFileDialog();
            sfd.Filter = "Text Files (*.txt)|*.txt";
            if (sfd.ShowDialog() == DialogResult.OK)
            {
                try
                {
                    using (StreamWriter sw = new StreamWriter(sfd.FileName))
                    {
                        for (int i = 0; i < currentAtlas.Sprites.Count; i++)
                        {
                            var sprite = currentAtlas.Sprites[i];
                            sw.WriteLine("Sprite{0}", i);
                            sw.WriteLine("  UV Min: {0:F6}, {1:F6}", sprite.UV_Min.X, sprite.UV_Min.Y);
                            sw.WriteLine("  UV Max: {0:F6}, {1:F6}", sprite.UV_Max.X, sprite.UV_Max.Y);
                            sw.WriteLine();
                        }
                    }

                    MessageBox.Show("Файл UV успешно экспортирован.");
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Ошибка экспорта UV: " + ex.Message);
                }
            }
        }

        private void btnDeleteSelected_Click(object sender, EventArgs e)
        {
            if (selectedSprite != null && currentAtlas != null)
            {
                DialogResult result = MessageBox.Show(
                    "Delete selected sprite?",
                    "Confirm Delete",
                    MessageBoxButtons.YesNo,
                    MessageBoxIcon.Question);

                if (result == DialogResult.Yes)
                {
                    int selectedIndex = lstSprites.SelectedIndex;

                    // Удаляем из атласа
                    currentAtlas.Sprites.Remove(selectedSprite);

                    // Обновляем список
                    RefreshSpriteList();

                    // Выбираем следующий спрайт, если возможно
                    if (currentAtlas.Sprites.Count > 0)
                    {
                        int newIndex = Math.Min(selectedIndex, currentAtlas.Sprites.Count - 1);
                        if (newIndex >= 0)
                        {
                            lstSprites.SelectedIndex = newIndex;
                            selectedSprite = currentAtlas.Sprites[newIndex];
                            UpdatePreview(); // Обновляем preview для нового спрайта
                        }
                        else
                        {
                            selectedSprite = null;
                            previewBox.Image = null;
                        }
                    }
                    else
                    {
                        selectedSprite = null;
                        previewBox.Image = null;
                    }

                    // Обновляем интерфейс
                    RefreshSpriteList();
                    UpdatePreview();
                    pictureBox1.Invalidate();
                    previewBox.Invalidate();
                    lblInfo.Text = "Sprite deleted";
                }
            }
            else
            {
                MessageBox.Show("No sprite selected");
            }
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void chkFillUVArea_CheckedChanged(object sender, EventArgs e)
        {
            fillUVAreaToolStripMenuItem.Text = chkFillUVArea.Checked ? "Don't Fill UV Area" : "Fill UV Area";
            if (previewBox != null)
            {
                previewBox.Invalidate();
            }
        }

        private void showGuidesToolStripMenuItem_Click(object sender, EventArgs e)
        {
            // Переключаем состояние
            chkShowGuides.Checked = !chkShowGuides.Checked;
            // Обновляем текст меню
            showGuidesToolStripMenuItem.Text = chkShowGuides.Checked ? "Hide Guides" : "Show Guides";
            previewBox.Invalidate();
        }

        private void fillUVAreaToolStripMenuItem_Click(object sender, EventArgs e)
        {
            // Переключаем состояние
            chkFillUVArea.Checked = !chkFillUVArea.Checked;
            // Обновляем текст меню
            fillUVAreaToolStripMenuItem.Text = chkFillUVArea.Checked ? "Don't Fill UV Area" : "Fill UV Area";
            previewBox.Invalidate();
        }

        private void showPivotGuidesToolStripMenuItem_Click(object sender, EventArgs e)
        {
            chkShowPivotGuides.Checked = !chkShowPivotGuides.Checked;
            showPivotGuidesToolStripMenuItem.Text = chkShowPivotGuides.Checked ? "Hide Pivot Guides" : "Show Pivot Guides";
            previewBox.Invalidate();
            UpdatePreview();
        }

        private void exportUVToolStripMenuItem_Click(object sender, EventArgs e)
        {
            btnExportUV_Click(sender, e);
        }

        private void btnPreviewZoomIn_Click(object sender, EventArgs e)
        {
            previewZoomFactor += previewZoomStep;
            UpdatePreview();
            previewBox.Invalidate();
            lblInfo.Text = string.Format("Preview Zoom: {0:P0}", previewZoomFactor);
        }

        private void btnPreviewZoomOut_Click(object sender, EventArgs e)
        {
            if (previewZoomFactor > previewZoomStep)
            {
                previewZoomFactor -= previewZoomStep;
                UpdatePreview();
                previewBox.Invalidate();
                lblInfo.Text = string.Format("Preview Zoom: {0:P0}", previewZoomFactor);
            }
        }

        private void btnPreviewZoomReset_Click(object sender, EventArgs e)
        {
            previewZoomFactor = 1.0f;
            UpdatePreview();
            previewBox.Invalidate();
            lblInfo.Text = string.Format("Preview Zoom: {0:P0}", previewZoomFactor);
        }

        private void UpdateSpriteListItem(SpriteRegion sprite)
        {
            if (sprite != null && lstSprites.Items.Contains(sprite))
            {
                int index = lstSprites.Items.IndexOf(sprite);
                lstSprites.Items[index] = sprite;
                lstSprites.Refresh();
            }
        }

        private void chkShowPivotGuides_CheckedChanged(object sender, EventArgs e)
        {
            showPivotGuidesToolStripMenuItem.Text = chkShowPivotGuides.Checked ? "Hide Pivot Guides" : "Show Pivot Guides";
            previewBox.Invalidate();
            UpdatePreview(); // Перерисовываем preview тоже
        }

        private void btnPivotTopLeft_Click(object sender, EventArgs e)
        {
            if (selectedSprite != null)
            {
                // Установить точку опоры в левый верхний угол спрайта
                selectedSprite.Pivot = new Point(0, 0);

                // Обновить отображение
                pictureBox1.Invalidate();
                UpdatePreview();
                previewBox.Invalidate();

                lblInfo.Text = "Pivot установлен в левый верхний угол: 0, 0";
            }
            else
            {
                MessageBox.Show("Сначала выберите спрайт.");
            }
        }

        private void btnEditCollision_Click(object sender, EventArgs e)
        {
            if (selectedSprite != null && loadedImage != null)
            {
                Rectangle srcRect = selectedSprite.OriginalBounds;
                if (srcRect.Width <= 0 || srcRect.Height <= 0) return;
                EditCollisionForm form = new EditCollisionForm(selectedSprite, loadedImage);
                if (form.ShowDialog() == DialogResult.OK)
                {
                    selectedSprite.Collision = form.CollisionInfo;
                    pictureBox1.Invalidate();
                    UpdatePreview();
                }
            }
            else
            {
                MessageBox.Show("Select a sprite first!");
            }
        }

        private void btnEditNodeWeight_Click(object sender, EventArgs e)
        {
            if (selectedSprite != null && loadedImage != null)
            {
                Rectangle srcRect = selectedSprite.OriginalBounds;
                if (srcRect.Width <= 0 || srcRect.Height <= 0) return;
                EditNodeWeightForm form = new EditNodeWeightForm(selectedSprite, loadedImage);
                if (form.ShowDialog() == DialogResult.OK)
                {
                    selectedSprite.NodeWeights = form.NodeWeights;
                    int count = selectedSprite.NodeWeights.Entries.Count;
                    lblInfo.Text = string.Format("NodeWeight entries: {0}", count);
                    RefreshSpriteList();
                }
            }
            else
            {
                MessageBox.Show("Select a sprite first!");
            }
        }

        private ushort ReadBigEndianUShort(BinaryReader br)
        {
            byte[] bytes = br.ReadBytes(2);
            if (BitConverter.IsLittleEndian)
                Array.Reverse(bytes);
            return BitConverter.ToUInt16(bytes, 0);
        }

        private void fileCloseToolStripMenuItem_Click(object sender, EventArgs e)
        {
            // Спрашиваем подтверждение, если есть несохраненные изменения
            if (currentAtlas != null && currentAtlas.Sprites.Count > 0)
            {
                DialogResult result = MessageBox.Show(
                    "Закрыть текущий атлас? Все несохраненные изменения будут потеряны.",
                    "Подтверждение закрытия",
                    MessageBoxButtons.YesNo,
                    MessageBoxIcon.Question);

                if (result == DialogResult.No)
                    return;
            }

            // Очищаем текущий атлас
            if (currentAtlas != null)
            {
                currentAtlas.Sprites.Clear();
            }

            // Очищаем изображение
            if (loadedImage != null)
            {
                loadedImage.Dispose();
                loadedImage = null;
            }

            // Сбрасываем переменные
            selectedSprite = null;
            loadedImagePath = "";
            lblFileName.Text = "No file loaded";

            // Очищаем списки
            lstSprites.Items.Clear();
            if (currentAtlas != null && currentAtlas.Type == AtlasType.MultiLevel)
            {
                MultiLevelAtlas multiAtlas = currentAtlas as MultiLevelAtlas;
                if (multiAtlas != null)
                {
                    multiAtlas.Groups.Clear();
                }
            }

            // Обновляем интерфейс
            RefreshSpriteList();
            RefreshGroupsList();
            pictureBox1.Invalidate();
            previewBox.Image = null;
            previewBox.Invalidate();

            // Сбрасываем масштаб
            zoomFactor = 1.0f;
            previewZoomFactor = 1.0f;
            ApplyZoom();

            lblInfo.Text = "Атлас закрыт";
        }
        //метод для создания зеркальных копий
        private void CreateMirroredCopy(SpriteRegion original, bool flipX, bool flipY, string suffix)
        {
            if (currentAtlas != null && original != null && loadedImage != null)
            {
                SpriteRegion mirror = new SpriteRegion(original.Bounds)
                {
                    OriginalBounds = original.OriginalBounds,
                    Pivot = original.Pivot,
                    DisplayBounds = original.DisplayBounds,
                    BlockOffset = original.BlockOffset,
                    Name = !string.IsNullOrEmpty(original.Name) ? original.Name + suffix : "Sprite" + suffix,
                    FlipX = flipX,
                    FlipY = flipY
                };

                // Корректируем pivot при зеркалировании
                if (original.HasPivot)
                {
                    int pivotX = original.Pivot.X;
                    int pivotY = original.Pivot.Y;

                    if (flipX)
                    {
                        pivotX = original.OriginalBounds.Width - original.Pivot.X - 1;
                    }
                    if (flipY)
                    {
                        pivotY = original.OriginalBounds.Height - original.Pivot.Y - 1;
                    }

                    mirror.Pivot = new Point(pivotX, pivotY);
                }

                currentAtlas.Sprites.Add(mirror);

                // Пересчитываем UV для нового спрайта
                mirror.CalculateUVFromRealCoordinates(loadedImage.Width, loadedImage.Height);
            }
        }


        private void btnCreateMirror_Click(object sender, EventArgs e)
        {
            if (selectedSprite != null && currentAtlas != null && loadedImage != null)
            {
                // Показываем диалог выбора типа зеркалирования
                MirrorOptionsForm mirrorForm = new MirrorOptionsForm();
                if (mirrorForm.ShowDialog() == DialogResult.OK)
                {
                    string suffix = "";
                    if (mirrorForm.MirrorX && mirrorForm.MirrorY)
                        suffix = "_MirrorXY";
                    else if (mirrorForm.MirrorX)
                        suffix = "_MirrorX";
                    else if (mirrorForm.MirrorY)
                        suffix = "_MirrorY";

                    // Создаем зеркальную копию с выбранными параметрами
                    CreateMirroredCopy(selectedSprite, mirrorForm.MirrorX, mirrorForm.MirrorY, suffix);

                    RefreshSpriteList();
                    pictureBox1.Invalidate();
                    UpdatePreview();

                    // Выбираем последний добавленный спрайт
                    if (currentAtlas.Sprites.Count > 0)
                    {
                        lstSprites.SelectedIndex = currentAtlas.Sprites.Count - 1;
                    }

                    string mirrorType = "";
                    if (mirrorForm.MirrorX && mirrorForm.MirrorY)
                        mirrorType = "horizontally and vertically";
                    else if (mirrorForm.MirrorX)
                        mirrorType = "horizontally";
                    else if (mirrorForm.MirrorY)
                        mirrorType = "vertically";

                    lblInfo.Text = string.Format("Created mirror copy ({0})", mirrorType);
                }
            }
            else
            {
                MessageBox.Show("Select a sprite first and make sure image is loaded!");
            }
        }

        private string GenerateUniqueSpriteName(SpriteRegion sprite, string prefix = "Sprite")
        {
            if (!string.IsNullOrEmpty(sprite.Name) && sprite.Name != "Sprite")
                return sprite.Name; // Уже есть имя

            // Если это MultiLevel атlas, используем имя группы
            if (currentAtlas != null && currentAtlas.Type == AtlasType.MultiLevel)
            {
                MultiLevelAtlas multiAtlas = currentAtlas as MultiLevelAtlas;
                if (multiAtlas != null)
                {
                    // Ищем, в какой группе этот спрайт
                    foreach (SpriteGroup group in multiAtlas.Groups)
                    {
                        int spriteIndexInGroup = group.Sprites.IndexOf(sprite);
                        if (spriteIndexInGroup >= 0)
                        {
                            // Формат: GroupName_SpriteIndex
                            return string.Format("{0}_{1:D3}", group.Name, spriteIndexInGroup + 1);
                        }
                    }
                }
            }

            // Для других типов атласов или если группа не найдена
            int spriteIndex = -1;
            if (currentAtlas != null)
            {
                spriteIndex = currentAtlas.Sprites.IndexOf(sprite);
            }

            if (spriteIndex >= 0)
            {
                return string.Format("{0}_{1:D3}", prefix, spriteIndex + 1);
            }

            // Если не можем определить индекс, генерируем случайное имя
            return string.Format("{0}_{1:X4}", prefix, Guid.NewGuid().ToString().Substring(0, 4).ToUpper());
        }

        private void AutoGenerateSpriteNames()
        {
            if (currentAtlas == null) return;

            HashSet<string> usedNames = new HashSet<string>();

            // Сначала собираем все существующие имена
            foreach (SpriteRegion sprite in currentAtlas.Sprites)
            {
                if (!string.IsNullOrEmpty(sprite.Name) &&
                    sprite.Name != "Sprite" &&
                    !sprite.Name.StartsWith("Sprite_"))
                {
                    usedNames.Add(sprite.Name.ToLower());
                }
            }

            for (int i = 0; i < currentAtlas.Sprites.Count; i++)
            {
                SpriteRegion sprite = currentAtlas.Sprites[i];

                // Пропускаем спрайты, у которых уже есть осмысленные имена
                if (!string.IsNullOrEmpty(sprite.Name) &&
                    sprite.Name != "Sprite" &&
                    !sprite.Name.StartsWith("Sprite_"))
                {
                    continue;
                }

                // Генерируем имя на основе группы
                string baseName = "";

                if (currentAtlas.Type == AtlasType.MultiLevel)
                {
                    MultiLevelAtlas multiAtlas = currentAtlas as MultiLevelAtlas;
                    if (multiAtlas != null)
                    {
                        foreach (SpriteGroup group in multiAtlas.Groups)
                        {
                            if (group.Sprites.Contains(sprite))
                            {
                                string groupName = SanitizeName(group.Name);
                                baseName = string.Format("{0}_{1:D2}", groupName, group.Sprites.IndexOf(sprite) + 1);
                                break;
                            }
                        }
                    }
                }

                // Если не нашли группу или это другой тип атласа
                if (string.IsNullOrEmpty(baseName))
                {
                    baseName = string.Format("Sprite_{0:D3}", i + 1);
                }

                // Делаем имя уникальным
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

        private string MakeNameUnique(string baseName, HashSet<string> usedNames)
        {
            string testName = baseName;
            int counter = 1;

            while (usedNames.Contains(testName.ToLower()))
            {
                testName = string.Format("{0}_{1}", baseName, counter);
                counter++;
            }

            return testName;
        }

        private string SanitizeName(string name)
        {
            if (string.IsNullOrEmpty(name)) return "unnamed";

            // Заменяем недопустимые символы подчеркиваниями
            char[] invalidChars = Path.GetInvalidFileNameChars();
            StringBuilder sb = new StringBuilder(name);

            for (int i = 0; i < sb.Length; i++)
            {
                if (Array.IndexOf(invalidChars, sb[i]) >= 0 || sb[i] == ' ')
                {
                    sb[i] = '_';
                }
            }

            // Убираем множественные подчеркивания
            string result = System.Text.RegularExpressions.Regex.Replace(sb.ToString(), "_+", "_");

            // Убираем подчеркивания в начале и конце
            result = result.Trim('_');

            return string.IsNullOrEmpty(result) ? "unnamed" : result;
        }

        private void btnAutoNameSprites_Click(object sender, EventArgs e)
        {
            if (selectedSprite != null)
            {
                // Показываем диалог переименования для выбранного спрайта
                RenameSelectedSprite();
            }
            else if (currentAtlas != null && currentAtlas.Sprites.Count > 0)
            {
                // Если ничего не выбрано, но есть спрайты - спрашиваем, хотим ли мы
                // автоматически сгенерировать имена для всех
                DialogResult result = MessageBox.Show(
                    "No sprite selected. Do you want to auto-generate names for all sprites?",
                    "Auto Name Sprites",
                    MessageBoxButtons.YesNo,
                    MessageBoxIcon.Question);

                if (result == DialogResult.Yes)
                {
                    AutoGenerateSpriteNames();
                    RefreshSpriteList();
                    MessageBox.Show(string.Format("Generated names for {0} sprites", currentAtlas.Sprites.Count));
                }
            }
            else
            {
                MessageBox.Show("No sprites to rename");
            }
        }

        private void RenameSelectedSprite()
        {
            if (selectedSprite != null)
            {
                RenameSpriteForm renameForm = new RenameSpriteForm(selectedSprite.Name);
                if (renameForm.ShowDialog() == DialogResult.OK)
                {
                    selectedSprite.Name = renameForm.NewName;
                    RefreshSpriteList();
                }
            }
        }

        private void lstSprites_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            RenameSelectedSprite();
        }

        private void editNameToolStripMenuItem_Click(object sender, EventArgs e)
        {
            RenameSelectedSprite();
        }

        private void btnCreateMirrorX_Click(object sender, EventArgs e)
        {
            if (selectedSprite != null && currentAtlas != null && loadedImage != null)
            {
                CreateMirroredCopy(selectedSprite, true, false, "_MirrorX");
                RefreshSpriteList();
                pictureBox1.Invalidate();
                UpdatePreview();

                if (currentAtlas.Sprites.Count > 0)
                {
                    lstSprites.SelectedIndex = currentAtlas.Sprites.Count - 1;
                }

                lblInfo.Text = "Created horizontal mirror copy";
            }
        }

        private void btnCreateMirrorY_Click(object sender, EventArgs e)
        {
            if (selectedSprite != null && currentAtlas != null && loadedImage != null)
            {
                CreateMirroredCopy(selectedSprite, false, true, "_MirrorY");
                RefreshSpriteList();
                pictureBox1.Invalidate();
                UpdatePreview();

                if (currentAtlas.Sprites.Count > 0)
                {
                    lstSprites.SelectedIndex = currentAtlas.Sprites.Count - 1;
                }

                lblInfo.Text = "Created vertical mirror copy";
            }
        }

        private void loadDefaultToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (string.IsNullOrEmpty(loadedImagePath))
            {
                MessageBox.Show("No image loaded. Please load an image first.", "Load Default",
                    MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            try
            {
                string directory = Path.GetDirectoryName(loadedImagePath);
                string fileNameWithoutExtension = Path.GetFileNameWithoutExtension(loadedImagePath);

                // Создаем имя файла по умолчанию
                foreach (char c in Path.GetInvalidFileNameChars())
                {
                    fileNameWithoutExtension = fileNameWithoutExtension.Replace(c, '_');
                }

                string defaultBinPath = Path.Combine(directory, fileNameWithoutExtension + ".bin");

                // Проверяем существование файла
                if (File.Exists(defaultBinPath))
                {
                    DialogResult result = MessageBox.Show(
                        string.Format("Load default file '{0}'?", Path.GetFileName(defaultBinPath)),
                        "Load Default",
                        MessageBoxButtons.YesNo,
                        MessageBoxIcon.Question);

                    if (result == DialogResult.Yes)
                    {
                        // Здесь нужно определить тип атласа по файлу
                        AtlasType detectedType = DetectAtlasTypeFromFile(defaultBinPath);

                        // Пересоздаем атлас правильного типа
                        CreateNewAtlas(detectedType);

                        // Устанавливаем правильный тип в комбо боксе
                        comboBoxAtlasType.SelectedIndex = (int)detectedType;

                        // Загружаем данные
                        currentAtlas.LoadFromFile(defaultBinPath);

                        if (currentAtlas != null)
                        {
                            foreach (var sprite in currentAtlas.Sprites)
                            {
                                if (sprite.DisplayBounds.Width == 0 || sprite.DisplayBounds.Height == 0)
                                {
                                    sprite.DisplayBounds = sprite.OriginalBounds;
                                }
                            }
                        }

                        RefreshSpriteList();
                        pictureBox1.Invalidate();

                        lblInfo.Text = string.Format("Loaded {0} sprites from default file", currentAtlas.Sprites.Count);
                        MessageBox.Show(string.Format("Successfully loaded {0} sprites from default file",
                            currentAtlas.Sprites.Count), "Load Default", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    }
                }
                else
                {
                    MessageBox.Show(
                        string.Format("Default file '{0}' not found.", Path.GetFileName(defaultBinPath)),
                        "Load Default",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Information);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error loading default file:\n" + ex.Message, "Load Default Error",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
    }
}