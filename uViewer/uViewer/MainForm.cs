using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Diagnostics;
using System.Threading.Tasks;
using System.Windows.Forms;
using libusbK;

namespace uViewer
{
    public partial class MainForm : Form
    {
        public UsbK USB = null;
        public ViewerMainForm Viewer = null;
        public EntryForm Entry = null;
        public Plugins.PluginContext LoadedPlugin = null;
        
        public MainForm()
        {
            InitializeComponent();
            ViewerButton.Enabled = Utils.IsWindows();

            if(Utils.IsWindows())
            {
                try
                {
                    var pat = new KLST_PATTERN_MATCH { DeviceID = @"USB\VID_057E&PID_3000" };
                    var lst = new LstK(0, ref pat);
                    lst.MoveNext(out var dinfo);
                    USB = new UsbK(dinfo);
                }
                catch
                {
                }

                if(USB == null) ViewerButton.Enabled = false;
            }

            var drive = Utils.DetectSDCardDrive();
            if(drive != null)
            {
                EntryManagerButton.Enabled = true;
                PluginButton.Enabled = true;
                FoundDriveText.Text = "Found SD card on drive \"" + drive.VolumeLabel + "\" (" + drive.Name + ")";
            }
        }

        private void ViewerButton_Click(object sender, EventArgs e)
        {
            if(Viewer != null && Viewer.IsDisposed) Viewer = null;
            if(Viewer == null) Viewer = new ViewerMainForm(USB) { Owner = this };
            Viewer.Show();
        }

        private void EntryManagerButton_Click(object sender, EventArgs e)
        {
            if(LoadedPlugin != null)
            {
                var val = LoadedPlugin.CanLoadPlugin(Utils.SDDrivePath);
                if(!string.IsNullOrEmpty(val))
                {
                    MessageBox.Show("Plugin refused to load: " + val);
                    return;
                }
            }

            if(Entry != null && Entry.IsDisposed) Entry = null;
            if(Entry == null) Entry = new EntryForm(LoadedPlugin) { Owner = this };
            Entry.Show();
        }

        private void PluginButton_Click(object sender, EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog
            {
                Title = "Load uViewer entry creator plugin",
                Filter = "Dynamic library (*.dll)|*.dll",
                Multiselect = false
            };
            if(ofd.ShowDialog() == DialogResult.OK)
            {
                var plugin = PluginHandler.LoadPluginLibrary(ofd.FileName);
                LoadedPlugin = plugin;
                if(LoadedPlugin != null)
                {
                    PluginLabel.Text = "Selected plugin: " + LoadedPlugin.GetPluginName();
                    PluginUnloadButton.Enabled = true;
                }
                else PluginLabel.Text = "There is no entry creator plugin selected.";
            }
        }

        private void PluginUnloadButton_Click(object sender, EventArgs e)
        {
            if(LoadedPlugin != null)
            {
                LoadedPlugin = null;
                PluginLabel.Text = "There is no entry creator plugin selected.";
                PluginUnloadButton.Enabled = false;
            }
        }

        private void AboutButton_Click(object sender, EventArgs e)
        {
            Process.Start("https://github.com/XorTroll/uLaunch");
        }
    }
}
