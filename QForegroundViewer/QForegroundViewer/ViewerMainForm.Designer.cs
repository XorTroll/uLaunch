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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ViewerMainForm));
            this.CaptureBox = new System.Windows.Forms.PictureBox();
            ((System.ComponentModel.ISupportInitialize)(this.CaptureBox)).BeginInit();
            this.SuspendLayout();
            // 
            // CaptureBox
            // 
            this.CaptureBox.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
            this.CaptureBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.CaptureBox.Location = new System.Drawing.Point(0, 0);
            this.CaptureBox.Margin = new System.Windows.Forms.Padding(0);
            this.CaptureBox.Name = "CaptureBox";
            this.CaptureBox.Size = new System.Drawing.Size(1280, 720);
            this.CaptureBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.Zoom;
            this.CaptureBox.TabIndex = 0;
            this.CaptureBox.TabStop = false;
            // 
            // ViewerMainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1280, 720);
            this.Controls.Add(this.CaptureBox);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "ViewerMainForm";
            this.Text = "uLaunch - Foreground display viewer";
            ((System.ComponentModel.ISupportInitialize)(this.CaptureBox)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.PictureBox CaptureBox;
    }
}

