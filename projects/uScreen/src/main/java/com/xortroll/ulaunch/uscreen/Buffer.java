package com.xortroll.ulaunch.uscreen;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class Buffer {
    private ByteBuffer buf;

    public Buffer(byte[] data) {
        this.buf = ByteBuffer.wrap(data);
        this.buf.order(ByteOrder.LITTLE_ENDIAN);
    }

    public int read32() {
        return this.buf.getInt();
    }

    public long read64() {
        return this.buf.getLong();
    }

    public byte[] readBytes(int length) {
        byte[] data = new byte[length];
        this.buf.get(data);
        return data;
    }

    public void write32(int val) {
        this.buf.putInt(val);
    }

    public void write64(long val) {
        this.buf.putLong(val);
    }

    public void writeBytes(byte[] data) {
        this.buf.put(data);
    }

    public void setPosition(int pos) {
        this.buf.position(pos);
    }

    public int getPosition() {
        return this.buf.position();
    }
}