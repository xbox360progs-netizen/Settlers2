using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace SpriteAtlasTool
{
    public partial class MirrorOptionsForm : Form
    {
        public bool MirrorX { get; private set; }
        public bool MirrorY { get; private set; }

        public MirrorOptionsForm()
        {
            InitializeComponent();
        }

        private void MirrorOptionsForm_Load(object sender, EventArgs e)
        {
            // По умолчанию выбираем зеркалирование по X
            chkMirrorX.Checked = true;
            chkMirrorY.Checked = false;
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            MirrorX = chkMirrorX.Checked;
            MirrorY = chkMirrorY.Checked;

            // Проверяем, что хотя бы одна опция выбрана
            if (!MirrorX && !MirrorY)
            {
                MessageBox.Show("Please select at least one mirror option!", "Mirror Options",
                    MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            DialogResult = DialogResult.OK;
            Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
            Close();
        }
    }
}
