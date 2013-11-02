import java.io.*;
import java.net.*;
import java.util.*;

public class Client {
    public static final String HOST = "bicycle.cs.washington.edu";
    public static final int PORT = 12235;

    public static void main(String[] args) throws IOException {
	// set up server thread to receive responses
	DatagramSocket socket = new DatagramSocket(5432);
	ServerThread server = new ServerThread(socket);
	server.start();

	byte[] buf;
	Scanner console = new Scanner(System.in);
	String in;

	System.out.println("Type anything to send");
	while (!(in = console.nextLine()).equalsIgnoreCase("exit")) {
	    byte[] psecret0 = {0, 0, 0, 0};
	    buf = packetize(toByteArray("hello world"), psecret0);
	    printPacket(buf);
	    InetAddress address = InetAddress.getByName(HOST);
	    System.out.println(address);
	    DatagramPacket packet = new DatagramPacket(buf, buf.length, address, PORT);
	    socket.send(packet);
	    System.out.println("Sending a message to " + HOST + ":" + PORT + "...");
	}
	server.stopListening();
	socket.close();
	console.close();
    }

    public static byte[] toByteArray(String s) {
	byte[] res = new byte[s.length() + 1];
	for (int i = 0; i < s.length(); i++)
	    res[i] = (byte) s.charAt(i);
	res[s.length()] = '\0'; // null-terminate
	return res;
    }

    public static void printPacket(byte[] packet) {
	if (packet.length < 12) throw new IllegalArgumentException();
	System.out.print("   payload_len:");
	for (int i = 0; i < 4; i++)
	    System.out.print(" " + packet[i]);
	System.out.println();
	System.out.print("   psecret:");
	for (int i = 0; i < 4; i++)
	    System.out.print(" " + packet[4 + i]);
	System.out.println("   step: " + packet[8] + " " + packet[9]);
	System.out.println("    id: " + packet[10] + " " + packet[11]);
	System.out.print("    payload:");
	for (int i = 12; i < packet.length; i++)
	    System.out.print(" " + packet[i]);
	System.out.println();
    }

    public static byte[] packetize(byte[] payload, byte[] prevSecret) {
	if (prevSecret.length != 4) throw new IllegalArgumentException();
	int length = 12 + payload.length;
	byte[] packet = new byte[length + (4 - length % 4) % 4];
	insertHeader(packet, payload.length, "a1", prevSecret);
	for (int i = 0; i < payload.length; i++)
	    packet[12 + i] = payload[i];
	return packet;
    }

    // inserts a 12 byte header at the beginning of 'packet'
    // throws IllegalArgumentException if header will not fit
    public static void insertHeader(byte[] packet, int trueLength, 
				    String step, byte[] prevSecret) {
	if (packet.length < 12)
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

    public static byte[] toByteArray(int n) {
	byte[] res = new byte[4];
	for (int i = 0; i < 4; i++) {
	    res[4 - 1 - i] = (byte) n;
	    n = n >> 8;
	}
	return res;
    }
}
