using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing.Imaging;
using System.Drawing;
using System.Linq;
using System.Threading;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using libusbK;

namespace uViewer
{
    public partial class ViewerMainForm : Form
    {
        private UsbK USB = null;
        private Thread USBThread;

        private ToolboxForm Toolbox;

        public const long RawRGBAScreenBufferSize = 1280 * 720 * 4;

        public byte[][] CaptureBlocks = new byte[][]
        {
            new byte[RawRGBAScreenBufferSize], // Current block
            new byte[RawRGBAScreenBufferSize], // Temporary blocks (5)
            new byte[RawRGBAScreenBufferSize],
            new byte[RawRGBAScreenBufferSize],
            new byte[RawRGBAScreenBufferSize],
            new byte[RawRGBAScreenBufferSize],
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
                Environment.Exit(Environment.ExitCode);
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
                USB.ReadPipe(0x81, CaptureBlocks[0], CaptureBlocks[0].Length, out _, IntPtr.Zero);
                Array.Copy(CaptureBlocks[4], CaptureBlocks[5], CaptureBlocks[4].Length);
                Array.Copy(CaptureBlocks[3], CaptureBlocks[4], CaptureBlocks[3].Length);
                Array.Copy(CaptureBlocks[2], CaptureBlocks[3], CaptureBlocks[2].Length);
                Array.Copy(CaptureBlocks[1], CaptureBlocks[2], CaptureBlocks[1].Length);
                Array.Copy(CaptureBlocks[0], CaptureBlocks[1], CaptureBlocks[0].Length);
                ApplyRGBAInternal(CaptureBlocks[0]);
            }
            catch
            {
                return false;
            }
            return true;
        }

        public static void InitializePictureBox(PictureBox Box)
        {
            int w = 1280;
            int h = 720;

            Box.Image = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        }

        public static unsafe void ApplyRGBAToPictureBox(PictureBox Box, byte[] RGBA)
        {
            var bmp = Box.Image as Bitmap;
            BitmapData img = bmp.LockBits(new Rectangle(0, 0, 1280, 720), ImageLockMode.ReadWrite, bmp.PixelFormat);

            fixed (byte* rawData = RGBA) unchecked
            {
                uint* ptr = (uint*)rawData;
                uint* image = (uint*)img.Scan0.ToPointer();
                uint* ptrEnd = ptr + 720 * 1280;
                while (ptr != ptrEnd)
                {
                    uint argb = *ptr << 8;
                    *image = ((argb & 0x0000FF00) << 8) | ((argb & 0x00FF0000) >> 8) | ((argb & 0xFF000000) >> 24) | 0xFF000000;
                    image++; ptr++;
                }
            }

            bmp.UnlockBits(img);
        }

        public delegate void ApplyDelegate(byte[] data);

        public void ApplyRGBAInternal(byte[] data)
        {
            if (CaptureBox.InvokeRequired)
            {
                var d = new ApplyDelegate(ApplyRGBAInternal);
                CaptureBox.Invoke(d, data);
            }
            else
            {
                ApplyRGBAToPictureBox(CaptureBox, data);
                Refresh();
            }
        }
    }
}
