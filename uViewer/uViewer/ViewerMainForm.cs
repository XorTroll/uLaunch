using libusbK;
using System;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Threading;
using System.Windows.Forms;

namespace uViewer
{
    public enum USBMode : uint
    {
        Invalid,
        RawRGBA,
        JPEG,
    }

    public partial class ViewerMainForm : Form
    {
        private UsbK USB = null;
        private Thread USBThread = null;

        private ToolboxForm Toolbox = null;
        private static MemoryStream BaseStream = new MemoryStream((int)PlainRgbaScreenBufferSize);
        private static USBMode Mode = USBMode.Invalid;

        public delegate void ApplyTypeImplDelegate(PictureBox Box, byte[] Data);
        public static ApplyTypeImplDelegate ApplyModeDelegate = null;

        public delegate void ApplyDelegate(byte[] data);

        private static unsafe void ApplyRGBA(PictureBox Box, byte[] RGBA)
        {
            var bmp = Box.Image as Bitmap;
            BitmapData img = bmp.LockBits(new Rectangle(0, 0, 1280, 720), ImageLockMode.ReadWrite, bmp.PixelFormat);

            fixed (byte* raw_data = RGBA) unchecked
            {
                uint* ptr = (uint*)raw_data;
                ptr++;
                uint* image = (uint*)img.Scan0.ToPointer();
                uint* ptr_end = ptr + 720 * 1280;
                while (ptr != ptr_end)
                {
                    uint argb = *ptr << 8;
                    *image = ((argb & 0x0000FF00) << 8) | ((argb & 0x00FF0000) >> 8) | ((argb & 0xFF000000) >> 24) | 0xFF000000;
                    image++;
                    ptr++;
                }
            }

            bmp.UnlockBits(img);
        }

        private static void ApplyJPEG(PictureBox Box, byte[] JPEG)
        {
            BaseStream.Position = 0;
            BaseStream.Write(JPEG, 4, (int)PlainRgbaScreenBufferSize);
            Box.Image = Image.FromStream(BaseStream);
        }

        public const long PlainRgbaScreenBufferSize = 1280 * 720 * 4;
        public const long USBPacketSize = PlainRgbaScreenBufferSize + 4;

        public byte[][] CaptureBlocks = new byte[][]
        {
            new byte[USBPacketSize], // Current block
            new byte[USBPacketSize], // Temporary blocks (5)
            new byte[USBPacketSize],
            new byte[USBPacketSize],
            new byte[USBPacketSize],
            new byte[USBPacketSize],
        };

        public ViewerMainForm(UsbK USB)
        {
            InitializeComponent();
            InitializePictureBox(CaptureBox);

            this.USB = USB;

            Toolbox = new ToolboxForm(this);
            Toolbox.Show();

            USBThread = new Thread(new ThreadStart(USBThreadMain));
            USBThread.Start();
        }

        protected override void OnClosing(CancelEventArgs e)
        {
            Toolbox.Close();
            USBThread.Abort();
            base.OnClosing(e);
        }

        protected override void OnShown(EventArgs e)
        {
            if(USB == null)
            {
                MessageBox.Show("Unable to connect to uLaunch via USB-C cable.", "Unable to connect");
                Environment.Exit(1);
            }
            base.OnShown(e);
        }

        public void USBThreadMain()
        {
            while(RefreshCapture());
        }

        public bool RefreshCapture()
        {
            try
            {
                USB.ReadPipe(0x81, CaptureBlocks[0], (int)USBPacketSize, out _, IntPtr.Zero);
                if(Mode == USBMode.Invalid)
                {
                    var mode_raw = BitConverter.ToUInt32(CaptureBlocks[0], 0);
                    Mode = (USBMode)mode_raw;
                    switch(Mode)
                    {
                        case USBMode.RawRGBA:
                            ApplyModeDelegate = ApplyRGBA;
                            break;
                        case USBMode.JPEG:
                            ApplyModeDelegate = ApplyJPEG;
                            break;
                        default:
                            break;
                    }
                }
                Buffer.BlockCopy(CaptureBlocks[4], 0, CaptureBlocks[5], 0, (int)USBPacketSize);
                Buffer.BlockCopy(CaptureBlocks[3], 0, CaptureBlocks[4], 0, (int)USBPacketSize);
                Buffer.BlockCopy(CaptureBlocks[2], 0, CaptureBlocks[3], 0, (int)USBPacketSize);
                Buffer.BlockCopy(CaptureBlocks[1], 0, CaptureBlocks[2], 0, (int)USBPacketSize);
                Buffer.BlockCopy(CaptureBlocks[0], 0, CaptureBlocks[1], 0, (int)USBPacketSize);
                ApplyDataImpl(CaptureBlocks[0]);
            }
            catch
            {
                return false;
            }
            return true;
        }

        public static void InitializePictureBox(PictureBox Box)
        {
            Box.Image = new Bitmap(1280, 720, PixelFormat.Format32bppArgb);
        }

        public void ApplyDataImpl(byte[] data)
        {
            if(CaptureBox.InvokeRequired)
            {
                var d = new ApplyDelegate(ApplyDataImpl);
                CaptureBox.Invoke(d, data);
            }
            else
            {
                ApplyModeDelegate(CaptureBox, data);
                Refresh();
            }
        }
    }
}
