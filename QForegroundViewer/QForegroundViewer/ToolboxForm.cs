using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace QForegroundViewer
{
    public partial class ToolboxForm : Form
    {
        private ViewerMainForm main;

        public ToolboxForm(ViewerMainForm Main)
        {
            InitializeComponent();
            main = Main;
        }

        private void AboutButton_Click(object sender, EventArgs e)
        {
            // TODO: change this before release
            Process.Start("https://github.com/XorTroll/unnamed-qlaunch-reimpl");
        }

        private void ScreenshotButton_Click(object sender, EventArgs e)
        {
            new ScreenshotForm(main).ShowDialog();
        }
    }
}
