using System.Windows.Forms;
namespace SpriteAtlasTool
{
    partial class EditCollisionForm
    {
        private CollisionInfo m_collisionInfo;
        private NumericUpDown numWidth;
        private NumericUpDown numHeight;
        private CheckBox chkBlocksMovement;
        private CheckBox chkIsTrigger;
        private Button btnOK;
        private Button btnCancel;
        private GroupBox groupBox1;
        private Label lblPreview;
        private Panel previewPanel;
        private ToolTip toolTip1;

        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.chkIsTrigger = new System.Windows.Forms.CheckBox();
            this.chkBlocksMovement = new System.Windows.Forms.CheckBox();
            this.numHeight = new System.Windows.Forms.NumericUpDown();
            this.numWidth = new System.Windows.Forms.NumericUpDown();
            this.lblPreview = new System.Windows.Forms.Label();
            this.previewPanel = new System.Windows.Forms.Panel();
            this.btnOK = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.btnAutoCollider = new System.Windows.Forms.Button();
            this.groupBox1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numHeight)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numWidth)).BeginInit();
            this.SuspendLayout();
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.chkIsTrigger);
            this.groupBox1.Controls.Add(this.chkBlocksMovement);
            this.groupBox1.Controls.Add(this.numHeight);
            this.groupBox1.Controls.Add(this.numWidth);
            this.groupBox1.Location = new System.Drawing.Point(12, 12);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(260, 120);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Collision Parameters";
            // 
            // chkIsTrigger
            // 
            this.chkIsTrigger.AutoSize = true;
            this.chkIsTrigger.Location = new System.Drawing.Point(130, 60);
            this.chkIsTrigger.Name = "chkIsTrigger";
            this.chkIsTrigger.Size = new System.Drawing.Size(70, 17);
            this.chkIsTrigger.TabIndex = 3;
            this.chkIsTrigger.Text = "Is Trigger";
            this.toolTip1.SetToolTip(this.chkIsTrigger, "Does not block movement, but triggers events");
            this.chkIsTrigger.UseVisualStyleBackColor = true;
            // 
            // chkBlocksMovement
            // 
            this.chkBlocksMovement.AutoSize = true;
            this.chkBlocksMovement.Location = new System.Drawing.Point(130, 30);
            this.chkBlocksMovement.Name = "chkBlocksMovement";
            this.chkBlocksMovement.Size = new System.Drawing.Size(111, 17);
            this.chkBlocksMovement.TabIndex = 2;
            this.chkBlocksMovement.Text = "Blocks Movement";
            this.toolTip1.SetToolTip(this.chkBlocksMovement, "Prevents units from walking through");
            this.chkBlocksMovement.UseVisualStyleBackColor = true;
            // 
            // numHeight
            // 
            this.numHeight.Location = new System.Drawing.Point(70, 60);
            this.numHeight.Maximum = new decimal(new int[] {
            10,
            0,
            0,
            0});
            this.numHeight.Name = "numHeight";
            this.numHeight.Size = new System.Drawing.Size(50, 20);
            this.numHeight.TabIndex = 1;
            this.numHeight.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.numHeight.ValueChanged += new System.EventHandler(this.ValuesChanged);
            // 
            // numWidth
            // 
            this.numWidth.Location = new System.Drawing.Point(10, 60);
            this.numWidth.Maximum = new decimal(new int[] {
            10,
            0,
            0,
            0});
            this.numWidth.Name = "numWidth";
            this.numWidth.Size = new System.Drawing.Size(50, 20);
            this.numWidth.TabIndex = 0;
            this.numWidth.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.numWidth.ValueChanged += new System.EventHandler(this.ValuesChanged);
            // 
            // lblPreview
            // 
            this.lblPreview.AutoSize = true;
            this.lblPreview.Location = new System.Drawing.Point(12, 140);
            this.lblPreview.Name = "lblPreview";
            this.lblPreview.Size = new System.Drawing.Size(48, 13);
            this.lblPreview.TabIndex = 1;
            this.lblPreview.Text = "Preview:";
            // 
            // previewPanel
            // 
            this.previewPanel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.previewPanel.Location = new System.Drawing.Point(12, 156);
            this.previewPanel.Name = "previewPanel";
            this.previewPanel.Size = new System.Drawing.Size(640, 457);
            this.previewPanel.TabIndex = 2;
            this.previewPanel.Paint += new System.Windows.Forms.PaintEventHandler(this.previewPanel_Paint);
            // 
            // btnOK
            // 
            this.btnOK.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnOK.Location = new System.Drawing.Point(32, 643);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(75, 23);
            this.btnOK.TabIndex = 3;
            this.btnOK.Text = "OK";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            // 
            // btnCancel
            // 
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(142, 643);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 4;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            // 
            // btnAutoCollider
            // 
            this.btnAutoCollider.Location = new System.Drawing.Point(270, 643);
            this.btnAutoCollider.Name = "btnAutoCollider";
            this.btnAutoCollider.Size = new System.Drawing.Size(75, 23);
            this.btnAutoCollider.TabIndex = 5;
            this.btnAutoCollider.Text = "AutoCollider";
            this.btnAutoCollider.UseVisualStyleBackColor = true;
            this.btnAutoCollider.Click += new System.EventHandler(this.btnAutoCollider_Click);
            // Entrance controls (to the right of collision group box)
            this.lblEntrance = new System.Windows.Forms.Label();
            this.lblEntrance.AutoSize = true;
            this.lblEntrance.Location = new System.Drawing.Point(285, 12);
            this.lblEntrance.Name = "lblEntrance";
            this.lblEntrance.Size = new System.Drawing.Size(90, 13);
            this.lblEntrance.Text = "Entrance (X, Y):";
            this.Controls.Add(this.lblEntrance);

            this.numEntranceX = new System.Windows.Forms.NumericUpDown();
            this.numEntranceX.Location = new System.Drawing.Point(285, 30);
            this.numEntranceX.Minimum = -20;
            this.numEntranceX.Maximum = 20;
            this.numEntranceX.Name = "numEntranceX";
            this.numEntranceX.Size = new System.Drawing.Size(50, 20);
            this.numEntranceX.TabIndex = 6;
            this.numEntranceX.ValueChanged += new System.EventHandler(this.ValuesChanged);
            this.Controls.Add(this.numEntranceX);

            this.numEntranceY = new System.Windows.Forms.NumericUpDown();
            this.numEntranceY.Location = new System.Drawing.Point(345, 30);
            this.numEntranceY.Minimum = -20;
            this.numEntranceY.Maximum = 20;
            this.numEntranceY.Name = "numEntranceY";
            this.numEntranceY.Size = new System.Drawing.Size(50, 20);
            this.numEntranceY.TabIndex = 7;
            this.numEntranceY.ValueChanged += new System.EventHandler(this.ValuesChanged);
            this.Controls.Add(this.numEntranceY);

            // Select Entrance button
            this.btnSelectEntrance = new System.Windows.Forms.Button();
            this.btnSelectEntrance.Location = new System.Drawing.Point(285, 55);
            this.btnSelectEntrance.Name = "btnSelectEntrance";
            this.btnSelectEntrance.Size = new System.Drawing.Size(110, 23);
            this.btnSelectEntrance.TabIndex = 8;
            this.btnSelectEntrance.Text = "Select Entrance";
            this.btnSelectEntrance.UseVisualStyleBackColor = true;
            this.btnSelectEntrance.Click += new System.EventHandler(this.btnSelectEntrance_Click);
            this.Controls.Add(this.btnSelectEntrance);

            // Entrance info label
            this.lblEntranceInfo = new System.Windows.Forms.Label();
            this.lblEntranceInfo.AutoSize = true;
            this.lblEntranceInfo.Location = new System.Drawing.Point(285, 82);
            this.lblEntranceInfo.Name = "lblEntranceInfo";
            this.lblEntranceInfo.Size = new System.Drawing.Size(100, 13);
            this.lblEntranceInfo.Text = "Click grid to set tile";
            this.Controls.Add(this.lblEntranceInfo);

            // Is Building checkbox
            this.chkIsBuilding = new System.Windows.Forms.CheckBox();
            this.chkIsBuilding.AutoSize = true;
            this.chkIsBuilding.Location = new System.Drawing.Point(285, 100);
            this.chkIsBuilding.Name = "chkIsBuilding";
            this.chkIsBuilding.Size = new System.Drawing.Size(84, 17);
            this.chkIsBuilding.TabIndex = 9;
            this.chkIsBuilding.Text = "Is Building";
            this.toolTip1.SetToolTip(this.chkIsBuilding, "Marks this sprite as a gameplay building");
            this.chkIsBuilding.UseVisualStyleBackColor = true;
            this.Controls.Add(this.chkIsBuilding);

            // После btnCancel
            this.btnSelectTiles = new System.Windows.Forms.Button();
            this.btnSelectTiles.Location = new System.Drawing.Point(12, 270);
            this.btnSelectTiles.Size = new System.Drawing.Size(100, 23);
            this.btnSelectTiles.Text = "Select Tiles";
            this.btnSelectTiles.UseVisualStyleBackColor = true;
            this.btnSelectTiles.Click += new System.EventHandler(this.btnSelectTiles_Click);
            this.Controls.Add(this.btnSelectTiles);

            this.btnResetCollision = new System.Windows.Forms.Button();
            this.btnResetCollision.Location = new System.Drawing.Point(120, 270);
            this.btnResetCollision.Size = new System.Drawing.Size(100, 23);
            this.btnResetCollision.Text = "Reset Collision";
            this.btnResetCollision.UseVisualStyleBackColor = true;
            this.btnResetCollision.Click += new System.EventHandler(this.btnResetCollision_Click);
            this.Controls.Add(this.btnResetCollision);

            // 
            // EditCollisionForm
            // 
            this.AcceptButton = this.btnOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(675, 687);
            this.Controls.Add(this.btnAutoCollider);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.previewPanel);
            this.Controls.Add(this.lblPreview);
            this.Controls.Add(this.groupBox1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "EditCollisionForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Edit Collision";
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numHeight)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numWidth)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Button btnAutoCollider;
        private Button btnResetCollision;
        private NumericUpDown numEntranceX;
        private NumericUpDown numEntranceY;
        private Label lblEntrance;
        private CheckBox chkIsBuilding;
        private Button btnSelectEntrance;
        private Label lblEntranceInfo;
    }
}