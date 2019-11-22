using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;
using System.Windows.Forms;

namespace uViewer.Plugins
{
    public abstract class PluginContext
    {

        public abstract string GetPluginName();

        public abstract string GetPluginDescription();

        public abstract string CanLoadPlugin(string sd_path);

        public abstract string GetNroPathNotSetError();

        public abstract string GetEntryMenuInformationLabel();

        // Name

        public abstract string GetEntryMenuNameFieldLabel();

        public virtual Control GetEntryMenuNameControl()
        {
            return null;
        }

        public virtual string GetEntryMenuNameControlSelectedValue(Control control)
        {
            return control.Text;
        }

        // Author

        public abstract string GetEntryMenuAuthorFieldLabel();

        public virtual Control GetEntryMenuAuthorControl()
        {
            return null;
        }

        public virtual string GetEntryMenuAuthorControlSelectedValue(Control control)
        {
            return control.Text;
        }

        // Version

        public abstract string GetEntryMenuVersionFieldLabel();

        public virtual Control GetEntryMenuVersionControl()
        {
            return null;
        }

        public virtual string GetEntryMenuVersionControlSelectedValue(Control control)
        {
            return control.Text;
        }

        // Nro

        public abstract string GetEntryMenuNroFieldLabel();

        public virtual Control GetEntryMenuNroControl()
        {
            return null;
        }

        public virtual string GetEntryMenuNroControlSelectedValue(Control control)
        {
            return control.Text;
        }

        // Argv

        public abstract string GetEntryMenuArgvFieldLabel();

        public virtual Control GetEntryMenuArgvControl()
        {
            return null;
        }

        public virtual string GetEntryMenuArgvControlSelectedValue(Control control)
        {
            return control.Text;
        }
    }
}
