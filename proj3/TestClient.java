import java.net.*;
import java.util.*;
import java.io.*;

public class TestClient {
    public static final String HOST = "attu4.cs.washington.edu";
    public static final int PORT = 5432;
    public static final int SIZE_INT = 4;

    public static void main(String[] args) throws IOException {
	DatagramSocket sock = new DatagramSocket();

	System.out.println("Sending init packet");
	byte[] buf = toByteArray(3);
	InetAddress address = InetAddress.getByName(HOST);
	DatagramPacket out = new DatagramPacket(buf, buf.length, address, PORT);
	sock.send(out);
	
	System.out.println("Receiving init packet");
	buf = new byte[SIZE_INT * 3];
	DatagramPacket in = new DatagramPacket(buf, buf.length);
	sock.receive(in);
	printBytes(buf);

	sock.close();
	
    }
    
    public static void printBytes(byte[] buf) {
	for (int i = 0; i < buf.length; i++) {
            if (i % 4 == 0)
		System.out.print(i + "\t| ");
            System.out.printf("%x ", buf[i]);
            if (i % 4 == 3)
		System.out.println();
        }
    }

    public static byte[] toByteArray(int n) {
        byte[] res = new byte[SIZE_INT];
        for (int i = 0; i < SIZE_INT; i++) {
            res[SIZE_INT - 1 - i] = (byte) n;
            n = n >> 8;
        }
        return res;
    }
}
