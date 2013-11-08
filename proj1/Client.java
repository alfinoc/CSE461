import java.io.*;
import java.net.*;
import java.util.*;

public class Client {
    public static final int SIZE_HEADER = 12;
    public static final int SIZE_INT = 4;

    public static final String HOST = "bicycle.cs.washington.edu";

    public static void main(String[] args) throws IOException {
	// step a
	System.out.println("Performing STEP A");
	byte[] resA = stepA();
	int num = PacketUtil.extractInt(resA, SIZE_HEADER);
	int len = PacketUtil.extractInt(resA, SIZE_HEADER + SIZE_INT);
	int udpPort = PacketUtil.extractInt(resA, SIZE_HEADER + SIZE_INT * 2);
	int secretA = PacketUtil.extractInt(resA, SIZE_HEADER + SIZE_INT * 3);

	// step b
	System.out.print("Performing STEP B");
	byte[] resB = stepB(num, len, udpPort, secretA);
	int tcpPort = PacketUtil.extractInt(resB, SIZE_HEADER);
	int secretB = PacketUtil.extractInt(resB, SIZE_HEADER + SIZE_INT);

	// step c
	System.out.println("Performing STEP C");
	Socket tcpSocket = new Socket(HOST, tcpPort);
	byte[] resC = stepC(secretB, tcpSocket);
	int num2 = PacketUtil.extractInt(resC, SIZE_HEADER);
	int len2 = PacketUtil.extractInt(resC, SIZE_HEADER + SIZE_INT);
	int secretC = PacketUtil.extractInt(resC, SIZE_HEADER + SIZE_INT * 2);
	byte c = resC[SIZE_HEADER + SIZE_INT * 3];

	// step d
	System.out.println("Performing STEP D");
	byte[] resD = stepD(num2, len2, c, secretC, tcpSocket);
	int secretD = PacketUtil.extractInt(resD, SIZE_HEADER);

	System.out.println();
	printSecrets(secretA, secretB, secretC, secretD);
    }

    // prints the four secrets 'a' - 'd', one per line
    public static void printSecrets(int a, int b, int c, int d) {
	System.out.println("secret a: " + a);
	System.out.println("secret b: " + b);
	System.out.println("secret c: " + c);
	System.out.println("secret d: " + d);
    }

    // runs step d, returning the received packets data
    // step d involves sending 'num' payloads of length 'len' full of byte 'c'
    // on 'socket'. 'secretC' is sent in the header.
    public static byte[] stepD(int num, int len, byte c,
			     int secretC, Socket socket) throws IOException {
	byte[] payload = new byte[len];
	Arrays.fill(payload, c);
	byte[] buf;

	// send 'num' payloads
	for (int i = 0; i < num; i++) {
	    buf = PacketUtil.packetize(payload, PacketUtil.toByteArray(secretC), "d1");
	    socket.getOutputStream().write(buf);
	}

	// read response
	buf = new byte[SIZE_HEADER + SIZE_INT];
	PacketUtil.read(socket, buf);
	socket.close();
	return buf;
    }

    // runs step c, returning the received packet's data
    // step c involves sending openning a connection on 'socket' to stepC and receiving
    // a 16-byte message.
    public static byte[] stepC(int secretB, Socket socket) throws IOException {
	byte[] buf = new byte[SIZE_HEADER + 4 * SIZE_INT];
	PacketUtil.read(socket, buf);
	return buf;
    }

    // runs stepA, returning the received packet's data
    // step a involves sending a "hello world" payload to 'HOST' on port 12235
    public static byte[] stepA() throws IOException {
	DatagramSocket socket = new DatagramSocket();
	socket.setSoTimeout(5000);
	byte[] psecret0 = {0, 0, 0, 0};
	byte[] bufOut = PacketUtil.packetize(PacketUtil.toByteArray("hello world"),
					     psecret0, "a1");
	InetAddress address = InetAddress.getByName(HOST);
	DatagramPacket out = new DatagramPacket(bufOut, bufOut.length, address, 12235);
	socket.send(out);
	byte[] bufIn = new byte[SIZE_HEADER + 4 * SIZE_INT];
	DatagramPacket in = new DatagramPacket(bufIn, bufIn.length);
	socket.receive(in);
	socket.close();
	return bufIn;
    }

    // runs set b, returning the received packet's data
    // step b involves sending 'num' udp packets, each with payload length len + 4 to
    // 'HOST' on 'udpPort', with 'secretA' in the header
    public static byte[] stepB(int num, int len,
			       int udpPort, int secretA) throws IOException {
	DatagramSocket socket = new DatagramSocket();
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

	    boolean timeout = true;
	    while (timeout) {
		socket.send(out);
		try {
		    socket.receive(in);
		    timeout = false;
		} catch (SocketTimeoutException e) {
		    timeout = true;
		}
		System.out.print(".");
	    }
	}
	System.out.println();
	byte[] bufIn = new byte[SIZE_HEADER + 2 * SIZE_INT];
	DatagramPacket in = new DatagramPacket(bufIn, bufIn.length);
	socket.receive(in);
	socket.close();
	return bufIn;
    }
}
