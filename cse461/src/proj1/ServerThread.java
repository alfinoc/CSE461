package proj1;
import java.io.*;
import java.net.*;

public class ServerThread extends Thread {
	public static final int PORT = 8000;
	
	protected DatagramSocket socket;
	private boolean listen;
	
	public ServerThread() throws IOException {
		this("SimpleServer");
	}

	public ServerThread(String name) throws IOException {
		super(name);
		socket = new DatagramSocket(PORT);
		listen = true;
	}
	
	// Waits for incoming datagrams, printing out basic information
	// about them when received.
	public void run() {
		while (listen) {
			try {
				byte[] buf = new byte[256];
				
				// receive request
				DatagramPacket packet = new DatagramPacket(buf, buf.length);
				socket.receive(packet);
				
				// print packet info
				System.out.println("NEW PACKET RECEIVED:");
				System.out.println("   address: " + packet.getAddress());
				System.out.println("   port:    " + packet.getPort());
				
				// print out packet body
				System.out.print("   message:");
				for (int i = 0; i < buf.length; i++)
					System.out.print((char) buf[i]);
				System.out.println();
			} catch (IOException e) {
				e.printStackTrace();
				listen = false;
			}
		}
		socket.close();
	}
	
	// stop listening and so stop running when interrupted
	public void stopListening() {
		listen = false;
	}
}
