using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Windows.Forms;

namespace uViewer.RetroPlugin
{
    public class RetroPlugin : Plugins.PluginContext
    {
        public override string GetPluginName() => "RetroArch plugin";

        public override string GetPluginDescription() => "RetroArch forwarder creator plugin";

        private string sd_root = null;
        private string selected_rom = null;

        public override string CanLoadPlugin(string sd_path)
        {
            sd_root = sd_path;

            if(!Directory.Exists(Path.Combine(sd_path, "retroarch"))) return "RetroArch doesn't seem to be installed in this SD card.";
            if(!Directory.Exists(Path.Combine(sd_path, "retroarch", "cores"))) return "No available cores were found.";
            var cores = Directory.GetFiles(Path.Combine(sd_path, "retroarch", "cores")).Where((file) => file.EndsWith("_libretro_libnx.nro"));
            if(!cores.Any()) return "No available cores were found.";
            return null;
        }

        public override string GetNroPathNotSetError() => "A RetroArch core needs to be specified.";

        public override string GetEntryMenuInformationLabel() => "Create forwarders for ROMs in the SD card";

        // Name

        public override string GetEntryMenuNameFieldLabel() => "ROM name";

        // Author

        public override string GetEntryMenuAuthorFieldLabel() => "Producer";

        // Version

        public override string GetEntryMenuVersionFieldLabel() => "Platform";

        // Nro

        public override string GetEntryMenuNroFieldLabel() => "RetroArch core (emulator)";

        public override Control GetEntryMenuNroControl()
        {
            ComboBox box = new ComboBox
            {
                Width = 300
            };

            var cores = Directory.GetFiles(Path.Combine(sd_root, "retroarch", "cores")).Where((file) => file.EndsWith("_libretro_libnx.nro"));
            foreach(var core in cores)
            {
                box.Items.Add(Path.GetFileNameWithoutExtension(core));
            }

            return box;
        }

        public override string GetEntryMenuNroControlSelectedValue(Control control)
        {
            if(((ComboBox)control).SelectedIndex < 0) return null;

            return Path.Combine(sd_root, "retroarch", "cores", ((ComboBox)control).SelectedItem as string + ".nro").Replace(sd_root, "sdmc:/").Replace('\\', '/');
        }

        // Argv

        public override string GetEntryMenuArgvFieldLabel() => "ROM path";

        public override Control GetEntryMenuArgvControl()
        {
            Button b = new Button()
            {
                Text = "Browse ROM file"
            };

            b.Width = 250;

            b.Click += new EventHandler((sender, e) =>
            {
                OpenFileDialog ofd = new OpenFileDialog
                {
                    Title = "Select ROM file",
                    InitialDirectory = sd_root,
                    Multiselect = false
                };

                if(ofd.ShowDialog() == DialogResult.OK)
                {
                    var tmpfile = ofd.FileName;
                    if(!tmpfile.StartsWith(sd_root))
                    {
                        var rc = MessageBox.Show("The ROM will be copied to the SD card with a random name (ulaunch/uviewer_meta/retro/). Continue?", "uViewer (retro) - Selecting non-SD ROM file", MessageBoxButtons.YesNo);
                        if(rc != DialogResult.Yes) return;

                        Random r = new Random();
                        Directory.CreateDirectory(Path.Combine(sd_root, "ulaunch", "uviewer_meta", "retro"));
                        var newfile = Path.Combine(sd_root, "ulaunch", "uviewer_meta", "retro", r.Next().ToString() + Path.GetExtension(tmpfile));
                        File.Copy(tmpfile, newfile);
                        selected_rom = newfile.Replace(sd_root, "sdmc:/").Replace('\\', '/');
                        MessageBox.Show("Copied ROM: \"" + selected_rom + "\"");
                    }
                    else
                    {
                        selected_rom = tmpfile.Replace(sd_root, "sdmc:/").Replace('\\', '/');
                        MessageBox.Show("Selected ROM: \"" + selected_rom + "\"");
                    }
                }
            });

            return b;
        }

        public override string GetEntryMenuArgvControlSelectedValue(Control control)
        {
            if(string.IsNullOrEmpty(selected_rom)) return null;
            return "\"" + selected_rom.Replace(sd_root, "sdmc:/").Replace('\\', '/') + "\"";
        }
    }
}
