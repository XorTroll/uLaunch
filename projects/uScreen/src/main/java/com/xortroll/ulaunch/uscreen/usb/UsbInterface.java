package com.xortroll.ulaunch.uscreen.usb;

import java.util.Optional;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;
import org.usb4java.*;

public class UsbInterface {
    public static final short VendorId = 0x057E;
    public static final short ProductId = 0x3000;

    public static final byte WriteEndpoint = (byte)0x1;
    public static final byte ReadEndpoint = (byte)0x81;

    private int interface_no;
    private Context context;
    private Device device;
    private DeviceHandle device_handle;

    private UsbInterface(int interface_no) {
        this.interface_no = interface_no;
        this.context = new Context();
        this.device_handle = null;
        this.device = null;
    }

    private synchronized boolean readImpl(ByteBuffer buf, int size) {
        IntBuffer out_read_size_buf = IntBuffer.allocate(1);
        int res = LibUsb.bulkTransfer(this.device_handle, ReadEndpoint, buf, out_read_size_buf, 0);
        int out_read_size = out_read_size_buf.get(0);
        return (res == LibUsb.SUCCESS) && (out_read_size == size);
    }

    private synchronized boolean writeImpl(ByteBuffer buf, int size) {
        IntBuffer out_write_size_buf = IntBuffer.allocate(1);
        int res = LibUsb.bulkTransfer(this.device_handle, WriteEndpoint, buf, out_write_size_buf, 0);
        int out_write_size = out_write_size_buf.get(0);
        return (res == LibUsb.SUCCESS) && (out_write_size == size);
    }

    public byte[] readBytes(int length) {
        ByteBuffer buf = ByteBuffer.allocateDirect(length);
        buf.order(ByteOrder.LITTLE_ENDIAN);
        this.readImpl(buf, length);
        byte[] data = new byte[length];
        buf.get(data);
        return data;
    }

    public boolean readByteBuffer(ByteBuffer buf, int length) {
        return this.readImpl(buf, length);
    }

    public boolean writeBytes(byte[] data) {
        ByteBuffer buf = ByteBuffer.allocateDirect(data.length);
        buf.order(ByteOrder.LITTLE_ENDIAN);
        buf.put(data);
        return this.writeImpl(buf, data.length);
    }

    public static Optional<UsbInterface> createInterface(int interface_no) {
        UsbInterface intf = new UsbInterface(interface_no);
        int res = LibUsb.init(intf.context);
        if(res == LibUsb.SUCCESS) {
            DeviceList devices = new DeviceList();
            int device_count = LibUsb.getDeviceList(intf.context, devices);
            if(device_count > 0) {
                for(Device dev: devices) {
                    DeviceDescriptor device_desc = new DeviceDescriptor();
                    res = LibUsb.getDeviceDescriptor(dev, device_desc);
                    if(res == LibUsb.SUCCESS) {
                        if((device_desc.idVendor() == VendorId) && (device_desc.idProduct() == ProductId)) {
                            intf.device = dev;
                            break;
                        }
                    }
                }

                if(intf.device != null) {
                    intf.device_handle = new DeviceHandle();
                    res = LibUsb.open(intf.device, intf.device_handle);
                    if(res == LibUsb.SUCCESS) {
                        LibUsb.freeDeviceList(devices, true);
                        res = LibUsb.setConfiguration(intf.device_handle, 1);
                        if(res == LibUsb.SUCCESS) {
                            res = LibUsb.claimInterface(intf.device_handle, intf.interface_no);
                            if(res == LibUsb.SUCCESS) {
                                return Optional.of(intf);
                            }
                        }
                    }
                }
            }
        }
        return Optional.empty();
    }

    public void finalize() {
        if(this.device_handle != null) {
            LibUsb.releaseInterface(this.device_handle, this.interface_no);
            LibUsb.close(this.device_handle);
            LibUsb.exit(this.context);
            this.device_handle = null;
        }
    }
}