import java.io.*;
import java.net.*;
import java.util.*;

public class Server {
	public static final int PORT = 12235;
	public static final int SIZE_HEADER = 12;
    public static final int SIZE_INT = 4;

	public static void main(String[] args) throws IOException {

		Scanner console = new Scanner(System.in);
		DatagramSocket socket = new DatagramSocket(PORT);
		do {
			try {
				// receive packets on Step A's port, forking a new thread if
				// we receive a new packet on that socket.
				byte[] bufIn = new byte[SIZE_HEADER + "hello world ".length() + 1];
				Arrays.fill(bufIn, (byte) 0xF);
	       		DatagramPacket in = new DatagramPacket(bufIn, bufIn.length);
				socket.receive(in);

				// fork a new server thread
				try {
					ServerThread newThread = new ServerThread(in);
					newThread.run();
				} catch (IllegalArgumentException e) {
					// silently refuse to respond to malformed packet
					System.out.println("   failed step a1");
				}
				System.out.println("Enter \"exit\" to gently kill server.");
				System.out.println("Enter anything else to continue.");
			} catch (Exception e) {
				System.err.println("Exception. Terminating reception.");
				System.err.println(e.getMessage());
			}
		} while (!console.next().equalsIgnoreCase("exit"));
		socket.close();
	}
}