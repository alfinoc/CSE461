import java.io.*;
import java.net.*;
import java.util.*;

public class ServerThread extends Thread {
   public static final int SIZE_HEADER = 12;
   public static final int SIZE_INT = 4;
   public static final int MAX_PORT = 65535;

   private Random rand;
   protected DatagramSocket socket = null;
   protected int id;
   protected InetAddress clientAddress;
   protected int clientPort;

   // Forks a new thread based on the Step A initial client datagram 'initial'.
   // If the contents of 'initial' are in any way invalid according to the
   // protocol, an IllegalArgumentException is thrown. Otherwise, a new socket
   // is opened on an available port for subsequent transmission with client.
   public ServerThread(DatagramPacket initial) throws IOException {
      this("ServerThread");

      // validate client message for step a
      boolean validHeader = Validator.validHeader(12, 0, 1, initial.getData());
      String body = "";
      byte[] data = initial.getData();
      for (int i = PacketUtil.SIZE_HEADER; i < data.length && data[i] != '\0'; i++)
         body += (char) data[i];
      if (!validHeader || !body.equals("hello world"))
         throw new IllegalArgumentException();

      // extract id and client port, tie to this thread
      byte[] twoByteBuff = new byte[SIZE_INT];
      twoByteBuff[2] = initial.getData()[SIZE_INT * 2 + 2];
      twoByteBuff[3] = initial.getData()[SIZE_INT * 2 + 3];
      id = PacketUtil.extractInt(twoByteBuff, 0);
      clientPort = initial.getPort();
      clientAddress = initial.getAddress();

      // set up socket and Random for transmission later
      rand = new Random();
      while (socket == null) {
         try {
            // creates a new socket on whatever 
            socket = new DatagramSocket();
         } catch (Exception e) {
            System.out.println(e.getMessage());
         }
      }
   }

   // Forks a new ServerThread with 'name'.
   public ServerThread(String name) throws IOException {
      super(name);
   }

   // Performs each of the four steps to the protocol, based on step a parameters
   // provided by the initial packet used to fork this thread.
   public void run() {
      System.out.println("Starting a new thread!");
      System.out.println("   student id:  " + id);
      System.out.println("   host port:   " + socket.getLocalPort());
      System.out.println("   client port: " + clientPort);
      try {
         int secretA = stepA();
         stepB(secretA);
      } catch (IOException e) {
         System.err.println(e.getMessage());
      }
      System.out.println("Closing the thread on host port: " + socket.getLocalPort());
      socket.close();
   }

   // Performs step A, sending the server response with various parameters for
   // step B. Returns the randomly generated secretA.
   public int stepA() throws IOException {
      byte[] num = PacketUtil.toByteArray(rand.nextInt(100));
      byte[] len = PacketUtil.toByteArray(rand.nextInt(100));
      byte[] udpPort = PacketUtil.toByteArray(socket.getLocalPort());
      int firstSecret = rand.nextInt();
      byte[] secretA = PacketUtil.toByteArray(firstSecret);

      // combine fields into payload buffer
      byte[] payload = new byte[SIZE_INT * 4];
      for (int i = 0; i < num.length; i++)
         payload[i] = num[i];
      for (int i = 0; i < len.length; i++)
         payload[i + SIZE_INT] = len[i];
      for (int i = 0; i < udpPort.length; i++)
         payload[i + SIZE_INT * 2] = udpPort[i];
      for (int i = 0; i < secretA.length; i++)
         payload[i + SIZE_INT * 3] = secretA[i];
      
      // send response with parameters for step b
      byte[] packet = PacketUtil.packetize(payload, (new byte[SIZE_INT]), "a1");
      PacketUtil.printPacket(packet);
      DatagramPacket out = new DatagramPacket(packet, packet.length,
                                              clientAddress, clientPort);
      socket.send(out);
      return firstSecret;
   }

   public void stepB(int secretA) throws IOException {
      
   }

   // Closes the socket used by this thread if such a socket exists.
   public void close() {
      if (socket != null) socket.close();
   }
}