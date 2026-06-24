using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;

namespace SpriteAtlasTool
{
    public partial class EditCollisionForm : Form
    {
        private SpriteRegion m_sprite;
        private Bitmap m_spriteTexture;
        private List<Point> m_selectedTiles = new List<Point>();
        private bool m_isSelectingTiles = false;
        private bool m_isSelectingEntrance = false;
        private Button btnSelectTiles;
        private int m_colliderOffsetX = 0;
        private int m_colliderOffsetY = 0;

        private const float NODE_TILE_W = 119.0f;
        private const float NODE_TILE_H = 74.0f;
        private const float HALF_NODE_W = 59.5f;
        private const float HALF_NODE_H = 37.0f;
        // Visual diamond size — matches engine GridMenu (m_cellVisualWidth/Height)
        private const float VISUAL_DIAMOND_W = 117.0f;
        private const float VISUAL_DIAMOND_H = 72.0f;
        private const float VISUAL_HALF_W = 58.5f;
        private const float VISUAL_HALF_H = 36.0f;
        private const int GRID_SIZE = 10;

        public CollisionInfo CollisionInfo
        {
            get { return m_collisionInfo; }
        }

        public EditCollisionForm(SpriteRegion sprite, Bitmap texture)
        {
            InitializeComponent();
            m_sprite = sprite;
            m_spriteTexture = texture;
            m_collisionInfo = sprite.Collision ?? new CollisionInfo();
            LoadValues();
            EnableDoubleBuffering();
        }

        private void LoadValues()
        {
            numWidth.Value = Math.Min(numWidth.Maximum, Math.Max(numWidth.Minimum, m_collisionInfo.Width));
            numHeight.Value = Math.Min(numHeight.Maximum, Math.Max(numHeight.Minimum, m_collisionInfo.Height));
            chkBlocksMovement.Checked = m_collisionInfo.BlocksMovement;
            chkIsTrigger.Checked = m_collisionInfo.IsTrigger;
            m_colliderOffsetX = m_collisionInfo.OffsetX;
            m_colliderOffsetY = m_collisionInfo.OffsetY;
            numEntranceX.Value = Math.Min(numEntranceX.Maximum, Math.Max(numEntranceX.Minimum, m_sprite.EntranceX));
            numEntranceY.Value = Math.Min(numEntranceY.Maximum, Math.Max(numEntranceY.Minimum, m_sprite.EntranceY));
            chkIsBuilding.Checked = m_sprite.IsBuilding;

            // Load mask if present
            m_selectedTiles.Clear();
            if (m_collisionInfo.MaskTiles != null && m_collisionInfo.MaskTiles.Count > 0)
            {
                foreach (Point rel in m_collisionInfo.MaskTiles)
                {
                    m_selectedTiles.Add(new Point(
                        rel.X + m_colliderOffsetX,
                        rel.Y + m_colliderOffsetY));
                }
                UpdateColliderFromSelectedTiles();
                m_colliderOffsetX = 0;
                m_colliderOffsetY = 0;
            }

            previewPanel.Invalidate();
            this.previewPanel.MouseClick += new System.Windows.Forms.MouseEventHandler(this.previewPanel_MouseClick);
        }

        private void EnableDoubleBuffering()
        {
            typeof(Panel).InvokeMember("DoubleBuffered",
                System.Reflection.BindingFlags.SetProperty | System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.NonPublic,
                null, previewPanel, new object[] { true });
        }

        private void ValuesChanged(object sender, EventArgs e)
        {
            previewPanel.Invalidate();
        }

        // Grid origin (0,0) in screen coords: aligned with sprite pivot
        private void GetGridCenter(out float cx, out float cy)
        {
            if (m_spriteTexture == null || m_sprite == null)
            {
                cx = previewPanel.Width * 0.5f;
                cy = previewPanel.Height * 0.5f;
                return;
            }

            float spriteW = m_sprite.OriginalBounds.Width;
            float spriteH = m_sprite.OriginalBounds.Height;
            float panelW = previewPanel.Width;
            float panelH = previewPanel.Height;
            float margin = 30.0f;
            float maxW = panelW - margin * 2;
            float maxH = panelH - margin * 2;
            float scale = 1.0f;
            if (spriteW > maxW || spriteH > maxH)
                scale = Math.Min(maxW / spriteW, maxH / spriteH);

            float drawW = spriteW * scale;
            float drawH = spriteH * scale;
            float drawX = (panelW - drawW) * 0.5f;
            float drawY = (panelH - drawH) * 0.5f;

            float pivotX = m_sprite.HasPivot ? m_sprite.Pivot.X : 0;
            float pivotY = m_sprite.HasPivot ? m_sprite.Pivot.Y : 0;

            cx = drawX + pivotX * scale;
            cy = drawY + pivotY * scale;
        }

        private void TileToScreen(int nx, int ny, float centerX, float centerY, out float sx, out float sy)
        {
            // Staggered projection: even rows shifted right by half tile (matches NodeTileToWorld)
            sx = centerX + nx * NODE_TILE_W + ((ny % 2 == 0) ? HALF_NODE_W : 0.0f);
            sy = centerY + ny * HALF_NODE_H;
        }

        private bool IsPointInDiamond(float px, float py, float cx, float cy, float hw, float hh)
        {
            float dx = Math.Abs(px - cx) / hw;
            float dy = Math.Abs(py - cy) / hh;
            return dx + dy <= 1.0f;
        }

        private PointF[] GetDiamondPoints(float cx, float cy, float hw, float hh)
        {
            return new PointF[]
            {
                new PointF(cx, cy - hh),
                new PointF(cx + hw, cy),
                new PointF(cx, cy + hh),
                new PointF(cx - hw, cy)
            };
        }

        private void previewPanel_MouseClick(object sender, MouseEventArgs e)
        {
            float centerX, centerY;
            GetGridCenter(out centerX, out centerY);

            int start = -(GRID_SIZE / 2);

            for (int y = 0; y < GRID_SIZE; y++)
            {
                for (int x = 0; x < GRID_SIZE; x++)
                {
                    int nx = start + x;
                    int ny = start + (GRID_SIZE - 1 - y);

                    float sx, sy;
                    TileToScreen(nx, ny, centerX, centerY, out sx, out sy);

                    if (IsPointInDiamond(e.X, e.Y, sx, sy, HALF_NODE_W, HALF_NODE_H))
                    {
                        Point tilePos = new Point(nx, ny);

                        if (m_isSelectingEntrance)
                        {
                            numEntranceX.Value = tilePos.X;
                            numEntranceY.Value = tilePos.Y;
                            previewPanel.Invalidate();
                            return;
                        }

                        if (m_isSelectingTiles)
                        {
                            if (m_selectedTiles.Contains(tilePos))
                                m_selectedTiles.Remove(tilePos);
                            else
                                m_selectedTiles.Add(tilePos);

                            previewPanel.Invalidate();
                            return;
                        }

                        return;
                    }
                }
            }
        }

        private void DrawSelectedTiles(Graphics graphics)
        {
            if (!m_isSelectingTiles || m_selectedTiles.Count == 0) return;

            float centerX, centerY;
            GetGridCenter(out centerX, out centerY);

            using (Brush selectBrush = new SolidBrush(Color.FromArgb(150, Color.Lime)))
            using (Pen selectPen = new Pen(Color.Lime, 2.0f))
            {
                foreach (Point tilePos in m_selectedTiles)
                {
                    float sx, sy;
                    TileToScreen(tilePos.X, tilePos.Y, centerX, centerY, out sx, out sy);

                    PointF[] pts = GetDiamondPoints(sx, sy, VISUAL_HALF_W, VISUAL_HALF_H);
                    graphics.FillPolygon(selectBrush, pts);
                    graphics.DrawPolygon(selectPen, pts);
                }
            }
        }

        private void btnSelectTiles_Click(object sender, EventArgs e)
        {
            m_isSelectingTiles = !m_isSelectingTiles;
            btnSelectTiles.Text = m_isSelectingTiles ? "Done Selecting" : "Select Tiles";
            if (m_isSelectingTiles) m_isSelectingEntrance = false;

            if (!m_isSelectingTiles)
            {
                UpdateColliderFromSelectedTiles();
                m_colliderOffsetX = 0;
                m_colliderOffsetY = 0;
            }

            UpdateEntranceButtonText();
            previewPanel.Invalidate();
        }

        private void btnSelectEntrance_Click(object sender, EventArgs e)
        {
            m_isSelectingEntrance = !m_isSelectingEntrance;
            if (m_isSelectingEntrance) m_isSelectingTiles = false;
            btnSelectTiles.Text = m_isSelectingTiles ? "Done Selecting" : "Select Tiles";
            UpdateEntranceButtonText();
            previewPanel.Invalidate();
        }

        private void UpdateEntranceButtonText()
        {
            if (m_isSelectingEntrance)
            {
                btnSelectEntrance.Text = "Done Entrance";
                lblEntranceInfo.Text = "Click a tile to set entrance";
            }
            else
            {
                btnSelectEntrance.Text = "Select Entrance";
                lblEntranceInfo.Text = "Click grid to set tile";
            }
        }

        private void UpdateColliderFromSelectedTiles()
        {
            if (m_selectedTiles.Count == 0) return;

            int minX = m_selectedTiles.Min(p => p.X);
            int maxX = m_selectedTiles.Max(p => p.X);
            int minY = m_selectedTiles.Min(p => p.Y);
            int maxY = m_selectedTiles.Max(p => p.Y);

            int width = maxX - minX + 1;
            int height = maxY - minY + 1;

            numWidth.Value = Math.Min(numWidth.Maximum, Math.Max(numWidth.Minimum, width));
            numHeight.Value = Math.Min(numHeight.Maximum, Math.Max(numHeight.Minimum, height));

            m_colliderOffsetX = minX;
            m_colliderOffsetY = minY;
        }

        private void DrawEntranceMarker(Graphics graphics)
        {
            int eX = (int)numEntranceX.Value;
            int eY = (int)numEntranceY.Value;
            if (eX == 0 && eY == 0) return;

            float centerX, centerY;
            GetGridCenter(out centerX, out centerY);

            float sx, sy;
            TileToScreen(eX, eY, centerX, centerY, out sx, out sy);

            using (Brush brush = new SolidBrush(Color.FromArgb(180, Color.Red)))
            using (Pen pen = new Pen(Color.Red, 2.0f))
            {
                PointF[] pts = GetDiamondPoints(sx, sy, VISUAL_HALF_W, VISUAL_HALF_H);
                graphics.FillPolygon(brush, pts);
                graphics.DrawPolygon(pen, pts);
            }

            using (Font font = new Font("Arial", 8, FontStyle.Bold))
            using (Brush textBrush = new SolidBrush(Color.White))
            {
                graphics.DrawString("E", font, textBrush, sx - 4, sy - 6);
            }

            lblEntranceInfo.Text = string.Format("Entrance: ({0}, {1})", eX, eY);
        }

        private void previewPanel_Paint(object sender, PaintEventArgs e)
        {
            e.Graphics.SmoothingMode = SmoothingMode.HighQuality;
            e.Graphics.InterpolationMode = InterpolationMode.HighQualityBilinear;
            e.Graphics.PixelOffsetMode = PixelOffsetMode.Default;

            e.Graphics.Clear(Color.FromArgb(20, 20, 20));

            DrawNodeGrid(e.Graphics);
            DrawCollisionCoverage(e.Graphics);
            DrawSpriteTexture(e.Graphics);
            DrawSelectedTiles(e.Graphics);
            DrawEntranceMarker(e.Graphics);

            using (Font font = new Font("Arial", 10, FontStyle.Bold))
            using (Brush textBrush = new SolidBrush(Color.White))
            {
                int width = (int)numWidth.Value;
                int height = (int)numHeight.Value;
                int eX = (int)numEntranceX.Value;
                int eY = (int)numEntranceY.Value;
                string entranceText = (eX == 0 && eY == 0) ? "" : string.Format("  Entrance: {0},{1}", eX, eY);
                string sizeText = string.Format("Collision: {0}×{1} tiles  Offset: {2},{3}{4}", width, height, m_colliderOffsetX, m_colliderOffsetY, entranceText);
                SizeF textSize = e.Graphics.MeasureString(sizeText, font);
                float textX = (previewPanel.Width - textSize.Width) * 0.5f;
                float textY = 10.0f;
                e.Graphics.DrawString(sizeText, font, textBrush, textX, textY);
            }
        }

        private void DrawNodeGrid(Graphics graphics)
        {
            float centerX, centerY;
            GetGridCenter(out centerX, out centerY);

            int start = -(GRID_SIZE / 2);

            using (Pen gridPen = new Pen(Color.FromArgb(80, 80, 80), 1.0f))
            using (Brush tileBrush = new SolidBrush(Color.FromArgb(40, 40, 40)))
            {
                for (int y = 0; y < GRID_SIZE; y++)
                {
                    for (int x = 0; x < GRID_SIZE; x++)
                    {
                        int nx = start + x;
                        int ny = start + (GRID_SIZE - 1 - y);

                        float sx, sy;
                        TileToScreen(nx, ny, centerX, centerY, out sx, out sy);

                        PointF[] pts = GetDiamondPoints(sx, sy, VISUAL_HALF_W, VISUAL_HALF_H);
                        graphics.FillPolygon(tileBrush, pts);
                        graphics.DrawPolygon(gridPen, pts);

                        // Label origin at tile (0,0)
                        if (nx == 0 && ny == 0)
                        {
                            using (Font f = new Font("Arial", 7, FontStyle.Bold))
                            using (Brush b = new SolidBrush(Color.FromArgb(180, Color.Orange)))
                            {
                                graphics.DrawString("0,0", f, b, sx - 8, sy - 12);
                            }
                        }
                    }
                }
            }
        }

        private void DrawCollisionCoverage(Graphics graphics)
        {
            if (m_spriteTexture == null || m_sprite == null) return;
            int collWidth = (int)numWidth.Value;
            int collHeight = (int)numHeight.Value;
            if (collWidth == 0 || collHeight == 0) return;

            float centerX, centerY;
            GetGridCenter(out centerX, out centerY);

            using (Brush coverBrush = new SolidBrush(Color.FromArgb(60, Color.Blue)))
            using (Pen coverPen = new Pen(Color.Blue, 1.0f))
            {
                if (m_selectedTiles.Count > 0)
                {
                    // Рисуем строго те тайлы, которые выделил пользователь (совпадает с зелёным)
                    foreach (Point tilePos in m_selectedTiles)
                    {
                        float sx, sy;
                        TileToScreen(tilePos.X, tilePos.Y, centerX, centerY, out sx, out sy);
                        PointF[] pts = GetDiamondPoints(sx, sy, VISUAL_HALF_W, VISUAL_HALF_H);
                        graphics.FillPolygon(coverBrush, pts);
                        graphics.DrawPolygon(coverPen, pts);
                    }
                }
                else
                {
                    // Без выделения — рисуем прямоугольник от сохранённого offset
                    int centerTileX = m_colliderOffsetX;
                    int centerTileY = m_colliderOffsetY;
                    for (int dy = 0; dy < collHeight; dy++)
                    {
                        for (int dx = 0; dx < collWidth; dx++)
                        {
                            int tileX = centerTileX + dx;
                            int tileY = centerTileY + dy;
                            float sx, sy;
                            TileToScreen(tileX, tileY, centerX, centerY, out sx, out sy);
                            PointF[] pts = GetDiamondPoints(sx, sy, VISUAL_HALF_W, VISUAL_HALF_H);
                            graphics.FillPolygon(coverBrush, pts);
                            graphics.DrawPolygon(coverPen, pts);
                        }
                    }
                }
            }
        }

        private void DrawSpriteTexture(Graphics graphics)
        {
            if (m_spriteTexture == null || m_sprite == null) return;

            Rectangle srcRect = m_sprite.OriginalBounds;
            float spriteW = srcRect.Width;
            float spriteH = srcRect.Height;

            float panelW = previewPanel.Width;
            float panelH = previewPanel.Height;

            float margin = 30.0f;
            float maxW = panelW - margin * 2;
            float maxH = panelH - margin * 2;

            float scale = 1.0f;
            if (spriteW > maxW || spriteH > maxH)
            {
                scale = Math.Min(maxW / spriteW, maxH / spriteH);
            }

            float drawW = spriteW * scale;
            float drawH = spriteH * scale;
            float drawX = (panelW - drawW) * 0.5f;
            float drawY = (panelH - drawH) * 0.5f;

            // Semi-transparency (40% visibility so grid shows through)
            float alpha = 0.4f;
            ColorMatrix colorMatrix = new ColorMatrix
            {
                Matrix33 = alpha
            };

            using (ImageAttributes imageAttributes = new ImageAttributes())
            {
                imageAttributes.SetColorMatrix(colorMatrix, ColorMatrixFlag.Default, ColorAdjustType.Bitmap);
                imageAttributes.SetWrapMode(System.Drawing.Drawing2D.WrapMode.Clamp);

                // Draw sub-region from full atlas using OriginalBounds
                graphics.DrawImage(
                    m_spriteTexture,
                    new Rectangle((int)drawX, (int)drawY, (int)drawW, (int)drawH),
                    srcRect.X,
                    srcRect.Y,
                    srcRect.Width,
                    srcRect.Height,
                    GraphicsUnit.Pixel,
                    imageAttributes
                );
            }

            // Yellow bounding box
            using (Pen bboxPen = new Pen(Color.Yellow, 1.0f))
            {
                graphics.DrawRectangle(bboxPen, drawX, drawY, drawW, drawH);
            }

            // Pivot crosshair
            float pivotX = m_sprite.HasPivot ? m_sprite.Pivot.X : 0;
            float pivotY = m_sprite.HasPivot ? m_sprite.Pivot.Y : 0;
            float pvx = drawX + pivotX * scale;
            float pvy = drawY + pivotY * scale;

            using (Pen pivotPen = new Pen(Color.Magenta, 2.0f))
            {
                graphics.DrawLine(pivotPen, pvx - 8, pvy, pvx + 8, pvy);
                graphics.DrawLine(pivotPen, pvx, pvy - 8, pvx, pvy + 8);
            }

            // "P" label at pivot
            using (Font font = new Font("Arial", 8, FontStyle.Bold))
            using (Brush brush = new SolidBrush(Color.Yellow))
            {
                graphics.DrawString("P", font, brush, pvx - 4, pvy - 14);
            }

            // Info label
            using (Font font = new Font("Arial", 8))
            using (Brush textBrush = new SolidBrush(Color.Cyan))
            {
                string info = string.Format("Sprite: {0}×{1}px", (int)spriteW, (int)spriteH);
                graphics.DrawString(info, font, textBrush, 10, 30);
            }
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            m_collisionInfo.Width = (int)numWidth.Value;
            m_collisionInfo.Height = (int)numHeight.Value;
            m_collisionInfo.BlocksMovement = chkBlocksMovement.Checked;
            m_collisionInfo.IsTrigger = chkIsTrigger.Checked;

            // Save mask from selected tiles (if any)
            m_collisionInfo.MaskTiles.Clear();
            if (m_selectedTiles.Count > 0)
            {
                UpdateColliderFromSelectedTiles();
                foreach (Point abs in m_selectedTiles)
                {
                    m_collisionInfo.MaskTiles.Add(new Point(
                        abs.X,
                        abs.Y));
                }
            }

            m_collisionInfo.OffsetX = 0;
            m_collisionInfo.OffsetY = 0;

            m_sprite.EntranceX = (int)numEntranceX.Value;
            m_sprite.EntranceY = (int)numEntranceY.Value;
            m_sprite.IsBuilding = chkIsBuilding.Checked;
        }

        private void btnAutoCollider_Click(object sender, EventArgs e)
        {
            if (m_sprite == null) return;

            float spriteW = m_sprite.OriginalBounds.Width;
            float spriteH = m_sprite.OriginalBounds.Height;

            int tilesWide = Math.Max(1, (int)Math.Ceiling(spriteW / NODE_TILE_W));
            int tilesHigh = Math.Max(1, (int)Math.Ceiling(spriteH / VISUAL_DIAMOND_H));

            numWidth.Value = Math.Min(numWidth.Maximum, tilesWide);
            numHeight.Value = Math.Min(numHeight.Maximum, tilesHigh);

            previewPanel.Invalidate();
        }

        private void btnResetCollision_Click(object sender, EventArgs e)
        {
            numWidth.Value = 1;
            numHeight.Value = 1;
            chkBlocksMovement.Checked = true;
            chkIsTrigger.Checked = false;
            m_colliderOffsetX = 0;
            m_colliderOffsetY = 0;
            m_selectedTiles.Clear();
            m_isSelectingTiles = false;
            btnSelectTiles.Text = "Select Tiles";
            m_collisionInfo.Width = 1;
            m_collisionInfo.Height = 1;
            m_collisionInfo.OffsetX = 0;
            m_collisionInfo.OffsetY = 0;
            m_collisionInfo.BlocksMovement = true;
            m_collisionInfo.IsTrigger = false;
            m_collisionInfo.MaskTiles.Clear();
            numEntranceX.Value = 0;
            numEntranceY.Value = 0;
            chkIsBuilding.Checked = false;
            previewPanel.Invalidate();
        }
    }
}
