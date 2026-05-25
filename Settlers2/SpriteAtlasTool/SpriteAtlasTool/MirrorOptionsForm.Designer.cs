using System;
using System.Windows.Forms;
namespace SpriteAtlasTool
{
    partial class MirrorOptionsForm
    {
        private System.ComponentModel.IContainer components = null;
        private CheckBox chkMirrorX;
        private CheckBox chkMirrorY;
        private Button btnOK;
        private Button btnCancel;
        private Label label1;

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
            this.chkMirrorX = new CheckBox();
            this.chkMirrorY = new CheckBox();
            this.btnOK = new Button();
            this.btnCancel = new Button();
            this.label1 = new Label();
            this.SuspendLayout();

            // label1
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 15);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(147, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Select mirror transformation:";

            // chkMirrorX
            this.chkMirrorX.AutoSize = true;
            this.chkMirrorX.Location = new System.Drawing.Point(15, 40);
            this.chkMirrorX.Name = "chkMirrorX";
            this.chkMirrorX.Size = new System.Drawing.Size(107, 17);
            this.chkMirrorX.TabIndex = 1;
            this.chkMirrorX.Text = "Mirror Horizontally";
            this.chkMirrorX.UseVisualStyleBackColor = true;

            // chkMirrorY
            this.chkMirrorY.AutoSize = true;
            this.chkMirrorY.Location = new System.Drawing.Point(15, 65);
            this.chkMirrorY.Name = "chkMirrorY";
            this.chkMirrorY.Size = new System.Drawing.Size(96, 17);
            this.chkMirrorY.TabIndex = 2;
            this.chkMirrorY.Text = "Mirror Vertically";
            this.chkMirrorY.UseVisualStyleBackColor = true;

            // btnOK
            this.btnOK.Location = new System.Drawing.Point(62, 100);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(75, 23);
            this.btnOK.TabIndex = 3;
            this.btnOK.Text = "OK";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new EventHandler(this.btnOK_Click);

            // btnCancel
            this.btnCancel.DialogResult = DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(143, 100);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 4;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new EventHandler(this.btnCancel_Click);

            // MirrorOptionsForm
            this.AcceptButton = this.btnOK;
            this.CancelButton = this.btnCancel;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(230, 135);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.chkMirrorY);
            this.Controls.Add(this.chkMirrorX);
            this.Controls.Add(this.label1);
            this.FormBorderStyle = FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "MirrorOptionsForm";
            this.StartPosition = FormStartPosition.CenterParent;
            this.Text = "Mirror Sprite";
            this.ResumeLayout(false);
            this.PerformLayout();
        }
    }
}