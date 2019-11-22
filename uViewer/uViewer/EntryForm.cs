using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Security.Cryptography;
using System.Windows.Forms;
using Newtonsoft.Json.Linq;

namespace uViewer
{
    public partial class EntryForm : Form
    {
        public Plugins.PluginContext Plugin;
        public string LoadedIcon = null;

        public EntryForm(Plugins.PluginContext plugin)
        {
            InitializeComponent();
            Plugin = plugin;

            if(Plugin != null)
            {
                Text += " (" + Plugin.GetPluginName() + ")";

                void SetField(string value, Control control)
                {
                    if(!string.IsNullOrEmpty(value)) control.Text = value;
                }

                void SetControl(Control custom, string parent_control, string actual_name)
                {
                    if (custom != null)
                    {
                        Control control;
                        if (!string.IsNullOrEmpty(parent_control))
                        {
                            var parent = Controls.Find(parent_control, false).First();
                            control = parent.Controls.Find(actual_name, false).First();
                        }
                        else control = Controls.Find(actual_name, false).First();

                        custom.Margin = control.Margin;
                        custom.Location = control.Location;
                        custom.Name = actual_name;

                        if (!string.IsNullOrEmpty(parent_control))
                        {
                            var parent = Controls.Find(parent_control, true).First();
                            parent.Controls.RemoveByKey(actual_name);
                            parent.Controls.Add(custom);
                        }

                        else
                        {
                            Controls.RemoveByKey(actual_name);
                            Controls.Add(custom);
                        }
                        
                    }
                }

                SetField(Plugin.GetEntryMenuInformationLabel(), InformationLabel);
                SetField(Plugin.GetEntryMenuNameFieldLabel(), NameLabel);
                SetControl(Plugin.GetEntryMenuNameControl(), "MainGroup", "NameText");
                SetField(Plugin.GetEntryMenuAuthorFieldLabel(), AuthorLabel);
                SetControl(Plugin.GetEntryMenuAuthorControl(), "MainGroup", "AuthorText");
                SetField(Plugin.GetEntryMenuVersionFieldLabel(), VersionLabel);
                SetControl(Plugin.GetEntryMenuVersionControl(), "MainGroup", "VersionText");
                SetField(Plugin.GetEntryMenuNroFieldLabel(), NroLabel);
                SetControl(Plugin.GetEntryMenuNroControl(), "MainGroup", "NroText");
                SetField(Plugin.GetEntryMenuArgvFieldLabel(), ArgvLabel);
                SetControl(Plugin.GetEntryMenuArgvControl(), "MainGroup", "ArgvText");
            }
        }

        private void MakeButton_Click(object sender, EventArgs e)
        {
            string name = null;
            string author = null;
            string version = null;
            string nro = null;
            string argv = null;

            if(Plugin != null)
            {
                string GetOutValue(Func<Control, string> fn, string parent_control, string ctrlname)
                {
                    Control control;
                    if (!string.IsNullOrEmpty(parent_control))
                    {
                        var parent = Controls.Find(parent_control, false).First();
                        control = parent.Controls.Find(ctrlname, false).First();
                    }
                    else control = Controls.Find(ctrlname, false).First();

                    if (Plugin == null) return control.Text;
                    else return fn(control);
                }

                name = GetOutValue(Plugin.GetEntryMenuNameControlSelectedValue, "MainGroup", "NameText");
                author = GetOutValue(Plugin.GetEntryMenuAuthorControlSelectedValue, "MainGroup", "AuthorText");
                version = GetOutValue(Plugin.GetEntryMenuVersionControlSelectedValue, "MainGroup", "VersionText");
                nro = GetOutValue(Plugin.GetEntryMenuNroControlSelectedValue, "MainGroup", "NroText");
                argv = GetOutValue(Plugin.GetEntryMenuArgvControlSelectedValue, "MainGroup", "ArgvText");
            }
            else
            {
                name = NameText.Text;
                author = AuthorText.Text;
                version = VersionText.Text;
                nro = NroText.Text;
                argv = ArgvText.Text;
            }

            dynamic dynjson = new JObject();
            dynjson.madeby = "uViewer";
            dynjson.type = 2;

            if(string.IsNullOrEmpty(nro))
            {
                var def = "A NRO path needs to be specified.";
                if(Plugin != null) def = Plugin.GetNroPathNotSetError();
                MessageBox.Show(def);
                return;
            }
            else dynjson.nro_path = nro;
            if(!string.IsNullOrEmpty(argv)) dynjson.nro_argv = argv;
            if(!string.IsNullOrEmpty(name)) dynjson.name = name;
            if(!string.IsNullOrEmpty(author)) dynjson.author = author;
            if(!string.IsNullOrEmpty(version)) dynjson.version = version;

            if(!string.IsNullOrEmpty(LoadedIcon))
            {
                if(!LoadedIcon.StartsWith(Utils.SDDrivePath))
                {
                    var rc = MessageBox.Show("Since the selected icon isn't in the SD card, a copy will be made in 'uviewer_meta' directory to be used by uLaunch. Continue with the creation?", "uViewer - Entry creation", MessageBoxButtons.YesNo);
                    if(rc != DialogResult.Yes) return;

                    Random r = new Random();
                    Directory.CreateDirectory(Path.Combine(Utils.SDDrivePath, "ulaunch", "uviewer_meta"));
                    var newfile = Path.Combine(Utils.SDDrivePath, "ulaunch", "uviewer_meta", r.Next().ToString() + Path.GetExtension(LoadedIcon));
                    IconPicture.Image.Save(newfile);
                    dynjson.icon = newfile.Replace(Utils.SDDrivePath, "sdmc:/").Replace('\\', '/');
                }
                else dynjson.icon = LoadedIcon.Replace(Utils.SDDrivePath, "sdmc:/").Replace('\\', '/');
            }

            string json = dynjson.ToString();

            SHA256 cd = SHA256.Create();
            var jsonhash = cd.ComputeHash(Encoding.UTF8.GetBytes(json));
            var fname = "uViewer-" + BitConverter.ToString(jsonhash).Replace("-", "").Substring(0, 32) + ".json";

            string entry = Path.Combine(Utils.SDDrivePath, "ulaunch", "entries", fname);
            File.WriteAllText(entry, json);

            MessageBox.Show("The entry was saved in uLaunch's entry directory as '" + fname + "'.");
        }

        private void IconButton_Click(object sender, EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog
            {
                Title = "Select entry icon",
                Filter = "Image (*.png, *.jpg, *.bmp)|*.png;*.jpg;*.bmp",
                Multiselect = false
            };
            if(ofd.ShowDialog() == DialogResult.OK)
            {
                var bmp = new Bitmap(ofd.FileName);
                IconPicture.Image = bmp;
                LoadedIcon = ofd.FileName;
            }
        }
    }
}
