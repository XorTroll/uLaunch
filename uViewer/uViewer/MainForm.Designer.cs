namespace uViewer
{
    partial class MainForm
    {
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
            this.EntryManagerButton = new System.Windows.Forms.Button();
            this.ViewerButton = new System.Windows.Forms.Button();
            this.FoundDriveText = new System.Windows.Forms.Label();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.PluginUnloadButton = new System.Windows.Forms.Button();
            this.PluginButton = new System.Windows.Forms.Button();
            this.PluginLabel = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.AboutButton = new System.Windows.Forms.Button();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // EntryManagerButton
            // 
            this.EntryManagerButton.Enabled = false;
            this.EntryManagerButton.Location = new System.Drawing.Point(360, 42);
            this.EntryManagerButton.Name = "EntryManagerButton";
            this.EntryManagerButton.Size = new System.Drawing.Size(345, 27);
            this.EntryManagerButton.TabIndex = 0;
            this.EntryManagerButton.Text = "Load entry creator";
            this.EntryManagerButton.TextImageRelation = System.Windows.Forms.TextImageRelation.TextAboveImage;
            this.EntryManagerButton.UseCompatibleTextRendering = true;
            this.EntryManagerButton.UseVisualStyleBackColor = true;
            this.EntryManagerButton.Click += new System.EventHandler(this.EntryManagerButton_Click);
            // 
            // ViewerButton
            // 
            this.ViewerButton.Enabled = false;
            this.ViewerButton.Location = new System.Drawing.Point(12, 143);
            this.ViewerButton.Name = "ViewerButton";
            this.ViewerButton.Size = new System.Drawing.Size(711, 68);
            this.ViewerButton.TabIndex = 1;
            this.ViewerButton.Text = "Screen viewer (needs USB, Windows-only)";
            this.ViewerButton.UseVisualStyleBackColor = true;
            this.ViewerButton.Click += new System.EventHandler(this.ViewerButton_Click);
            // 
            // FoundDriveText
            // 
            this.FoundDriveText.AutoSize = true;
            this.FoundDriveText.Dock = System.Windows.Forms.DockStyle.Right;
            this.FoundDriveText.Location = new System.Drawing.Point(446, 16);
            this.FoundDriveText.Name = "FoundDriveText";
            this.FoundDriveText.Size = new System.Drawing.Size(262, 13);
            this.FoundDriveText.TabIndex = 2;
            this.FoundDriveText.Text = "No SD card drive (containing uLaunch) was detected.";
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.PluginUnloadButton);
            this.groupBox1.Controls.Add(this.PluginButton);
            this.groupBox1.Controls.Add(this.PluginLabel);
            this.groupBox1.Controls.Add(this.EntryManagerButton);
            this.groupBox1.Controls.Add(this.FoundDriveText);
            this.groupBox1.Location = new System.Drawing.Point(12, 47);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(711, 81);
            this.groupBox1.TabIndex = 5;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Main menu entry creator";
            // 
            // PluginUnloadButton
            // 
            this.PluginUnloadButton.Enabled = false;
            this.PluginUnloadButton.Location = new System.Drawing.Point(138, 46);
            this.PluginUnloadButton.Name = "PluginUnloadButton";
            this.PluginUnloadButton.Size = new System.Drawing.Size(126, 23);
            this.PluginUnloadButton.TabIndex = 7;
            this.PluginUnloadButton.Text = "Unload plugin";
            this.PluginUnloadButton.UseVisualStyleBackColor = true;
            this.PluginUnloadButton.Click += new System.EventHandler(this.PluginUnloadButton_Click);
            // 
            // PluginButton
            // 
            this.PluginButton.Enabled = false;
            this.PluginButton.Location = new System.Drawing.Point(10, 46);
            this.PluginButton.Name = "PluginButton";
            this.PluginButton.Size = new System.Drawing.Size(122, 23);
            this.PluginButton.TabIndex = 6;
            this.PluginButton.Text = "Load plugin";
            this.PluginButton.UseVisualStyleBackColor = true;
            this.PluginButton.Click += new System.EventHandler(this.PluginButton_Click);
            // 
            // PluginLabel
            // 
            this.PluginLabel.AutoSize = true;
            this.PluginLabel.Location = new System.Drawing.Point(7, 30);
            this.PluginLabel.Name = "PluginLabel";
            this.PluginLabel.Size = new System.Drawing.Size(199, 13);
            this.PluginLabel.TabIndex = 5;
            this.PluginLabel.Text = "There is no entry creator plugin selected.";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(8, 18);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(411, 17);
            this.label1.TabIndex = 6;
            this.label1.Text = "Welcome to uViewer! Are you having a good time with uLaunch?";
            // 
            // AboutButton
            // 
            this.AboutButton.Location = new System.Drawing.Point(12, 217);
            this.AboutButton.Name = "AboutButton";
            this.AboutButton.Size = new System.Drawing.Size(711, 37);
            this.AboutButton.TabIndex = 7;
            this.AboutButton.Text = "About uLaunch project";
            this.AboutButton.UseVisualStyleBackColor = true;
            this.AboutButton.Click += new System.EventHandler(this.AboutButton_Click);
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(737, 270);
            this.Controls.Add(this.AboutButton);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.ViewerButton);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "MainForm";
            this.Text = "uViewer - uLaunch\'s PC toolbox";
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button EntryManagerButton;
        private System.Windows.Forms.Button ViewerButton;
        private System.Windows.Forms.Label FoundDriveText;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Button PluginButton;
        private System.Windows.Forms.Label PluginLabel;
        private System.Windows.Forms.Button PluginUnloadButton;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button AboutButton;
    }
}