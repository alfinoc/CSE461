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

    // prints the contents of the packet. if the packet is too small
    // to have the header used in this project, throws IllegalArgumentException
    // otherwise, each byte is
    public static void printPacket(byte[] packet) {
        if (packet.length < SIZE_HEADER) throw new IllegalArgumentException();
        System.out.print("   payload_len:");
        for (int i = 0; i < 4; i++)
            System.out.print(" " + packet[i]);
        System.out.println();
        System.out.print("   psecret:");
        for (int i = 0; i < 4; i++)
            System.out.print(" " + packet[4 + i]);
        System.out.println("   step: " + packet[8] + " " + packet[9]);
        System.out.println("   id: " + packet[10] + " " + packet[11]);
        System.out.print("   payload:");
        for (int i = 12; i < packet.length; i++)
            System.out.print(" " + packet[i]);
        System.out.println();
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
	// a hacky assertion exception
	if (packet.length % 4 != 0 ||
	    packet.length - payload.length - SIZE_HEADER > 3)
	    throw new IllegalStateException("bad packet length!");
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
        packet[8] = (byte) (step.charAt(0) - 'a');
        packet[9] = (byte) Integer.parseInt("" + step.charAt(1));

        // 2-byte student number;
        packet[10] = (byte) 0x1;
	packet[11] = (byte) 0xD8;
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
}
