import java.io.*;
import java.net.*;
import java.util.*;

public class Client {
    public static final int SIZE_HEADER = 12;
    public static final int SIZE_INT = 4;

    public static final String HOST = "bicycle.cs.washington.edu";
    public static final int PORT = 12235;

    public static void main(String[] args) throws IOException {
	// step a
	byte[] resA = stepA();
	int num = PacketUtil.extractInt(resA, SIZE_HEADER);
	int len = PacketUtil.extractInt(resA, SIZE_HEADER + 4);
	int udpPort = PacketUtil.extractInt(resA, SIZE_HEADER + 8);
	int secretA = PacketUtil.extractInt(resA, SIZE_HEADER + 12);
	System.out.println("STEP A RESPONSE: ");
	PacketUtil.printBytes(resA, 0, resA.length);
	System.out.println("   num:    " + num);
	System.out.println("   len:    " + len);
	System.out.println("   port:   " + udpPort);
	System.out.println("   secret: " + secretA);
	System.out.println();

	// step b
	stepB(num, len, udpPort, secretA);
	
    }

    // runs stepA, returning the received packet's data
    public static byte[] stepA() throws IOException {
	System.out.println("Performing STEP A");
	DatagramSocket socket = new DatagramSocket(5432);
	socket.setSoTimeout(5000);
	byte[] psecret0 = {0, 0, 0, 0};
	byte[] bufOut = PacketUtil.packetize(PacketUtil.toByteArray("hello world"),
					  psecret0, "a1");
	InetAddress address = InetAddress.getByName(HOST);
	DatagramPacket out = new DatagramPacket(bufOut, bufOut.length, address, PORT);
	socket.send(out);
	byte[] bufIn = new byte[SIZE_HEADER + 4 * SIZE_INT];
	DatagramPacket in = new DatagramPacket(bufIn, bufIn.length);
	socket.receive(in);
	socket.close();
	return bufIn;
    }
    
    public static void stepB(int num, int len,
			     int udpPort, int secretA) throws IOException {
	DatagramSocket socket = new DatagramSocket(udpPort);
	socket.setSoTimeout(500);

	InetAddress address = InetAddress.getByName(HOST);

	for (int i = 0; i < num; i++) {
	    byte[] payload = new byte[SIZE_INT + len];
	    byte[] id = PacketUtil.toByteArray(i);
	    for (int j = 0; j < id.length; j++)
		payload[j] = id[j];
	    byte[] bufOut = PacketUtil.packetize(payload, PacketUtil.toByteArray(secretA), "b1");
	    byte[] bufIn = new byte[SIZE_HEADER + SIZE_INT];

	    DatagramPacket out = new DatagramPacket(bufOut, bufOut.length,
						    address, udpPort);
	    DatagramPacket in = new DatagramPacket(bufIn, bufIn.length);

	    for (int j = 0; j < 4; j++) {
		System.out.println("sending to " + out.getAddress() + ":" + out.getPort() + "...");
		socket.send(out);
		
		PacketUtil.printPacket(out.getData());

		try {
		    socket.receive(in);
		    System.out.println("SUCCESS");
		} catch (SocketTimeoutException e) {
		    System.out.println("timeout");
		}
	    }
	}
	socket.close();
    }
}
