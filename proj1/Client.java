import java.io.*;
import java.net.*;
import java.util.*;

public class Client {
    public static final int SIZE_HEADER = 12;
    public static final int SIZE_INT = 4;

    public static final String HOST = "bicycle.cs.washington.edu";

    public static void main(String[] args) throws IOException {
	// step a
	byte[] resA = stepA();
	int num = PacketUtil.extractInt(resA, SIZE_HEADER);
	int len = PacketUtil.extractInt(resA, SIZE_HEADER + SIZE_INT);
	int udpPort = PacketUtil.extractInt(resA, SIZE_HEADER + SIZE_INT * 2);
	int secretA = PacketUtil.extractInt(resA, SIZE_HEADER + SIZE_INT * 3);

	// step b
	byte[] resB = stepB(num, len, udpPort, secretA);
	int tcpPort = PacketUtil.extractInt(resB, SIZE_HEADER);
	int secretB = PacketUtil.extractInt(resB, SIZE_HEADER + SIZE_INT);

	// step c
	Socket tcpSocket = new Socket(HOST, tcpPort);
	byte[] resC = stepC(secretB, tcpSocket);
	PacketUtil.printPacket(resC);
	int num2 = PacketUtil.extractInt(resC, SIZE_HEADER);
	int len2 = PacketUtil.extractInt(resC, SIZE_HEADER + SIZE_INT);
	int secretC = PacketUtil.extractInt(resC, SIZE_HEADER + SIZE_INT * 2);
	byte c = resC[SIZE_HEADER + SIZE_INT * 3];

	// step d	
	stepD(num2, len2, c, secretC, tcpSocket);
    }

    public static void stepD(int num, int len, byte c,
			     int secretC, Socket socket) throws IOException {
	System.out.println("num: " + num);
	System.out.println("len: " + len);
	System.out.println("c: " + c);

	byte[] payload = new byte[len];
	Arrays.fill(payload, c);

	// send 'num' payloads
	for (int i = 0; i < num; i++) {
	    byte[] buf = PacketUtil.packetize(payload, PacketUtil.toByteArray(secretC), "d1");
	    socket.getOutputStream().write(buf);
	}

	// read response
	ByteArrayOutputStream baos = new ByteArrayOutputStream();
	InputStream in = socket.getInputStream();
	byte[] buf = new byte[SIZE_HEADER + SIZE_INT];
        int s;
        int total = 0;
        while (total < 4 && (s = in.read(buf)) != -1) {
            total += s;
            baos.write(buf, 0, s);
        }
	PacketUtil.printPacket(buf);
	
	socket.close();
    }


    public static byte[] stepC(int secretB, Socket socket) throws IOException {
	InputStream in = socket.getInputStream();
	ByteArrayOutputStream baos = new ByteArrayOutputStream();

	byte[] buf = new byte[SIZE_HEADER + 4 * SIZE_INT];
	int s;
	int total = 0;
	while (total < 16 && (s = in.read(buf)) != -1) {
	    total += s;
	    baos.write(buf, 0, s);
	}

	return baos.toByteArray();
    }

    // runs stepA, returning the received packet's data
    public static byte[] stepA() throws IOException {
	System.out.println("Performing STEP A");
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
    
    public static byte[] stepB(int num, int len,
			       int udpPort, int secretA) throws IOException {
	System.out.println("Performing STEP B");
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
	    System.out.print("   sending " + i + " : ");
	    while (timeout) {
		socket.send(out);
		try {
		    socket.receive(in);
		    System.out.println("SUCCESS");
		    timeout = false;
		} catch (SocketTimeoutException e) {
		    timeout = true;
		    System.out.print(". ");
		}
	    }
	}
	byte[] bufIn = new byte[SIZE_HEADER + 2 * SIZE_INT];
	DatagramPacket in = new DatagramPacket(bufIn, bufIn.length);
	socket.receive(in);
	socket.close();
	return bufIn;
    }
}
