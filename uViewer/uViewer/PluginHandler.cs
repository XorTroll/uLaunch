using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;

namespace uViewer
{
    public static class PluginHandler
    {
        public static Plugins.PluginContext LoadPluginLibrary(string path)
        {
            Plugins.PluginContext plugin = null;

            try
            {
                var ldll = Assembly.LoadFile(path);
                var types = ldll.GetExportedTypes();
                if(types.Any())
                {
                    foreach(var type in types)
                    {
                        if(typeof(Plugins.PluginContext) != type)
                        {
                            if(typeof(Plugins.PluginContext).IsAssignableFrom(type))
                            {
                                plugin = Activator.CreateInstance(type) as Plugins.PluginContext;
                            }
                        }
                    }
                }
            }
            catch
            {
            }

            return plugin;
        }
    }
}
