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
    public partial class RenameSpriteForm : Form
    {
        public string NewName { get; set; }

        public RenameSpriteForm(string currentName)
        {
            InitializeComponent();
            NewName = currentName;
            textBoxName.Text = currentName;
        }

        private void RenameSpriteForm_Load(object sender, EventArgs e)
        {
            textBoxName.SelectAll();
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            NewName = textBoxName.Text.Trim();
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
