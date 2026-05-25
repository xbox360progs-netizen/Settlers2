using System.Drawing;
using System.Windows.Forms;

namespace SpriteAtlasTool
{
    partial class EditNodeWeightForm
    {
        private System.ComponentModel.IContainer components = null;
        private Panel previewPanel;
        private FlowLayoutPanel flowWeightPanel;
        private Button btnClearAll;
        private Button btnOK;
        private Button btnCancel;
        private Label lblStatus;

        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        private void InitializeComponent()
        {
            this.previewPanel = new System.Windows.Forms.Panel();
            this.flowWeightPanel = new System.Windows.Forms.FlowLayoutPanel();
            this.btnClearAll = new System.Windows.Forms.Button();
            this.btnOK = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.lblStatus = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // previewPanel
            // 
            this.previewPanel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.previewPanel.Location = new System.Drawing.Point(12, 12);
            this.previewPanel.Name = "previewPanel";
            this.previewPanel.Size = new System.Drawing.Size(640, 457);
            this.previewPanel.TabIndex = 0;
            this.previewPanel.Paint += new System.Windows.Forms.PaintEventHandler(this.previewPanel_Paint);
            this.previewPanel.MouseClick += new System.Windows.Forms.MouseEventHandler(this.previewPanel_MouseClick);
            // 
            // flowWeightPanel
            // 
            this.flowWeightPanel.FlowDirection = System.Windows.Forms.FlowDirection.TopDown;
            this.flowWeightPanel.Location = new System.Drawing.Point(660, 12);
            this.flowWeightPanel.Name = "flowWeightPanel";
            this.flowWeightPanel.Size = new System.Drawing.Size(130, 200);
            this.flowWeightPanel.TabIndex = 1;
            // 
            // btnClearAll
            // 
            this.btnClearAll.Location = new System.Drawing.Point(660, 220);
            this.btnClearAll.Name = "btnClearAll";
            this.btnClearAll.Size = new System.Drawing.Size(130, 23);
            this.btnClearAll.TabIndex = 2;
            this.btnClearAll.Text = "Clear All";
            this.btnClearAll.UseVisualStyleBackColor = true;
            this.btnClearAll.Click += new System.EventHandler(this.btnClearAll_Click);
            // 
            // btnOK
            // 
            this.btnOK.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnOK.Location = new System.Drawing.Point(660, 420);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(60, 23);
            this.btnOK.TabIndex = 3;
            this.btnOK.Text = "OK";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            // 
            // btnCancel
            // 
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(730, 420);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(60, 23);
            this.btnCancel.TabIndex = 4;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            // 
            // lblStatus
            // 
            this.lblStatus.AutoSize = true;
            this.lblStatus.Location = new System.Drawing.Point(12, 475);
            this.lblStatus.Name = "lblStatus";
            this.lblStatus.Size = new System.Drawing.Size(260, 13);
            this.lblStatus.TabIndex = 5;
            this.lblStatus.Text = "Left-click: paint weight   Right-click: remove entry";
            // 
            // EditNodeWeightForm
            // 
            this.AcceptButton = this.btnOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(804, 500);
            this.Controls.Add(this.lblStatus);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.btnClearAll);
            this.Controls.Add(this.flowWeightPanel);
            this.Controls.Add(this.previewPanel);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "EditNodeWeightForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Edit Node Weights";
            this.ResumeLayout(false);
            this.PerformLayout();

            // Add weight radio buttons programmatically
            AddWeightButton("Deep (0)", 0, Color.FromArgb(50, 100, 255));
            AddWeightButton("Shallow (1)", 1, Color.FromArgb(50, 255, 255));
            AddWeightButton("Land (2)", 2, Color.FromArgb(100, 220, 100));
            AddWeightButton("Block (3)", 3, Color.FromArgb(255, 80, 80));
        }

        private void AddWeightButton(string text, byte weight, Color color)
        {
            RadioButton rb = new RadioButton();
            rb.Text = text;
            rb.Tag = weight;
            rb.ForeColor = color;
            rb.Font = new System.Drawing.Font("Arial", 9, System.Drawing.FontStyle.Bold);
            rb.Checked = (weight == 2);
            rb.CheckedChanged += new System.EventHandler(this.WeightRadio_CheckedChanged);
            this.flowWeightPanel.Controls.Add(rb);
        }
    }
}
