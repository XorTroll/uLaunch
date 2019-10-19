namespace QForegroundViewer
{
    partial class ViewerMainForm
    {
        /// <summary>
        /// Variable del diseñador necesaria.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Limpiar los recursos que se estén usando.
        /// </summary>
        /// <param name="disposing">true si los recursos administrados se deben desechar; false en caso contrario.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Código generado por el Diseñador de Windows Forms

        /// <summary>
        /// Método necesario para admitir el Diseñador. No se puede modificar
        /// el contenido de este método con el editor de código.
        /// </summary>
        private void InitializeComponent()
        {
            this.CaptureBox = new System.Windows.Forms.PictureBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.ScreenshotButton = new System.Windows.Forms.Button();
            ((System.ComponentModel.ISupportInitialize)(this.CaptureBox)).BeginInit();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // CaptureBox
            // 
            this.CaptureBox.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
            this.CaptureBox.Location = new System.Drawing.Point(12, 12);
            this.CaptureBox.Name = "CaptureBox";
            this.CaptureBox.Size = new System.Drawing.Size(1280, 720);
            this.CaptureBox.TabIndex = 0;
            this.CaptureBox.TabStop = false;
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.ScreenshotButton);
            this.groupBox1.Location = new System.Drawing.Point(1305, 12);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(200, 282);
            this.groupBox1.TabIndex = 1;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Display options";
            // 
            // ScreenshotButton
            // 
            this.ScreenshotButton.Location = new System.Drawing.Point(20, 19);
            this.ScreenshotButton.Name = "ScreenshotButton";
            this.ScreenshotButton.Size = new System.Drawing.Size(160, 23);
            this.ScreenshotButton.TabIndex = 0;
            this.ScreenshotButton.Text = "Save screenshot";
            this.ScreenshotButton.UseVisualStyleBackColor = true;
            this.ScreenshotButton.Click += new System.EventHandler(this.ScreenshotButton_Click);
            // 
            // ViewerMainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1517, 741);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.CaptureBox);
            this.Name = "ViewerMainForm";
            this.Text = "{qlaunch-reimpl} - Foreground display viewer";
            ((System.ComponentModel.ISupportInitialize)(this.CaptureBox)).EndInit();
            this.groupBox1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.PictureBox CaptureBox;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Button ScreenshotButton;
    }
}

