package proj1;

import java.io.*;
import java.net.*;
import java.util.Scanner;

public class Client {
	public static final String HOST = "bicycle.cs.washington.edu";
	public static final int PORT = 12235;
	
	public static void main(String[] args) throws IOException {
		// set up server thread to receive responses
		ServerThread server = new ServerThread();
		server.start();
		
		DatagramSocket socket = new DatagramSocket();
		byte[] buf = new byte[256];
		Scanner console = new Scanner(System.in);
		String in;
		
		System.out.println("Type your message:");
		while (!(in = console.nextLine()).equalsIgnoreCase("exit")) {
			System.out.println("Type your message:");
			fillByteArray(buf, in);
			InetAddress address = InetAddress.getByName(HOST);
			DatagramPacket packet = new DatagramPacket(buf, buf.length, address, PORT);
			socket.send(packet);
			System.out.println("Sending a message to" + HOST + ":" + PORT + "...");
		}
		server.stopListening();
		socket.close();
		console.close();
	}
	
	// returns true if all of 's' fit in 'bytes', false otherwise
	// fills 'bytes' with as many of the characters in s as possible,
	// overwriting previous contents and zeroing out unused space in 'bytes'
	public static boolean fillByteArray(byte[] bytes, String s) {
		// fill buffer as much as possible
		for (int i = 0; i < Math.min(bytes.length, s.length()); i++)
			bytes[i] = (byte) s.charAt(i);
		
		// null unused part of buffer
		for (int i = s.length(); i < bytes.length; i++)
			bytes[i] = 0;
		return s.length() <= bytes.length;
	}
	
	public static byte[] packetize(byte[] payload, byte[] prevSecret) {
		int length = 12 + payload.length;
		byte[] packet = new byte[length  - length / 4 + 4];
		
		
		return packet;
	}
	
	// inserts a 12 byte header at the beginning of 'packet'
	// throws IllegalArgumentException if header will not fit
	public static void insertHeader(byte[] packet, int trueLength, 
											  int step, byte[] prevSecret) {
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
		packet[8] = (byte) step;
		packet[9] = (byte) step;
		
		// 2-byte student number;
		packet[10] = (byte) 0x1;
		packet[11] = (byte) 0xD8;
	}
	
	public static byte[] toByteArray(int n) {
		byte[] res = new byte[4];
		for (int i = 0; i < 4; i++) {
			res[4 - i] = (byte) n;
			n = n >> 8;
		}
		return res;
	}
}
