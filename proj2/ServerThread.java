import java.io.*;
import java.net.*;
import java.util.*;

public class ServerThread extends Thread {
   public static final int SIZE_HEADER = 12;
   public static final int SIZE_INT = 4;
   public static final int MAX_PORT = 65535;

   private Random rand;
   protected DatagramSocket socket = null;
   protected Socket tcp;
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
            // creates a new socket on some random port
            socket = new DatagramSocket();
         } catch (Exception e) {
            System.out.println(e.getMessage());
         }
      }
      socket.setSoTimeout(10000);
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
         byte[] sent = stepA();
         int secretB = stepB(PacketUtil.extractInt(sent, SIZE_HEADER),
                             PacketUtil.extractInt(sent, SIZE_HEADER + SIZE_INT),
                             PacketUtil.extractInt(sent, SIZE_HEADER + SIZE_INT * 3));
      } catch (IOException e) {
         System.err.println(e.getMessage());
      }
      System.out.println("Closing the thread on host port: " + socket.getLocalPort());
      socket.close();
   }

   // Performs step A, sending the server response with various parameters for
   // step B. Returns the randomly generated secretA.
   public byte[] stepA() throws IOException {
      byte[] num = PacketUtil.toByteArray(rand.nextInt(50));
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
      byte[] packet = PacketUtil.packetize(payload, (new byte[SIZE_INT]), "a1", id);
      DatagramPacket out = new DatagramPacket(packet, packet.length,
                                              clientAddress, clientPort);
      socket.send(out);
      return packet;
   }

   public int stepB(int num, int len, int secretA) throws IOException {
      int payloadSize = len + SIZE_INT;
      int packetSize = SIZE_HEADER + payloadSize;
      packetSize += (SIZE_INT - packetSize % SIZE_INT) % SIZE_INT;
      byte[] bufIn = new byte[packetSize + 1];
      Arrays.fill(bufIn, (byte) 0xF);

      boolean haveFailed = false;  // to ensure we fail at least once     
      for (int i = 0; i < num;) {
         System.out.print(".");
         DatagramPacket in = new DatagramPacket(bufIn, bufIn.length);
         socket.receive(in);
         clientAddress = in.getAddress();
         clientPort = in.getPort();

         // validate received packet
         if (!Validator.validHeader(payloadSize, secretA, 1, bufIn))
            throw new IllegalArgumentException();
         for (int j = SIZE_HEADER + SIZE_INT; j < bufIn.length - 1; j++)
            if (bufIn[j] != 0)
               throw new IllegalArgumentException();
         int seqNum = PacketUtil.extractInt(bufIn, SIZE_HEADER);
         if (seqNum != i)
            throw new IllegalStateException();

         // randomly fail to acknowledge some packets, ensuring failure on
         // the last packet if we have not yet failed
         if ((rand.nextDouble() < 0.2) || (i == num - 1 && !haveFailed)) {
            haveFailed = true;
         } else {
            byte[] payload = PacketUtil.toByteArray(i);
            byte[] bufOut = PacketUtil.packetize(payload,
                                    PacketUtil.toByteArray(secretA), "b1", id);
            DatagramPacket out = new DatagramPacket(bufOut, bufOut.length,
                                                    clientAddress, clientPort);
            socket.send(out);
            i++;
         }

         // reset buffer to catch improperly sized packets
         Arrays.fill(bufIn, (byte) 0xF);
      }
      System.out.println();

      // open the tcp socket


      // send final response
      byte[] payload = new byte[SIZE_INT * 2];
      int nextSecret = rand.nextInt();
      byte[] secretB = PacketUtil.toByteArray(nextSecret);
      byte[] tcpPort = PacketUtil.toByteArray(4);  //tcp.getLocalPort());
      for (int i = 0; i < tcpPort.length; i++)
         payload[i] = tcpPort[i];
      for (int i = 0; i < secretB.length; i++)
         payload[SIZE_INT + i] = secretB[i];
      byte[] bufOut = PacketUtil.packetize(payload, PacketUtil.toByteArray(secretA),
                                           "b2", id);
      DatagramPacket out = new DatagramPacket(bufOut, bufOut.length,
                                              clientAddress, clientPort);
      socket.send(out);
      return nextSecret;
   }

   // Closes the socket used by this thread if such a socket exists.
   public void close() {
      if (socket != null) socket.close();
   }
}