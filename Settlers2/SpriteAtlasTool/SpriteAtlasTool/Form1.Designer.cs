using System.Drawing;
using System.Windows.Forms;
namespace SpriteAtlasTool
{
    partial class Form1
    {
        private System.ComponentModel.IContainer components = null;

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (components != null)
                {
                    components.Dispose();
                }
                if (animationTimer != null)
                {
                    animationTimer.Dispose();
                }
            }
            base.Dispose(disposing);
        }

        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.previewPanel = new System.Windows.Forms.Panel();
            this.previewBox = new System.Windows.Forms.PictureBox();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.buttonLoad = new System.Windows.Forms.Button();
            this.buttonSave = new System.Windows.Forms.Button();
            this.lstSprites = new System.Windows.Forms.ListBox();
            this.btnAddRect = new System.Windows.Forms.Button();
            this.btnDeleteRect = new System.Windows.Forms.Button();
            this.lblInfo = new System.Windows.Forms.Label();
            this.panelAtlas = new System.Windows.Forms.Panel();
            this.btnCenterPivot = new System.Windows.Forms.Button();
            this.btnZoomIn = new System.Windows.Forms.Button();
            this.btnZoomOut = new System.Windows.Forms.Button();
            this.btnZoomReset = new System.Windows.Forms.Button();
            this.btnEditRect = new System.Windows.Forms.Button();
            this.btnFillRect = new System.Windows.Forms.Button();
            this.lblFileName = new System.Windows.Forms.Label();
            this.btnSaveDefaultBin = new System.Windows.Forms.Button();
            this.buttonLoadBin = new System.Windows.Forms.Button();
            this.lstGroups = new System.Windows.Forms.ListBox();
            this.txtGroupName = new System.Windows.Forms.TextBox();
            this.btnAddGroup = new System.Windows.Forms.Button();
            this.btnRemoveGroup = new System.Windows.Forms.Button();
            this.btnAddToGroup = new System.Windows.Forms.Button();
            this.groupBoxAtlasType = new System.Windows.Forms.GroupBox();
            this.labelSpritesPerLevel = new System.Windows.Forms.Label();
            this.numericUpDownSpritesPerLevel = new System.Windows.Forms.NumericUpDown();
            this.comboBoxAtlasType = new System.Windows.Forms.ComboBox();
            this.btnPlayAnimation = new System.Windows.Forms.Button();
            this.btnStopAnimation = new System.Windows.Forms.Button();
            this.btnAutoPack = new System.Windows.Forms.Button();
            this.animationTimer = new System.Windows.Forms.Timer(this.components);
            this.btnCenterPivotAnimation = new System.Windows.Forms.Button();
            this.btnCenterSprite = new System.Windows.Forms.Button();
            this.chkShowGuides = new System.Windows.Forms.CheckBox();
            this.btnMoveUp = new System.Windows.Forms.Button();
            this.btnMoveDown = new System.Windows.Forms.Button();
            this.btnExportUV = new System.Windows.Forms.Button();
            this.btnDeleteSelected = new System.Windows.Forms.Button();
            this.chkFillUVArea = new System.Windows.Forms.CheckBox();
            this.menuStrip1 = new System.Windows.Forms.MenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.openTextureToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.openAtlasToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.saveToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.saveDefaultToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.fileCloseToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.exitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.viewToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.showGuidesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.fillUVAreaToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.showPivotGuidesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.uVToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.autoPackToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.exportUVToolStripMenuItem1 = new System.Windows.Forms.ToolStripMenuItem();
            this.btnPreviewZoomIn = new System.Windows.Forms.Button();
            this.btnPreviewZoomOut = new System.Windows.Forms.Button();
            this.btnPreviewZoomReset = new System.Windows.Forms.Button();
            this.chkShowPivotGuides = new System.Windows.Forms.CheckBox();
            this.btnPivotTopLeft = new System.Windows.Forms.Button();
            this.btnEditCollision = new System.Windows.Forms.Button();
            this.btnEditNodeWeight = new System.Windows.Forms.Button();
            this.btnCreateMirror = new System.Windows.Forms.Button();
            this.btnAutoNameSprites = new System.Windows.Forms.Button();
            this.editNameToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mirrorToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.xToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.yToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.loadDefaultToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.panel1 = new System.Windows.Forms.Panel();
            this.panel2 = new System.Windows.Forms.Panel();
            this.previewPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.previewBox)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.panelAtlas.SuspendLayout();
            this.groupBoxAtlasType.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numericUpDownSpritesPerLevel)).BeginInit();
            this.menuStrip1.SuspendLayout();
            this.panel1.SuspendLayout();
            this.panel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // previewPanel
            // 
            this.previewPanel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.previewPanel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.previewPanel.Controls.Add(this.previewBox);
            this.previewPanel.Location = new System.Drawing.Point(1, 346);
            this.previewPanel.Name = "previewPanel";
            this.previewPanel.Size = new System.Drawing.Size(587, 400);
            this.previewPanel.TabIndex = 0;
            // 
            // previewBox
            // 
            this.previewBox.BackColor = System.Drawing.Color.White;
            this.previewBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.previewBox.Location = new System.Drawing.Point(0, 0);
            this.previewBox.Name = "previewBox";
            this.previewBox.Size = new System.Drawing.Size(585, 398);
            this.previewBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.AutoSize;
            this.previewBox.TabIndex = 0;
            this.previewBox.TabStop = false;
            this.previewBox.Paint += new System.Windows.Forms.PaintEventHandler(this.previewBox_Paint);
            // 
            // pictureBox1
            // 
            this.pictureBox1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.pictureBox1.Location = new System.Drawing.Point(-1, 5);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(865, 723);
            this.pictureBox1.SizeMode = System.Windows.Forms.PictureBoxSizeMode.AutoSize;
            this.pictureBox1.TabIndex = 0;
            this.pictureBox1.TabStop = false;
            this.pictureBox1.Paint += new System.Windows.Forms.PaintEventHandler(this.pictureBox1_Paint);
            this.pictureBox1.MouseClick += new System.Windows.Forms.MouseEventHandler(this.pictureBox1_MouseClick);
            this.pictureBox1.MouseDown += new System.Windows.Forms.MouseEventHandler(this.pictureBox1_MouseDown);
            this.pictureBox1.MouseMove += new System.Windows.Forms.MouseEventHandler(this.pictureBox1_MouseMove);
            this.pictureBox1.MouseUp += new System.Windows.Forms.MouseEventHandler(this.pictureBox1_MouseUp);
            // 
            // buttonLoad
            // 
            this.buttonLoad.Location = new System.Drawing.Point(533, 0);
            this.buttonLoad.Name = "buttonLoad";
            this.buttonLoad.Size = new System.Drawing.Size(64, 23);
            this.buttonLoad.TabIndex = 1;
            this.buttonLoad.Text = "Load Pic";
            this.buttonLoad.UseVisualStyleBackColor = true;
            this.buttonLoad.Click += new System.EventHandler(this.buttonLoad_Click);
            // 
            // buttonSave
            // 
            this.buttonSave.Location = new System.Drawing.Point(261, 0);
            this.buttonSave.Name = "buttonSave";
            this.buttonSave.Size = new System.Drawing.Size(65, 23);
            this.buttonSave.TabIndex = 2;
            this.buttonSave.Text = "Save .bin";
            this.buttonSave.UseVisualStyleBackColor = true;
            this.buttonSave.Visible = false;
            this.buttonSave.Click += new System.EventHandler(this.buttonSave_Click);
            // 
            // lstSprites
            // 
            this.lstSprites.FormattingEnabled = true;
            this.lstSprites.Location = new System.Drawing.Point(314, 32);
            this.lstSprites.Name = "lstSprites";
            this.lstSprites.Size = new System.Drawing.Size(247, 277);
            this.lstSprites.TabIndex = 3;
            this.lstSprites.SelectedIndexChanged += new System.EventHandler(this.lstSprites_SelectedIndexChanged);
            // 
            // btnAddRect
            // 
            this.btnAddRect.Location = new System.Drawing.Point(3, 157);
            this.btnAddRect.Name = "btnAddRect";
            this.btnAddRect.Size = new System.Drawing.Size(105, 23);
            this.btnAddRect.TabIndex = 4;
            this.btnAddRect.Text = "Add Rect";
            this.btnAddRect.UseVisualStyleBackColor = true;
            this.btnAddRect.Visible = false;
            this.btnAddRect.Click += new System.EventHandler(this.btnAddRect_Click);
            // 
            // btnDeleteRect
            // 
            this.btnDeleteRect.Location = new System.Drawing.Point(524, 317);
            this.btnDeleteRect.Name = "btnDeleteRect";
            this.btnDeleteRect.Size = new System.Drawing.Size(73, 23);
            this.btnDeleteRect.TabIndex = 5;
            this.btnDeleteRect.Text = "Delete";
            this.btnDeleteRect.UseVisualStyleBackColor = true;
            this.btnDeleteRect.Visible = false;
            this.btnDeleteRect.Click += new System.EventHandler(this.btnDeleteRect_Click);
            // 
            // lblInfo
            // 
            this.lblInfo.AutoSize = true;
            this.lblInfo.Location = new System.Drawing.Point(3, 330);
            this.lblInfo.Name = "lblInfo";
            this.lblInfo.Size = new System.Drawing.Size(38, 13);
            this.lblInfo.TabIndex = 6;
            this.lblInfo.Text = "Ready";
            // 
            // panelAtlas
            // 
            this.panelAtlas.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.panelAtlas.AutoScroll = true;
            this.panelAtlas.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.panelAtlas.Controls.Add(this.pictureBox1);
            this.panelAtlas.Location = new System.Drawing.Point(12, 25);
            this.panelAtlas.Name = "panelAtlas";
            this.panelAtlas.Size = new System.Drawing.Size(853, 722);
            this.panelAtlas.TabIndex = 7;
            // 
            // btnCenterPivot
            // 
            this.btnCenterPivot.Location = new System.Drawing.Point(3, 70);
            this.btnCenterPivot.Name = "btnCenterPivot";
            this.btnCenterPivot.Size = new System.Drawing.Size(105, 23);
            this.btnCenterPivot.TabIndex = 8;
            this.btnCenterPivot.Text = "Center Pivot";
            this.btnCenterPivot.UseVisualStyleBackColor = true;
            this.btnCenterPivot.Click += new System.EventHandler(this.btnCenterPivot_Click);
            // 
            // btnZoomIn
            // 
            this.btnZoomIn.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.btnZoomIn.Location = new System.Drawing.Point(3, 0);
            this.btnZoomIn.Name = "btnZoomIn";
            this.btnZoomIn.Size = new System.Drawing.Size(111, 23);
            this.btnZoomIn.TabIndex = 9;
            this.btnZoomIn.Text = "Zoom+";
            this.btnZoomIn.UseVisualStyleBackColor = true;
            this.btnZoomIn.Click += new System.EventHandler(this.btnZoomIn_Click);
            // 
            // btnZoomOut
            // 
            this.btnZoomOut.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.btnZoomOut.Location = new System.Drawing.Point(147, 1);
            this.btnZoomOut.Name = "btnZoomOut";
            this.btnZoomOut.Size = new System.Drawing.Size(137, 23);
            this.btnZoomOut.TabIndex = 10;
            this.btnZoomOut.Text = "Zoom-";
            this.btnZoomOut.UseVisualStyleBackColor = true;
            this.btnZoomOut.Click += new System.EventHandler(this.btnZoomOut_Click);
            // 
            // btnZoomReset
            // 
            this.btnZoomReset.Anchor = System.Windows.Forms.AnchorStyles.Right;
            this.btnZoomReset.Location = new System.Drawing.Point(722, -1);
            this.btnZoomReset.Name = "btnZoomReset";
            this.btnZoomReset.Size = new System.Drawing.Size(130, 23);
            this.btnZoomReset.TabIndex = 11;
            this.btnZoomReset.Text = "Reset Zoom";
            this.btnZoomReset.UseVisualStyleBackColor = true;
            this.btnZoomReset.Click += new System.EventHandler(this.btnZoomReset_Click);
            // 
            // btnEditRect
            // 
            this.btnEditRect.Location = new System.Drawing.Point(3, 99);
            this.btnEditRect.Name = "btnEditRect";
            this.btnEditRect.Size = new System.Drawing.Size(105, 23);
            this.btnEditRect.TabIndex = 12;
            this.btnEditRect.Text = "Edit Rectangle";
            this.btnEditRect.UseVisualStyleBackColor = true;
            this.btnEditRect.Click += new System.EventHandler(this.btnEditRect_Click);
            // 
            // btnFillRect
            // 
            this.btnFillRect.Location = new System.Drawing.Point(3, 128);
            this.btnFillRect.Name = "btnFillRect";
            this.btnFillRect.Size = new System.Drawing.Size(105, 23);
            this.btnFillRect.TabIndex = 13;
            this.btnFillRect.Text = "Fill Rect";
            this.btnFillRect.UseVisualStyleBackColor = true;
            this.btnFillRect.Click += new System.EventHandler(this.btnFillRect_Click);
            // 
            // lblFileName
            // 
            this.lblFileName.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.lblFileName.AutoSize = true;
            this.lblFileName.Location = new System.Drawing.Point(379, 9);
            this.lblFileName.Name = "lblFileName";
            this.lblFileName.Size = new System.Drawing.Size(72, 13);
            this.lblFileName.TabIndex = 14;
            this.lblFileName.Text = "No file loaded";
            // 
            // btnSaveDefaultBin
            // 
            this.btnSaveDefaultBin.Location = new System.Drawing.Point(427, 0);
            this.btnSaveDefaultBin.Name = "btnSaveDefaultBin";
            this.btnSaveDefaultBin.Size = new System.Drawing.Size(100, 23);
            this.btnSaveDefaultBin.TabIndex = 15;
            this.btnSaveDefaultBin.Text = "Save Default .bin";
            this.btnSaveDefaultBin.UseVisualStyleBackColor = true;
            this.btnSaveDefaultBin.Click += new System.EventHandler(this.btnSaveDefaultBin_Click);
            // 
            // buttonLoadBin
            // 
            this.buttonLoadBin.Location = new System.Drawing.Point(354, 0);
            this.buttonLoadBin.Name = "buttonLoadBin";
            this.buttonLoadBin.Size = new System.Drawing.Size(67, 23);
            this.buttonLoadBin.TabIndex = 16;
            this.buttonLoadBin.Text = "Load .bin";
            this.buttonLoadBin.UseVisualStyleBackColor = true;
            this.buttonLoadBin.Click += new System.EventHandler(this.buttonLoadBin_Click);
            // 
            // lstGroups
            // 
            this.lstGroups.FormattingEnabled = true;
            this.lstGroups.Location = new System.Drawing.Point(149, 120);
            this.lstGroups.Name = "lstGroups";
            this.lstGroups.Size = new System.Drawing.Size(157, 160);
            this.lstGroups.TabIndex = 17;
            this.lstGroups.SelectedIndexChanged += new System.EventHandler(this.lstGroups_SelectedIndexChanged);
            // 
            // txtGroupName
            // 
            this.txtGroupName.Location = new System.Drawing.Point(149, 65);
            this.txtGroupName.Name = "txtGroupName";
            this.txtGroupName.Size = new System.Drawing.Size(157, 20);
            this.txtGroupName.TabIndex = 18;
            this.txtGroupName.Text = "NewGroup";
            // 
            // btnAddGroup
            // 
            this.btnAddGroup.Location = new System.Drawing.Point(149, 91);
            this.btnAddGroup.Name = "btnAddGroup";
            this.btnAddGroup.Size = new System.Drawing.Size(76, 23);
            this.btnAddGroup.TabIndex = 19;
            this.btnAddGroup.Text = "Add";
            this.btnAddGroup.UseVisualStyleBackColor = true;
            this.btnAddGroup.Click += new System.EventHandler(this.btnAddGroup_Click);
            // 
            // btnRemoveGroup
            // 
            this.btnRemoveGroup.Location = new System.Drawing.Point(231, 91);
            this.btnRemoveGroup.Name = "btnRemoveGroup";
            this.btnRemoveGroup.Size = new System.Drawing.Size(75, 23);
            this.btnRemoveGroup.TabIndex = 20;
            this.btnRemoveGroup.Text = "Remove";
            this.btnRemoveGroup.UseVisualStyleBackColor = true;
            this.btnRemoveGroup.Click += new System.EventHandler(this.btnRemoveGroup_Click);
            // 
            // btnAddToGroup
            // 
            this.btnAddToGroup.Location = new System.Drawing.Point(149, 286);
            this.btnAddToGroup.Name = "btnAddToGroup";
            this.btnAddToGroup.Size = new System.Drawing.Size(157, 23);
            this.btnAddToGroup.TabIndex = 21;
            this.btnAddToGroup.Text = "Add Selected to Group";
            this.btnAddToGroup.UseVisualStyleBackColor = true;
            this.btnAddToGroup.Click += new System.EventHandler(this.btnAddToGroup_Click);
            // 
            // groupBoxAtlasType
            // 
            this.groupBoxAtlasType.Controls.Add(this.labelSpritesPerLevel);
            this.groupBoxAtlasType.Controls.Add(this.numericUpDownSpritesPerLevel);
            this.groupBoxAtlasType.Location = new System.Drawing.Point(540, 35);
            this.groupBoxAtlasType.Name = "groupBoxAtlasType";
            this.groupBoxAtlasType.Size = new System.Drawing.Size(280, 60);
            this.groupBoxAtlasType.TabIndex = 22;
            this.groupBoxAtlasType.TabStop = false;
            this.groupBoxAtlasType.Text = "Atlas Type";
            // 
            // labelSpritesPerLevel
            // 
            this.labelSpritesPerLevel.AutoSize = true;
            this.labelSpritesPerLevel.Location = new System.Drawing.Point(140, 23);
            this.labelSpritesPerLevel.Name = "labelSpritesPerLevel";
            this.labelSpritesPerLevel.Size = new System.Drawing.Size(73, 13);
            this.labelSpritesPerLevel.TabIndex = 24;
            this.labelSpritesPerLevel.Text = "Sprites/Level:";
            // 
            // numericUpDownSpritesPerLevel
            // 
            this.numericUpDownSpritesPerLevel.Location = new System.Drawing.Point(210, 20);
            this.numericUpDownSpritesPerLevel.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.numericUpDownSpritesPerLevel.Name = "numericUpDownSpritesPerLevel";
            this.numericUpDownSpritesPerLevel.Size = new System.Drawing.Size(50, 20);
            this.numericUpDownSpritesPerLevel.TabIndex = 25;
            this.numericUpDownSpritesPerLevel.Value = new decimal(new int[] {
            2,
            0,
            0,
            0});
            this.numericUpDownSpritesPerLevel.ValueChanged += new System.EventHandler(this.numericUpDownSpritesPerLevel_ValueChanged);
            // 
            // comboBoxAtlasType
            // 
            this.comboBoxAtlasType.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.comboBoxAtlasType.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxAtlasType.FormattingEnabled = true;
            this.comboBoxAtlasType.Items.AddRange(new object[] {
            "Single Sprite",
            "Multi-Level",
            "Animation"});
            this.comboBoxAtlasType.Location = new System.Drawing.Point(759, 1);
            this.comboBoxAtlasType.Name = "comboBoxAtlasType";
            this.comboBoxAtlasType.Size = new System.Drawing.Size(106, 21);
            this.comboBoxAtlasType.TabIndex = 22;
            this.comboBoxAtlasType.SelectedIndexChanged += new System.EventHandler(this.comboBoxAtlasType_SelectedIndexChanged);
            // 
            // btnPlayAnimation
            // 
            this.btnPlayAnimation.Location = new System.Drawing.Point(3, 304);
            this.btnPlayAnimation.Name = "btnPlayAnimation";
            this.btnPlayAnimation.Size = new System.Drawing.Size(60, 23);
            this.btnPlayAnimation.TabIndex = 26;
            this.btnPlayAnimation.Text = "▶ Play";
            this.btnPlayAnimation.UseVisualStyleBackColor = true;
            this.btnPlayAnimation.Click += new System.EventHandler(this.btnPlayAnimation_Click);
            // 
            // btnStopAnimation
            // 
            this.btnStopAnimation.Location = new System.Drawing.Point(69, 304);
            this.btnStopAnimation.Name = "btnStopAnimation";
            this.btnStopAnimation.Size = new System.Drawing.Size(60, 23);
            this.btnStopAnimation.TabIndex = 27;
            this.btnStopAnimation.Text = "⏹ Stop";
            this.btnStopAnimation.UseVisualStyleBackColor = true;
            this.btnStopAnimation.Click += new System.EventHandler(this.btnStopAnimation_Click);
            // 
            // btnAutoPack
            // 
            this.btnAutoPack.Location = new System.Drawing.Point(3, 186);
            this.btnAutoPack.Name = "btnAutoPack";
            this.btnAutoPack.Size = new System.Drawing.Size(105, 23);
            this.btnAutoPack.TabIndex = 0;
            this.btnAutoPack.Text = "Auto-Pack 256x256";
            this.btnAutoPack.UseVisualStyleBackColor = true;
            this.btnAutoPack.Visible = false;
            this.btnAutoPack.Click += new System.EventHandler(this.btnAutoPack_Click);
            // 
            // animationTimer
            // 
            this.animationTimer.Tick += new System.EventHandler(this.animationTimer_Tick);
            // 
            // btnCenterPivotAnimation
            // 
            this.btnCenterPivotAnimation.Enabled = false;
            this.btnCenterPivotAnimation.Location = new System.Drawing.Point(3, 215);
            this.btnCenterPivotAnimation.Name = "btnCenterPivotAnimation";
            this.btnCenterPivotAnimation.Size = new System.Drawing.Size(105, 23);
            this.btnCenterPivotAnimation.TabIndex = 28;
            this.btnCenterPivotAnimation.Text = "Center Animation";
            this.btnCenterPivotAnimation.UseVisualStyleBackColor = true;
            this.btnCenterPivotAnimation.Visible = false;
            this.btnCenterPivotAnimation.Click += new System.EventHandler(this.btnCenterPivotAnimation_Click);
            // 
            // btnCenterSprite
            // 
            this.btnCenterSprite.Enabled = false;
            this.btnCenterSprite.Location = new System.Drawing.Point(3, 244);
            this.btnCenterSprite.Name = "btnCenterSprite";
            this.btnCenterSprite.Size = new System.Drawing.Size(105, 23);
            this.btnCenterSprite.TabIndex = 0;
            this.btnCenterSprite.Text = "Center Sprite";
            this.btnCenterSprite.UseVisualStyleBackColor = true;
            this.btnCenterSprite.Visible = false;
            this.btnCenterSprite.Click += new System.EventHandler(this.btnCenterSprite_Click);
            // 
            // chkShowGuides
            // 
            this.chkShowGuides.Checked = true;
            this.chkShowGuides.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkShowGuides.Location = new System.Drawing.Point(496, 751);
            this.chkShowGuides.Name = "chkShowGuides";
            this.chkShowGuides.Size = new System.Drawing.Size(101, 23);
            this.chkShowGuides.TabIndex = 0;
            this.chkShowGuides.Text = "Show Guides";
            this.chkShowGuides.Visible = false;
            this.chkShowGuides.CheckedChanged += new System.EventHandler(this.chkShowGuides_CheckedChanged);
            // 
            // btnMoveUp
            // 
            this.btnMoveUp.Location = new System.Drawing.Point(567, 32);
            this.btnMoveUp.Name = "btnMoveUp";
            this.btnMoveUp.Size = new System.Drawing.Size(30, 23);
            this.btnMoveUp.TabIndex = 0;
            this.btnMoveUp.Text = "↑";
            this.btnMoveUp.UseVisualStyleBackColor = true;
            this.btnMoveUp.Click += new System.EventHandler(this.btnMoveUp_Click);
            // 
            // btnMoveDown
            // 
            this.btnMoveDown.Location = new System.Drawing.Point(567, 286);
            this.btnMoveDown.Name = "btnMoveDown";
            this.btnMoveDown.Size = new System.Drawing.Size(30, 23);
            this.btnMoveDown.TabIndex = 1;
            this.btnMoveDown.Text = "↓";
            this.btnMoveDown.UseVisualStyleBackColor = true;
            this.btnMoveDown.Click += new System.EventHandler(this.btnMoveDown_Click);
            // 
            // btnExportUV
            // 
            this.btnExportUV.Location = new System.Drawing.Point(3, 3);
            this.btnExportUV.Name = "btnExportUV";
            this.btnExportUV.Size = new System.Drawing.Size(84, 23);
            this.btnExportUV.TabIndex = 1;
            this.btnExportUV.Text = "Export UV.txt";
            this.btnExportUV.UseVisualStyleBackColor = true;
            this.btnExportUV.Visible = false;
            this.btnExportUV.Click += new System.EventHandler(this.btnExportUV_Click);
            // 
            // btnDeleteSelected
            // 
            this.btnDeleteSelected.Location = new System.Drawing.Point(361, 317);
            this.btnDeleteSelected.Name = "btnDeleteSelected";
            this.btnDeleteSelected.Size = new System.Drawing.Size(158, 23);
            this.btnDeleteSelected.TabIndex = 0;
            this.btnDeleteSelected.Text = "Delete Selected Rect>";
            this.btnDeleteSelected.UseVisualStyleBackColor = true;
            this.btnDeleteSelected.Click += new System.EventHandler(this.btnDeleteSelected_Click);
            // 
            // chkFillUVArea
            // 
            this.chkFillUVArea.Checked = true;
            this.chkFillUVArea.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkFillUVArea.Location = new System.Drawing.Point(267, 750);
            this.chkFillUVArea.Name = "chkFillUVArea";
            this.chkFillUVArea.Size = new System.Drawing.Size(97, 23);
            this.chkFillUVArea.TabIndex = 1;
            this.chkFillUVArea.Text = "Fill UV Area";
            this.chkFillUVArea.Visible = false;
            this.chkFillUVArea.CheckedChanged += new System.EventHandler(this.chkFillUVArea_CheckedChanged);
            // 
            // menuStrip1
            // 
            this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.viewToolStripMenuItem,
            this.uVToolStripMenuItem,
            this.editNameToolStripMenuItem,
            this.mirrorToolStripMenuItem});
            this.menuStrip1.Location = new System.Drawing.Point(0, 0);
            this.menuStrip1.Name = "menuStrip1";
            this.menuStrip1.Size = new System.Drawing.Size(1470, 24);
            this.menuStrip1.TabIndex = 0;
            this.menuStrip1.Text = "menuStrip1";
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.openTextureToolStripMenuItem,
            this.openAtlasToolStripMenuItem,
            this.loadDefaultToolStripMenuItem,
            this.saveToolStripMenuItem,
            this.saveDefaultToolStripMenuItem,
            this.toolStripSeparator1,
            this.fileCloseToolStripMenuItem,
            this.exitToolStripMenuItem});
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(37, 20);
            this.fileToolStripMenuItem.Text = "File";
            // 
            // openTextureToolStripMenuItem
            // 
            this.openTextureToolStripMenuItem.Name = "openTextureToolStripMenuItem";
            this.openTextureToolStripMenuItem.Size = new System.Drawing.Size(144, 22);
            this.openTextureToolStripMenuItem.Text = "Open Texture";
            this.openTextureToolStripMenuItem.Click += new System.EventHandler(this.buttonLoad_Click);
            // 
            // openAtlasToolStripMenuItem
            // 
            this.openAtlasToolStripMenuItem.Name = "openAtlasToolStripMenuItem";
            this.openAtlasToolStripMenuItem.Size = new System.Drawing.Size(144, 22);
            this.openAtlasToolStripMenuItem.Text = "Open Atlas";
            this.openAtlasToolStripMenuItem.Click += new System.EventHandler(this.buttonLoadBin_Click);
            // 
            // saveToolStripMenuItem
            // 
            this.saveToolStripMenuItem.Name = "saveToolStripMenuItem";
            this.saveToolStripMenuItem.Size = new System.Drawing.Size(144, 22);
            this.saveToolStripMenuItem.Text = "Save";
            this.saveToolStripMenuItem.Click += new System.EventHandler(this.buttonSave_Click);
            // 
            // saveDefaultToolStripMenuItem
            // 
            this.saveDefaultToolStripMenuItem.Name = "saveDefaultToolStripMenuItem";
            this.saveDefaultToolStripMenuItem.Size = new System.Drawing.Size(144, 22);
            this.saveDefaultToolStripMenuItem.Text = "Save Default";
            this.saveDefaultToolStripMenuItem.Click += new System.EventHandler(this.btnSaveDefaultBin_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(141, 6);
            // 
            // fileCloseToolStripMenuItem
            // 
            this.fileCloseToolStripMenuItem.Name = "fileCloseToolStripMenuItem";
            this.fileCloseToolStripMenuItem.Size = new System.Drawing.Size(144, 22);
            this.fileCloseToolStripMenuItem.Text = "File Close";
            this.fileCloseToolStripMenuItem.Click += new System.EventHandler(this.fileCloseToolStripMenuItem_Click);
            // 
            // exitToolStripMenuItem
            // 
            this.exitToolStripMenuItem.Name = "exitToolStripMenuItem";
            this.exitToolStripMenuItem.Size = new System.Drawing.Size(144, 22);
            this.exitToolStripMenuItem.Text = "Exit";
            this.exitToolStripMenuItem.Click += new System.EventHandler(this.exitToolStripMenuItem_Click);
            // 
            // viewToolStripMenuItem
            // 
            this.viewToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.showGuidesToolStripMenuItem,
            this.fillUVAreaToolStripMenuItem,
            this.showPivotGuidesToolStripMenuItem});
            this.viewToolStripMenuItem.Name = "viewToolStripMenuItem";
            this.viewToolStripMenuItem.Size = new System.Drawing.Size(44, 20);
            this.viewToolStripMenuItem.Text = "View";
            // 
            // showGuidesToolStripMenuItem
            // 
            this.showGuidesToolStripMenuItem.Name = "showGuidesToolStripMenuItem";
            this.showGuidesToolStripMenuItem.Size = new System.Drawing.Size(172, 22);
            this.showGuidesToolStripMenuItem.Text = "Show Guides";
            this.showGuidesToolStripMenuItem.Click += new System.EventHandler(this.showGuidesToolStripMenuItem_Click);
            // 
            // fillUVAreaToolStripMenuItem
            // 
            this.fillUVAreaToolStripMenuItem.Name = "fillUVAreaToolStripMenuItem";
            this.fillUVAreaToolStripMenuItem.Size = new System.Drawing.Size(172, 22);
            this.fillUVAreaToolStripMenuItem.Text = "Fill UV Area";
            this.fillUVAreaToolStripMenuItem.Click += new System.EventHandler(this.fillUVAreaToolStripMenuItem_Click);
            // 
            // showPivotGuidesToolStripMenuItem
            // 
            this.showPivotGuidesToolStripMenuItem.Name = "showPivotGuidesToolStripMenuItem";
            this.showPivotGuidesToolStripMenuItem.Size = new System.Drawing.Size(172, 22);
            this.showPivotGuidesToolStripMenuItem.Text = "Show Pivot Guides";
            this.showPivotGuidesToolStripMenuItem.Click += new System.EventHandler(this.showPivotGuidesToolStripMenuItem_Click);
            // 
            // uVToolStripMenuItem
            // 
            this.uVToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.autoPackToolStripMenuItem,
            this.exportUVToolStripMenuItem1});
            this.uVToolStripMenuItem.Name = "uVToolStripMenuItem";
            this.uVToolStripMenuItem.Size = new System.Drawing.Size(34, 20);
            this.uVToolStripMenuItem.Text = "UV";
            // 
            // autoPackToolStripMenuItem
            // 
            this.autoPackToolStripMenuItem.Name = "autoPackToolStripMenuItem";
            this.autoPackToolStripMenuItem.Size = new System.Drawing.Size(130, 22);
            this.autoPackToolStripMenuItem.Text = "Auto-Pack";
            this.autoPackToolStripMenuItem.Click += new System.EventHandler(this.btnAutoPack_Click);
            // 
            // exportUVToolStripMenuItem1
            // 
            this.exportUVToolStripMenuItem1.Name = "exportUVToolStripMenuItem1";
            this.exportUVToolStripMenuItem1.Size = new System.Drawing.Size(130, 22);
            this.exportUVToolStripMenuItem1.Text = "Export UV";
            this.exportUVToolStripMenuItem1.Click += new System.EventHandler(this.exportUVToolStripMenuItem_Click);
            // 
            // btnPreviewZoomIn
            // 
            this.btnPreviewZoomIn.Location = new System.Drawing.Point(11, 750);
            this.btnPreviewZoomIn.Name = "btnPreviewZoomIn";
            this.btnPreviewZoomIn.Size = new System.Drawing.Size(30, 23);
            this.btnPreviewZoomIn.TabIndex = 0;
            this.btnPreviewZoomIn.Text = "+";
            this.btnPreviewZoomIn.UseVisualStyleBackColor = true;
            this.btnPreviewZoomIn.Visible = false;
            this.btnPreviewZoomIn.Click += new System.EventHandler(this.btnPreviewZoomIn_Click);
            // 
            // btnPreviewZoomOut
            // 
            this.btnPreviewZoomOut.Location = new System.Drawing.Point(47, 750);
            this.btnPreviewZoomOut.Name = "btnPreviewZoomOut";
            this.btnPreviewZoomOut.Size = new System.Drawing.Size(30, 23);
            this.btnPreviewZoomOut.TabIndex = 1;
            this.btnPreviewZoomOut.Text = "-";
            this.btnPreviewZoomOut.UseVisualStyleBackColor = true;
            this.btnPreviewZoomOut.Visible = false;
            this.btnPreviewZoomOut.Click += new System.EventHandler(this.btnPreviewZoomOut_Click);
            // 
            // btnPreviewZoomReset
            // 
            this.btnPreviewZoomReset.Location = new System.Drawing.Point(83, 751);
            this.btnPreviewZoomReset.Name = "btnPreviewZoomReset";
            this.btnPreviewZoomReset.Size = new System.Drawing.Size(52, 23);
            this.btnPreviewZoomReset.TabIndex = 2;
            this.btnPreviewZoomReset.Text = "Reset";
            this.btnPreviewZoomReset.UseVisualStyleBackColor = true;
            this.btnPreviewZoomReset.Visible = false;
            this.btnPreviewZoomReset.Click += new System.EventHandler(this.btnPreviewZoomReset_Click);
            // 
            // chkShowPivotGuides
            // 
            this.chkShowPivotGuides.Checked = true;
            this.chkShowPivotGuides.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkShowPivotGuides.Location = new System.Drawing.Point(370, 750);
            this.chkShowPivotGuides.Name = "chkShowPivotGuides";
            this.chkShowPivotGuides.Size = new System.Drawing.Size(120, 23);
            this.chkShowPivotGuides.TabIndex = 0;
            this.chkShowPivotGuides.Text = "Show Pivot Guides";
            this.chkShowPivotGuides.Visible = false;
            this.chkShowPivotGuides.CheckedChanged += new System.EventHandler(this.chkShowPivotGuides_CheckedChanged);
            // 
            // btnPivotTopLeft
            // 
            this.btnPivotTopLeft.Location = new System.Drawing.Point(3, 41);
            this.btnPivotTopLeft.Name = "btnPivotTopLeft";
            this.btnPivotTopLeft.Size = new System.Drawing.Size(105, 23);
            this.btnPivotTopLeft.TabIndex = 29;
            this.btnPivotTopLeft.Text = "PivotTopLeft";
            this.btnPivotTopLeft.UseVisualStyleBackColor = true;
            this.btnPivotTopLeft.Click += new System.EventHandler(this.btnPivotTopLeft_Click);
            // 
            // btnEditCollision
            // 
            this.btnEditCollision.Location = new System.Drawing.Point(3, 273);
            this.btnEditCollision.Name = "btnEditCollision";
            this.btnEditCollision.Size = new System.Drawing.Size(105, 23);
            this.btnEditCollision.TabIndex = 30;
            this.btnEditCollision.Text = "EditCollision";
            this.btnEditCollision.UseVisualStyleBackColor = true;
            this.btnEditCollision.Click += new System.EventHandler(this.btnEditCollision_Click);
            // 
            // btnEditNodeWeight
            // 
            this.btnEditNodeWeight.Location = new System.Drawing.Point(3, 302);
            this.btnEditNodeWeight.Name = "btnEditNodeWeight";
            this.btnEditNodeWeight.Size = new System.Drawing.Size(105, 23);
            this.btnEditNodeWeight.TabIndex = 33;
            this.btnEditNodeWeight.Text = "NodeWeight";
            this.btnEditNodeWeight.UseVisualStyleBackColor = true;
            this.btnEditNodeWeight.Click += new System.EventHandler(this.btnEditNodeWeight_Click);
            // 
            // btnCreateMirror
            // 
            this.btnCreateMirror.Location = new System.Drawing.Point(231, 32);
            this.btnCreateMirror.Name = "btnCreateMirror";
            this.btnCreateMirror.Size = new System.Drawing.Size(75, 23);
            this.btnCreateMirror.TabIndex = 31;
            this.btnCreateMirror.Text = "CreateMirror";
            this.btnCreateMirror.UseVisualStyleBackColor = true;
            this.btnCreateMirror.Click += new System.EventHandler(this.btnCreateMirror_Click);
            // 
            // btnAutoNameSprites
            // 
            this.btnAutoNameSprites.Location = new System.Drawing.Point(216, 32);
            this.btnAutoNameSprites.Name = "btnAutoNameSprites";
            this.btnAutoNameSprites.Size = new System.Drawing.Size(75, 23);
            this.btnAutoNameSprites.TabIndex = 32;
            this.btnAutoNameSprites.Text = "NameSprite";
            this.btnAutoNameSprites.UseVisualStyleBackColor = true;
            this.btnAutoNameSprites.Visible = false;
            this.btnAutoNameSprites.Click += new System.EventHandler(this.btnAutoNameSprites_Click);
            // 
            // editNameToolStripMenuItem
            // 
            this.editNameToolStripMenuItem.Name = "editNameToolStripMenuItem";
            this.editNameToolStripMenuItem.Size = new System.Drawing.Size(74, 20);
            this.editNameToolStripMenuItem.Text = "Edit Name";
            this.editNameToolStripMenuItem.Click += new System.EventHandler(this.editNameToolStripMenuItem_Click);
            // 
            // mirrorToolStripMenuItem
            // 
            this.mirrorToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.xToolStripMenuItem,
            this.yToolStripMenuItem});
            this.mirrorToolStripMenuItem.Name = "mirrorToolStripMenuItem";
            this.mirrorToolStripMenuItem.Size = new System.Drawing.Size(52, 20);
            this.mirrorToolStripMenuItem.Text = "Mirror";
            // 
            // xToolStripMenuItem
            // 
            this.xToolStripMenuItem.Name = "xToolStripMenuItem";
            this.xToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
            this.xToolStripMenuItem.Text = "X";
            this.xToolStripMenuItem.Click += new System.EventHandler(this.btnCreateMirrorX_Click);
            // 
            // yToolStripMenuItem
            // 
            this.yToolStripMenuItem.Name = "yToolStripMenuItem";
            this.yToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
            this.yToolStripMenuItem.Text = "Y";
            this.yToolStripMenuItem.Click += new System.EventHandler(this.btnCreateMirrorY_Click);
            // 
            // loadDefaultToolStripMenuItem
            // 
            this.loadDefaultToolStripMenuItem.Name = "loadDefaultToolStripMenuItem";
            this.loadDefaultToolStripMenuItem.Size = new System.Drawing.Size(170, 22);
            this.loadDefaultToolStripMenuItem.Text = "Load Default Atlas";
            this.loadDefaultToolStripMenuItem.Click += new System.EventHandler(this.loadDefaultToolStripMenuItem_Click);
            // 
            // panel1
            // 
            this.panel1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.panel1.Controls.Add(this.btnExportUV);
            this.panel1.Controls.Add(this.chkShowPivotGuides);
            this.panel1.Controls.Add(this.btnCreateMirror);
            this.panel1.Controls.Add(this.btnPreviewZoomIn);
            this.panel1.Controls.Add(this.btnPreviewZoomOut);
            this.panel1.Controls.Add(this.btnAutoNameSprites);
            this.panel1.Controls.Add(this.btnPreviewZoomReset);
            this.panel1.Controls.Add(this.buttonSave);
            this.panel1.Controls.Add(this.chkFillUVArea);
            this.panel1.Controls.Add(this.buttonLoadBin);
            this.panel1.Controls.Add(this.chkShowGuides);
            this.panel1.Controls.Add(this.btnDeleteSelected);
            this.panel1.Controls.Add(this.btnEditNodeWeight);
            this.panel1.Controls.Add(this.btnEditCollision);
            this.panel1.Controls.Add(this.btnSaveDefaultBin);
            this.panel1.Controls.Add(this.btnDeleteRect);
            this.panel1.Controls.Add(this.btnPivotTopLeft);
            this.panel1.Controls.Add(this.buttonLoad);
            this.panel1.Controls.Add(this.btnMoveDown);
            this.panel1.Controls.Add(this.btnCenterPivot);
            this.panel1.Controls.Add(this.btnEditRect);
            this.panel1.Controls.Add(this.btnMoveUp);
            this.panel1.Controls.Add(this.btnFillRect);
            this.panel1.Controls.Add(this.btnAddRect);
            this.panel1.Controls.Add(this.btnAutoPack);
            this.panel1.Controls.Add(this.btnCenterPivotAnimation);
            this.panel1.Controls.Add(this.previewPanel);
            this.panel1.Controls.Add(this.lstSprites);
            this.panel1.Controls.Add(this.lstGroups);
            this.panel1.Controls.Add(this.btnCenterSprite);
            this.panel1.Controls.Add(this.btnAddToGroup);
            this.panel1.Controls.Add(this.btnRemoveGroup);
            this.panel1.Controls.Add(this.btnAddGroup);
            this.panel1.Controls.Add(this.txtGroupName);
            this.panel1.Controls.Add(this.lblInfo);
            this.panel1.Controls.Add(this.btnPlayAnimation);
            this.panel1.Controls.Add(this.btnStopAnimation);
            this.panel1.Location = new System.Drawing.Point(870, 1);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(600, 776);
            this.panel1.TabIndex = 33;
            // 
            // panel2
            // 
            this.panel2.Anchor = System.Windows.Forms.AnchorStyles.Bottom;
            this.panel2.Controls.Add(this.btnZoomReset);
            this.panel2.Controls.Add(this.btnZoomOut);
            this.panel2.Controls.Add(this.btnZoomIn);
            this.panel2.Location = new System.Drawing.Point(12, 753);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(852, 23);
            this.panel2.TabIndex = 34;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.ClientSize = new System.Drawing.Size(1470, 780);
            this.Controls.Add(this.panel2);
            this.Controls.Add(this.panel1);
            this.Controls.Add(this.comboBoxAtlasType);
            this.Controls.Add(this.panelAtlas);
            this.Controls.Add(this.lblFileName);
            this.Controls.Add(this.menuStrip1);
            this.Name = "Form1";
            this.Text = "Sprite Atlas Tool";
            this.previewPanel.ResumeLayout(false);
            this.previewPanel.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.previewBox)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.panelAtlas.ResumeLayout(false);
            this.panelAtlas.PerformLayout();
            this.groupBoxAtlasType.ResumeLayout(false);
            this.groupBoxAtlasType.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numericUpDownSpritesPerLevel)).EndInit();
            this.menuStrip1.ResumeLayout(false);
            this.menuStrip1.PerformLayout();
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.panel2.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }
        private System.Windows.Forms.Button buttonLoadBin;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.Button buttonLoad;
        private System.Windows.Forms.Button buttonSave;
        private System.Windows.Forms.ListBox lstSprites;
        private System.Windows.Forms.Button btnAddRect;
        private System.Windows.Forms.Button btnDeleteRect;
        private System.Windows.Forms.Label lblInfo;
        private System.Windows.Forms.Panel previewPanel;
        private System.Windows.Forms.PictureBox previewBox;
        private System.Windows.Forms.Panel panelAtlas;
        private System.Windows.Forms.Button btnCenterPivot;
        private System.Windows.Forms.Button btnZoomIn;
        private System.Windows.Forms.Button btnZoomOut;
        private System.Windows.Forms.Button btnZoomReset;
        private System.Windows.Forms.Button btnEditRect;
        private System.Windows.Forms.Button btnFillRect;
        private System.Windows.Forms.Label lblFileName;
        private System.Windows.Forms.Button btnSaveDefaultBin;

        private System.Windows.Forms.GroupBox groupBoxAtlasType;
        private System.Windows.Forms.Label labelSpritesPerLevel;
        private System.Windows.Forms.NumericUpDown numericUpDownSpritesPerLevel;

        private ListBox lstGroups;
        private Button btnAddGroup;
        private Button btnRemoveGroup;
        private Button btnAddToGroup;
        private TextBox txtGroupName;
        private ComboBox comboBoxAtlasType;

        private System.Windows.Forms.Button btnPlayAnimation;
        private System.Windows.Forms.Button btnStopAnimation;
        private System.Windows.Forms.Timer animationTimer;
        private System.Windows.Forms.Button btnCenterPivotAnimation;
        private System.Windows.Forms.Button btnCenterSprite;
        private System.Windows.Forms.CheckBox chkShowGuides;
        private System.Windows.Forms.Button btnMoveUp;
        private System.Windows.Forms.Button btnMoveDown;
        private System.Windows.Forms.Button btnAutoPack;
        private System.Windows.Forms.Button btnExportUV;
        private System.Windows.Forms.Button btnDeleteSelected;
        private System.Windows.Forms.CheckBox chkFillUVArea;

        private System.Windows.Forms.MenuStrip menuStrip1;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem openTextureToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem openAtlasToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem saveToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem saveDefaultToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem exitToolStripMenuItem;

        private System.Windows.Forms.ToolStripMenuItem viewToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem showGuidesToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem fillUVAreaToolStripMenuItem;

        private System.Windows.Forms.Button btnPreviewZoomIn;
        private System.Windows.Forms.Button btnPreviewZoomOut;
        private System.Windows.Forms.Button btnPreviewZoomReset;
        private System.Windows.Forms.CheckBox chkShowPivotGuides;
        private ToolStripMenuItem showPivotGuidesToolStripMenuItem;
        private ToolStripMenuItem uVToolStripMenuItem;
        private ToolStripMenuItem autoPackToolStripMenuItem;
        private ToolStripMenuItem exportUVToolStripMenuItem1;
        private Button btnPivotTopLeft;
        private Button btnEditCollision;
        private Button btnEditNodeWeight;
        private ToolStripMenuItem fileCloseToolStripMenuItem;
        private Button btnCreateMirror;
        private Button btnAutoNameSprites;
        private ToolStripMenuItem editNameToolStripMenuItem;
        private ToolStripMenuItem mirrorToolStripMenuItem;
        private ToolStripMenuItem xToolStripMenuItem;
        private ToolStripMenuItem yToolStripMenuItem;
        private ToolStripMenuItem loadDefaultToolStripMenuItem;
        private Panel panel1;
        private Panel panel2;
    }
}
