import java.net.*;
import java.util.*;
import java.io.*;

public class Server {
    public static void main(String[] args) throws IOException {
	if (args.length < 1) {
	    System.out.println("gimme a port");
	    return;
	}
	DatagramSocket socket = new DatagramSocket(Integer.parseInt(args[0]));
	byte[] buf = new byte[256];
	DatagramPacket packet = new DatagramPacket(buf, buf.length);
	
	Scanner console = new Scanner(System.in);
	while (!console.next().equals("exit")) {
	    socket.receive(packet);
	    System.out.println("data received: ");
	    System.out.println(Arrays.toString(packet.getData()));
	}
    }
}
