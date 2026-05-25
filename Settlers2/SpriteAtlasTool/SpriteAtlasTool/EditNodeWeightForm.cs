using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Linq;
using System.Windows.Forms;

namespace SpriteAtlasTool
{
    public partial class EditNodeWeightForm : Form
    {
        private SpriteRegion m_sprite;
        private Bitmap m_spriteTexture;
        private List<NodeWeightEntry> m_entries = new List<NodeWeightEntry>();
        private byte m_activeWeight = 2; // Default: Land

        private const float NODE_TILE_W = 119.0f;
        private const float NODE_TILE_H = 74.0f;
        private const float HALF_NODE_W = 59.5f;
        private const float HALF_NODE_H = 37.0f;
        private float m_scale = 1.0f;
        private float m_drawX = 0.0f;
        private float m_drawY = 0.0f;
        private const int NY_COUNT = 4;
        private int GetNXCount(int ny) { return (ny % 2 == 0) ? 2 : 3; }

        public NodeWeightInfo NodeWeights
        {
            get
            {
                NodeWeightInfo info = new NodeWeightInfo();
                info.Entries.AddRange(m_entries);
                return info;
            }
        }

        public EditNodeWeightForm(SpriteRegion sprite, Bitmap texture)
        {
            InitializeComponent();
            m_sprite = sprite;
            Rectangle bounds = sprite.OriginalBounds;
            if (bounds.Width > 0 && bounds.Height > 0 && texture != null)
            {
                try
                {
                    m_spriteTexture = texture.Clone(bounds, texture.PixelFormat);
                }
                catch
                {
                    m_spriteTexture = texture;
                }
            }
            else
            {
                m_spriteTexture = texture;
            }
            if (sprite.NodeWeights != null)
            {
                foreach (var e in sprite.NodeWeights.Entries)
                    m_entries.Add(new NodeWeightEntry(e.NX, e.NY, e.Weight));
            }
            EnableDoubleBuffering();
            UpdateWeightButtons();
        }

        private void EnableDoubleBuffering()
        {
            typeof(Panel).InvokeMember("DoubleBuffered",
                System.Reflection.BindingFlags.SetProperty | System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.NonPublic,
                null, previewPanel, new object[] { true });
        }

        private void UpdateWeightButtons()
        {
            foreach (Control c in flowWeightPanel.Controls)
            {
                RadioButton rb = c as RadioButton;
                if (rb != null)
                {
                    rb.Checked = ((byte)rb.Tag == m_activeWeight);
                }
            }
            previewPanel.Invalidate();
        }

        private void WeightRadio_CheckedChanged(object sender, EventArgs e)
        {
            RadioButton rb = sender as RadioButton;
            if (rb != null && rb.Checked)
            {
                m_activeWeight = (byte)rb.Tag;
                previewPanel.Invalidate();
            }
        }

        private void ComputeLayout()
        {
            if (m_spriteTexture == null || m_sprite == null)
            {
                m_scale = 1.0f;
                m_drawX = previewPanel.Width * 0.5f;
                m_drawY = previewPanel.Height * 0.5f;
                return;
            }

            float spriteW = m_spriteTexture.Width;
            float spriteH = m_spriteTexture.Height;
            float panelW = previewPanel.Width;
            float panelH = previewPanel.Height;
            float margin = 30.0f;
            float maxW = panelW - margin * 2;
            float maxH = panelH - margin * 2;
            m_scale = 1.0f;
            if (spriteW > maxW || spriteH > maxH)
                m_scale = Math.Min(maxW / spriteW, maxH / spriteH);

            float drawW = spriteW * m_scale;
            float drawH = spriteH * m_scale;
            m_drawX = (panelW - drawW) * 0.5f;
            m_drawY = (panelH - drawH) * 0.5f;
        }

        private void TileToScreen(int nx, int ny, out float sx, out float sy)
        {
            float nodeW = NODE_TILE_W * m_scale;
            float halfW = HALF_NODE_W * m_scale;
            float halfH = HALF_NODE_H * m_scale;
            sx = m_drawX + nx * nodeW + ((ny % 2 == 0) ? halfW : 0.0f);
            sy = m_drawY + ny * halfH + halfH;
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

        private static Color WeightToColor(byte weight)
        {
            switch (weight)
            {
                case 0: return Color.FromArgb(180, 50, 100, 255);  // Deep
                case 1: return Color.FromArgb(180, 50, 255, 255);  // Shallow
                case 2: return Color.FromArgb(180, 100, 220, 100); // Land
                case 3: return Color.FromArgb(180, 255, 80, 80);   // Block
                default: return Color.FromArgb(180, 100, 100, 100);
            }
        }

        private byte GetWeightAt(int nx, int ny)
        {
            foreach (var e in m_entries)
                if (e.NX == nx && e.NY == ny)
                    return e.Weight;
            return 2; // default Land
        }

        private void SetWeightAt(int nx, int ny, byte weight)
        {
            for (int i = 0; i < m_entries.Count; i++)
            {
                if (m_entries[i].NX == nx && m_entries[i].NY == ny)
                {
                    if (weight == 2) // Land is default, remove entry
                    {
                        m_entries.RemoveAt(i);
                    }
                    else
                    {
                        m_entries[i] = new NodeWeightEntry(nx, ny, weight);
                    }
                    return;
                }
            }
            if (weight != 2)
                m_entries.Add(new NodeWeightEntry(nx, ny, weight));
        }

        private void previewPanel_MouseClick(object sender, MouseEventArgs e)
        {
            ComputeLayout();

            float halfW = HALF_NODE_W * m_scale;
            float halfH = HALF_NODE_H * m_scale;

            for (int ny = 0; ny < NY_COUNT; ny++)
            {
                for (int nx = 0; nx < GetNXCount(ny); nx++)
                {
                    float sx, sy;
                    TileToScreen(nx, ny, out sx, out sy);

                    if (IsPointInDiamond(e.X, e.Y, sx, sy, halfW, halfH))
                    {
                        if (e.Button == MouseButtons.Left)
                        {
                            SetWeightAt(nx, ny, m_activeWeight);
                        }
                        else if (e.Button == MouseButtons.Right)
                        {
                            for (int i = 0; i < m_entries.Count; i++)
                            {
                                if (m_entries[i].NX == nx && m_entries[i].NY == ny)
                                {
                                    m_entries.RemoveAt(i);
                                    break;
                                }
                            }
                        }
                        previewPanel.Invalidate();
                        return;
                    }
                }
            }
        }

        private void DrawNodeGrid(Graphics graphics)
        {
            float halfW = HALF_NODE_W * m_scale;
            float halfH = HALF_NODE_H * m_scale;
            using (Pen gridPen = new Pen(Color.FromArgb(80, 80, 80), 1.0f))
            {
                for (int ny = 0; ny < NY_COUNT; ny++)
                {
                    for (int nx = 0; nx < GetNXCount(ny); nx++)
                    {
                        float sx, sy;
                        TileToScreen(nx, ny, out sx, out sy);

                        byte weight = GetWeightAt(nx, ny);
                        using (Brush fillBrush = new SolidBrush(WeightToColor(weight)))
                        {
                            PointF[] pts = GetDiamondPoints(sx, sy, halfW, halfH);
                            graphics.FillPolygon(fillBrush, pts);
                        }

                        graphics.DrawPolygon(gridPen, GetDiamondPoints(sx, sy, halfW, halfH));

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

        private void DrawSpriteTexture(Graphics graphics)
        {
            if (m_spriteTexture == null || m_sprite == null) return;

            float spriteW = m_spriteTexture.Width;
            float spriteH = m_spriteTexture.Height;
            float drawW = spriteW * m_scale;
            float drawH = spriteH * m_scale;
            using (ImageAttributes attr = new ImageAttributes())
            {
                ColorMatrix cm = new ColorMatrix();
                cm.Matrix33 = 0.85f;
                attr.SetColorMatrix(cm, ColorMatrixFlag.Default, ColorAdjustType.Bitmap);
                graphics.DrawImage(m_spriteTexture,
                    new Rectangle((int)m_drawX, (int)m_drawY, (int)drawW, (int)drawH),
                    0, 0, spriteW, spriteH,
                    GraphicsUnit.Pixel, attr);
            }

            using (Pen bboxPen = new Pen(Color.Yellow, 1.0f))
                graphics.DrawRectangle(bboxPen, m_drawX, m_drawY, drawW, drawH);

            float pivotX = m_sprite.HasPivot ? m_sprite.Pivot.X : 0;
            float pivotY = m_sprite.HasPivot ? m_sprite.Pivot.Y : 0;
            float pvx = m_drawX + pivotX * m_scale;
            float pvy = m_drawY + pivotY * m_scale;
            using (Pen pivotPen = new Pen(Color.Magenta, 2.0f))
            {
                graphics.DrawLine(pivotPen, pvx - 8, pvy, pvx + 8, pvy);
                graphics.DrawLine(pivotPen, pvx, pvy - 8, pvx, pvy + 8);
            }

            using (Font font = new Font("Arial", 8))
            using (Brush textBrush = new SolidBrush(Color.Cyan))
            {
                string info = string.Format("Sprite: {0}x{1}px", (int)spriteW, (int)spriteH);
                graphics.DrawString(info, font, textBrush, 10, 30);
            }
        }

        private void previewPanel_Paint(object sender, PaintEventArgs e)
        {
            e.Graphics.SmoothingMode = SmoothingMode.HighQuality;
            e.Graphics.InterpolationMode = InterpolationMode.HighQualityBilinear;
            e.Graphics.PixelOffsetMode = PixelOffsetMode.Default;
            e.Graphics.Clear(Color.FromArgb(20, 20, 20));

            ComputeLayout();

            DrawSpriteTexture(e.Graphics);
            DrawNodeGrid(e.Graphics);
        }

        private void btnClearAll_Click(object sender, EventArgs e)
        {
            m_entries.Clear();
            previewPanel.Invalidate();
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            // entries already stored in m_entries
        }
    }
}
