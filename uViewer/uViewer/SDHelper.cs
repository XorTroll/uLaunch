using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Drawing;
using System.Windows.Forms;
using System.Text;

namespace uViewer
{
    public static class SDHelper
    {
        public static List<string> ListNROsByPath(string sd_name, string path)
        {
            var fullpath = Path.Combine(sd_name, path);
            var nros = Directory.GetFiles(fullpath, "*.nro", SearchOption.AllDirectories);
            return nros.ToList();
        }

        public static (string name, string author, string version, Bitmap icon, long size) GetNROInformation(string nro_path)
        {
            string nroname = null;
            string nroauthor = null;
            string nrover = null;
            Bitmap nroicon = null;
            long nrosize = 0;

            FileStream fs = new FileStream(nro_path, FileMode.Open);
            BinaryReader br = new BinaryReader(fs);

            br.ReadBytes(16);

            uint magic = br.ReadUInt32();

            if(magic == 0x304F524E)
            {
                br.ReadUInt32();
                uint size = br.ReadUInt32();
                br.BaseStream.Position = size;

                uint asthdr = br.ReadUInt32();

                if(asthdr == 0x54455341)
                {
                    br.ReadUInt32();

                    ulong icon_off = br.ReadUInt64();
                    ulong icon_size = br.ReadUInt64();

                    var tmppos = br.BaseStream.Position;
                    br.BaseStream.Position = size + (long)icon_off;
                    var icon = br.ReadBytes((int)icon_size);
                    br.BaseStream.Position = tmppos;

                    nroicon = Image.FromStream(new MemoryStream(icon)) as Bitmap;

                    ulong nacp_off = br.ReadUInt64();
                    ulong nacp_size = br.ReadUInt64();

                    tmppos = br.BaseStream.Position;
                    br.BaseStream.Position = size + (long)nacp_off;
                    var rawnacp = br.ReadBytes((int)nacp_size);
                    br.BaseStream.Position = tmppos;

                    for(var i = 0; i < 16; i++)
                    {
                        int baseoff = i * 0x300;
                        var name = Encoding.UTF8.GetString(rawnacp, baseoff, 0x200);
                        var author = Encoding.UTF8.GetString(rawnacp, baseoff + 0x200, 0x100);

                        if(name.Any() && author.Any())
                        {
                            nroname = name.TrimEnd('\0');
                            nroauthor = author.TrimEnd('\0');
                            break;
                        }
                    }

                    var ver = Encoding.UTF8.GetString(rawnacp, 0x3060, 0x10);
                    nrover = ver.TrimEnd('\0');
                    nrosize = new FileInfo(nro_path).Length;
                }
            }

            fs.Close();
            br.Close();

            return (nroname, nroauthor, nrover, nroicon, nrosize);
        }
    }
}
