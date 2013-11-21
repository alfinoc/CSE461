/*
    Chris Alfino
    1024472
    CSE 461
    
    This class provides some utilities for transforming between integers and
    bytes, combining arrays of bytes into protocol-compliant packets, and
    printing for various debugging purposes.
*/

import java.io.OutputStream;
import java.io.InputStream;
import java.io.ByteArrayOutputStream;
import java.net.Socket;
import java.util.Arrays;

public class PacketUtil {
    public static final int SIZE_HEADER = 12;
    public static final int SIZE_INT = 4;

    // returns the integer resulting from interpreting the four bytes
    // starting at 'start' in 'data' as an integer (big-endian). throws
    // IllegalArgumentException if 'start' is not a possible starting point.
    public static int extractInt(byte[] data, int start) {
        int end = start + SIZE_INT;
        if (start < 0 || end > data.length)
            throw new IllegalArgumentException();
        int n = 0;
        for (int i = start; i < end; i++) {
            n = n << 8;
            n += (int) data[i] & 0xFF;
        }
        return n;
    }

    // returns an array of bytes storing the characters of s, including
    // a single number null-terminator at the end
    public static byte[] toByteArray(String s) {
        byte[] res = new byte[s.length() + 1];
        for (int i = 0; i < s.length(); i++)
            res[i] = (byte) s.charAt(i);
        res[s.length()] = '\0'; // null-terminate
        return res;
    }

    // returns a 4-byte array storing the contents of n in Java's
    // default big-endian order.
    public static byte[] toByteArray(int n) {
        byte[] res = new byte[SIZE_INT];
        for (int i = 0; i < SIZE_INT; i++) {
            res[SIZE_INT - 1 - i] = (byte) n;
            n = n >> 8;
        }
        return res;
    }

    // returns a byte[] "packet" with an appropriate header based on
    // 'prevSecret' and 'step' leading the 'payload'. packet is padded
    // to four-bytes
    public static byte[] packetize(byte[] payload, byte[] prevSecret,
                                   String step) {
        if (prevSecret.length != 4) throw new IllegalArgumentException();
        int length = SIZE_HEADER + payload.length;
        byte[] packet = new byte[length +
                                 (SIZE_INT - length % SIZE_INT) % SIZE_INT];
        insertHeader(packet, payload.length, step, prevSecret);
        for (int i = 0; i < payload.length; i++)
            packet[12 + i] = payload[i];
        return packet;
    }

    // inserts a 12 byte header at the beginning of 'packet'
    // throws IllegalArgumentException if header will not fit
    public static void insertHeader(byte[] packet, int trueLength,
                                    String step, byte[] prevSecret) {
        if (packet.length < SIZE_HEADER)
            throw new IllegalArgumentException();

        // 4-byte length
        byte[] length = toByteArray(trueLength);
        for (int i = 0; i < 4; i++)
            packet[i] = length[i];

        // 4-byte prev secret
        for (int i = 0; i < 4; i++)
            packet[4 + i] = prevSecret[i];

        // 2-byte step number
        packet[8] = (byte) 0;
        packet[9] = (byte) Integer.parseInt("" + step.charAt(1));

        // 2-byte student number;
        packet[10] = (byte) 0x1;
        packet[11] = (byte) 0xD8;
    }

    // reads buf.length bytes from 'socket' into 'buf', returning true if
    // all bytes are read in, and false if an error occurred or 'buf' was
    // not filled socket must be connected
    public static boolean read(Socket socket, byte[] buf) {
        try {
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            InputStream in = socket.getInputStream();
            int s;
            int total = 0;
            while (total < buf.length - SIZE_HEADER && (s = in.read(buf)) != -1) {
                total += s;
                baos.write(buf, 0, s);
            }
            return total == buf.length;
        } catch (Exception e) {
            return false;
        }
    }

    // prints the contents of the packet. if the packet is too small
    // to have the header used in this project, throws IllegalArgumentException
    // otherwise, each field is printed
    public static void printPacket(byte[] packet) {
        System.out.println("PACKET CONTENTS");
        if (packet.length < SIZE_HEADER) throw new IllegalArgumentException(
            "packet length is only " + packet.length + " (bytes)");
        System.out.println("   payload_len: " + extractInt(packet, 0));
        System.out.println("   psecret: " + extractInt(packet, 4));
        System.out.print("   step: " + packet[8] + " " + packet[9] + " ");
        System.out.println("   id: " + packet[10] + " " + packet[11]);
        System.out.printf("   payload + padding (%d bytes):\n",
                          packet.length - SIZE_HEADER);
        printBytes(packet, 12, packet.length);
        System.out.println();
    }

    // prints the contents of a from indices 'start' to 'end' exclusive, 4
    // bytes per line, space separated, in hexidecimal.
    public static void printBytes(byte[] a, int start, int end) {
        if (start < 0 || start > end || end > a.length)
            throw new IllegalArgumentException();

        for (int i = start; i < end; i++) {
            if (i % 4 == 0)
            System.out.print(i + "\t| ");
            System.out.printf("%x ", a[i]);
            if (i % 4 == 3)
            System.out.println();
        }
    }
}
