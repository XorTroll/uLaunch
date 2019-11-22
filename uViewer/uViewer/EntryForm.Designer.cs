namespace uViewer
{
    partial class EntryForm
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(EntryForm));
            this.InformationLabel = new System.Windows.Forms.Label();
            this.MainGroup = new System.Windows.Forms.GroupBox();
            this.ArgvText = new System.Windows.Forms.TextBox();
            this.ArgvLabel = new System.Windows.Forms.Label();
            this.NroText = new System.Windows.Forms.TextBox();
            this.NroLabel = new System.Windows.Forms.Label();
            this.VersionText = new System.Windows.Forms.TextBox();
            this.VersionLabel = new System.Windows.Forms.Label();
            this.AuthorLabel = new System.Windows.Forms.Label();
            this.AuthorText = new System.Windows.Forms.TextBox();
            this.NameText = new System.Windows.Forms.TextBox();
            this.NameLabel = new System.Windows.Forms.Label();
            this.IconGroup = new System.Windows.Forms.GroupBox();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.MakeButton = new System.Windows.Forms.Button();
            this.MainGroup.SuspendLayout();
            this.IconGroup.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // InformationLabel
            // 
            this.InformationLabel.AutoSize = true;
            this.InformationLabel.Location = new System.Drawing.Point(12, 26);
            this.InformationLabel.Name = "InformationLabel";
            this.InformationLabel.Size = new System.Drawing.Size(373, 13);
            this.InformationLabel.TabIndex = 0;
            this.InformationLabel.Text = "Create custom menu entries here, to be later used from uLaunch\'s main menu.";
            // 
            // MainGroup
            // 
            this.MainGroup.Controls.Add(this.ArgvText);
            this.MainGroup.Controls.Add(this.ArgvLabel);
            this.MainGroup.Controls.Add(this.NroText);
            this.MainGroup.Controls.Add(this.NroLabel);
            this.MainGroup.Controls.Add(this.VersionText);
            this.MainGroup.Controls.Add(this.VersionLabel);
            this.MainGroup.Controls.Add(this.AuthorLabel);
            this.MainGroup.Controls.Add(this.AuthorText);
            this.MainGroup.Controls.Add(this.NameText);
            this.MainGroup.Controls.Add(this.NameLabel);
            this.MainGroup.Location = new System.Drawing.Point(15, 56);
            this.MainGroup.Name = "MainGroup";
            this.MainGroup.Size = new System.Drawing.Size(341, 321);
            this.MainGroup.TabIndex = 1;
            this.MainGroup.TabStop = false;
            this.MainGroup.Text = "Entry settings";
            // 
            // ArgvText
            // 
            this.ArgvText.Location = new System.Drawing.Point(6, 273);
            this.ArgvText.Name = "ArgvText";
            this.ArgvText.Size = new System.Drawing.Size(329, 20);
            this.ArgvText.TabIndex = 9;
            // 
            // ArgvLabel
            // 
            this.ArgvLabel.AutoSize = true;
            this.ArgvLabel.Location = new System.Drawing.Point(6, 257);
            this.ArgvLabel.Name = "ArgvLabel";
            this.ArgvLabel.Size = new System.Drawing.Size(55, 13);
            this.ArgvLabel.TabIndex = 8;
            this.ArgvLabel.Text = "NRO argv";
            // 
            // NroText
            // 
            this.NroText.Location = new System.Drawing.Point(6, 223);
            this.NroText.Name = "NroText";
            this.NroText.Size = new System.Drawing.Size(329, 20);
            this.NroText.TabIndex = 7;
            // 
            // NroLabel
            // 
            this.NroLabel.AutoSize = true;
            this.NroLabel.Location = new System.Drawing.Point(6, 207);
            this.NroLabel.Name = "NroLabel";
            this.NroLabel.Size = new System.Drawing.Size(55, 13);
            this.NroLabel.TabIndex = 6;
            this.NroLabel.Text = "NRO path";
            // 
            // VersionText
            // 
            this.VersionText.Location = new System.Drawing.Point(6, 142);
            this.VersionText.Name = "VersionText";
            this.VersionText.Size = new System.Drawing.Size(329, 20);
            this.VersionText.TabIndex = 5;
            // 
            // VersionLabel
            // 
            this.VersionLabel.AutoSize = true;
            this.VersionLabel.Location = new System.Drawing.Point(6, 126);
            this.VersionLabel.Name = "VersionLabel";
            this.VersionLabel.Size = new System.Drawing.Size(68, 13);
            this.VersionLabel.TabIndex = 4;
            this.VersionLabel.Text = "Entry version";
            // 
            // AuthorLabel
            // 
            this.AuthorLabel.AutoSize = true;
            this.AuthorLabel.Location = new System.Drawing.Point(6, 78);
            this.AuthorLabel.Name = "AuthorLabel";
            this.AuthorLabel.Size = new System.Drawing.Size(64, 13);
            this.AuthorLabel.TabIndex = 3;
            this.AuthorLabel.Text = "Entry author";
            // 
            // AuthorText
            // 
            this.AuthorText.Location = new System.Drawing.Point(6, 94);
            this.AuthorText.Name = "AuthorText";
            this.AuthorText.Size = new System.Drawing.Size(329, 20);
            this.AuthorText.TabIndex = 2;
            // 
            // NameText
            // 
            this.NameText.Location = new System.Drawing.Point(6, 46);
            this.NameText.Name = "NameText";
            this.NameText.Size = new System.Drawing.Size(329, 20);
            this.NameText.TabIndex = 1;
            // 
            // NameLabel
            // 
            this.NameLabel.AutoSize = true;
            this.NameLabel.Location = new System.Drawing.Point(6, 30);
            this.NameLabel.Name = "NameLabel";
            this.NameLabel.Size = new System.Drawing.Size(60, 13);
            this.NameLabel.TabIndex = 0;
            this.NameLabel.Text = "Entry name";
            // 
            // IconGroup
            // 
            this.IconGroup.Controls.Add(this.pictureBox1);
            this.IconGroup.Location = new System.Drawing.Point(368, 56);
            this.IconGroup.Name = "IconGroup";
            this.IconGroup.Size = new System.Drawing.Size(269, 293);
            this.IconGroup.TabIndex = 2;
            this.IconGroup.TabStop = false;
            this.IconGroup.Text = "Entry icon";
            // 
            // pictureBox1
            // 
            this.pictureBox1.Location = new System.Drawing.Point(6, 19);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(256, 256);
            this.pictureBox1.TabIndex = 0;
            this.pictureBox1.TabStop = false;
            // 
            // MakeButton
            // 
            this.MakeButton.Location = new System.Drawing.Point(368, 364);
            this.MakeButton.Name = "MakeButton";
            this.MakeButton.Size = new System.Drawing.Size(269, 39);
            this.MakeButton.TabIndex = 3;
            this.MakeButton.Text = "Create";
            this.MakeButton.UseVisualStyleBackColor = true;
            this.MakeButton.Click += new System.EventHandler(this.MakeButton_Click);
            // 
            // EntryForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(653, 422);
            this.Controls.Add(this.MakeButton);
            this.Controls.Add(this.IconGroup);
            this.Controls.Add(this.MainGroup);
            this.Controls.Add(this.InformationLabel);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "EntryForm";
            this.Text = "uViewer - uLaunch menu entry creator";
            this.MainGroup.ResumeLayout(false);
            this.MainGroup.PerformLayout();
            this.IconGroup.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label InformationLabel;
        private System.Windows.Forms.GroupBox MainGroup;
        private System.Windows.Forms.TextBox VersionText;
        private System.Windows.Forms.Label VersionLabel;
        private System.Windows.Forms.Label AuthorLabel;
        private System.Windows.Forms.TextBox AuthorText;
        private System.Windows.Forms.TextBox NameText;
        private System.Windows.Forms.Label NameLabel;
        private System.Windows.Forms.GroupBox IconGroup;
        private System.Windows.Forms.TextBox ArgvText;
        private System.Windows.Forms.Label ArgvLabel;
        private System.Windows.Forms.TextBox NroText;
        private System.Windows.Forms.Label NroLabel;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.Button MakeButton;
    }
}