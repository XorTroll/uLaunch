using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;
using System.Reflection;

namespace uViewer
{
    public static class Utils
    {
        public static string SDDrivePath { get; set; }

        public static string GetCwd() => Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);

        public static DriveInfo DetectSDCardDrive()
        {
            if (DriveInfo.GetDrives().Any())
            {
                foreach(DriveInfo driveInfo in DriveInfo.GetDrives())
                {
                    if(driveInfo.IsReady)
                    {
                        // Exists hbmenu.nro?
                        if(File.Exists(Path.Combine(driveInfo.Name, "hbmenu.nro")))
                        {
                            // Exists /switch?
                            if(Directory.Exists(Path.Combine(driveInfo.Name, "switch")))
                            {
                                // Exists /ulaunch?
                                if(Directory.Exists(Path.Combine(driveInfo.Name, "ulaunch")))
                                {
                                    SDDrivePath = driveInfo.Name;
                                    return driveInfo;
                                }
                            }
                        }
                    }
                }
            }

            return null;
        }

        public static bool IsWindows() => Environment.OSVersion.ToString().Contains("Windows");
    }
}
