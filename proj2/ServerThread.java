/*
   Chris Alfino
   1024472
   CSE 461

   The runnable class handles one Server <-> Client interaction according to the
   project 2 specification. When created, a new thread is created and each of the
   four steps is performed. If the protocol is violated, an IllegalArgumentException or
   IllegalStateException is thrown and execution of this thread is terminated.
*/

import java.io.*;
import java.net.*;
import java.util.*;

public class ServerThread implements Runnable {
   public static final int SIZE_HEADER = 12;
   public static final int SIZE_INT = 4;
   public static final int MAX_PORT = 65535;

   // info about the client that this thread is communicating with
   protected InetAddress clientAddress;
   protected int clientPort;

   // all sockets used by this thread
   protected DatagramSocket udpSocket;
   protected ServerSocket tcpServer;
   protected Socket tcpSocket;

   private Random rand;
   protected TransmissionRecord record;
   public Thread thread;

   // Creates a new thread based on the Step A initial client datagram 'initial'.
   // If the contents of 'initial' are in any way invalid according to the
   // protocol, an IllegalArgumentException is thrown. Otherwise, a new socket
   // is opened on an available port for subsequent transmission with client.
   public ServerThread(DatagramPacket initial) throws IOException {
      record = new TransmissionRecord();

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
      record.id = PacketUtil.extractInt(twoByteBuff, 0);
      clientPort = initial.getPort();
      clientAddress = initial.getAddress();

      // set up socket and Random for transmission later
      rand = new Random();
      udpSocket = new DatagramSocket();
      udpSocket.setSoTimeout(10000);

      // create a new thread and start it up
      thread = new Thread(this, "student id: " + record.id);
      thread.start();
   }

   // Performs each of the four steps to the protocol, based on step a parameters
   // provided by the initial packet used to create this thread.
   public void run() {
      System.out.println("Starting thread with student id: " + record.id);
      try {

         stepA();
         stepB();
         udpSocket.close();

         stepC();
         stepD();
         tcpSocket.close();

         record.print();

      } catch (IOException e) {
         System.err.println(e.getMessage());
      }
      System.out.println("Closing the thread with student id: " + record.id);
   }

   // Performs step A, sending the server response with various parameters for
   // step B. Stores results in record.
   public void stepA() throws IOException {
      byte[] num = PacketUtil.toByteArray(record.num1 = rand.nextInt(50));
      byte[] len = PacketUtil.toByteArray(record.len1 = rand.nextInt(100));
      byte[] udpPort = PacketUtil.toByteArray(record.udpPort = udpSocket.getLocalPort());
      record.secretA = rand.nextInt();
      byte[] secretA = PacketUtil.toByteArray(record.secretA);

      byte[][] fields = {num, len, udpPort, secretA};
      byte[] payload = PacketUtil.adjoinByteArrays(fields);
      
      // send response with parameters for step b
      byte[] packet = PacketUtil.packetize(payload, (new byte[SIZE_INT]), "a1", record.id);
      DatagramPacket out = new DatagramPacket(packet, packet.length,
                                              clientAddress, clientPort);
      udpSocket.send(out);
   }

   // Performs step B, randomly failing to acknowledge some of the received packets
   // updSocket must be initialized. Stores results in record.
   public void stepB() throws IOException {
      int payloadSize = record.len1 + SIZE_INT;
      int packetSize = SIZE_HEADER + payloadSize;
      packetSize += (SIZE_INT - packetSize % SIZE_INT) % SIZE_INT;
      byte[] bufIn = new byte[packetSize];

      boolean haveFailed = false;  // to ensure we fail at least once     
      for (int i = 0; i < record.num1;) {
         DatagramPacket in = new DatagramPacket(bufIn, bufIn.length);
         udpSocket.receive(in);
         clientAddress = in.getAddress();
         clientPort = in.getPort();

         // validate received packet
         if (!Validator.validHeader(payloadSize, record.secretA, 1, bufIn))
            throw new IllegalArgumentException();
         for (int j = SIZE_HEADER + SIZE_INT; j < bufIn.length - 1; j++)
            if (bufIn[j] != 0)
               throw new IllegalArgumentException();
         int seqNum = PacketUtil.extractInt(bufIn, SIZE_HEADER);
         if (seqNum != i)
            throw new IllegalStateException();

         // randomly fail to acknowledge some packets, ensuring failure on
         // the last packet if we have not yet failed
         if ((rand.nextDouble() < 0.2) || (i == record.num1 - 1 && !haveFailed)) {
            haveFailed = true;
         } else {
            byte[] payload = PacketUtil.toByteArray(i);
            byte[] bufOut = PacketUtil.packetize(payload,
                                    PacketUtil.toByteArray(record.secretA), "b1", record.id);
            DatagramPacket out = new DatagramPacket(bufOut, bufOut.length,
                                                    clientAddress, clientPort);
            udpSocket.send(out);
            i++;
         }
      }

      initTCPServer();

      // send final response
      record.secretB = rand.nextInt();
      byte[] secretB = PacketUtil.toByteArray(record.secretB);
      byte[] tcpPort = PacketUtil.toByteArray(tcpServer.getLocalPort());

      byte[][] fields = {tcpPort, secretB};
      byte[] payload = PacketUtil.adjoinByteArrays(fields);
      byte[] bufOut = PacketUtil.packetize(payload, PacketUtil.toByteArray(record.secretA),
                                           "b2", record.id);
      DatagramPacket out = new DatagramPacket(bufOut, bufOut.length,
                                              clientAddress, clientPort);
      udpSocket.send(out);
   }

   // Initializes a tcp server on a random port, recording the appropriate fields of
   // the record if successful.
   private void initTCPServer() {
      boolean valid = false;
      while (!valid) {
         try {
            tcpServer = new ServerSocket(record.tcpPort = rand.nextInt(MAX_PORT - 2000) + 2000);
            valid = true;
         } catch (Exception e) {
            // keep trying as long as the randomly chosen port is not available
         }
      }
   }

   // Performs step C, sending a single TCP packet to the client containing parameters
   // for step D. Requires that the 'tcpServer' is initialized.
   public void stepC() throws IOException {
      tcpSocket = tcpServer.accept();
      tcpServer.close();
      tcpSocket.setSoTimeout(1000);
      byte[] num2 = PacketUtil.toByteArray(record.num2 = rand.nextInt(50));
      byte[] len2 = PacketUtil.toByteArray(record.len2 = rand.nextInt(50));
      byte[] secretC = PacketUtil.toByteArray(record.secretC = rand.nextInt());
      record.c = (char) (byte) rand.nextInt();

      byte[][] fields = {num2, len2, secretC, {(byte) record.c}};
      byte[] bufOut = PacketUtil.adjoinByteArrays(fields);
      bufOut = PacketUtil.packetize(bufOut, PacketUtil.toByteArray(record.secretB),
                                    "c2", record.id);
      tcpSocket.getOutputStream().write(bufOut);
   }

   // Performs step D, the reception of a random number of TCP payloads on the open
   // tcpSocket. Requires that this socket is connected.
   public void stepD() throws IOException {
      int paddedLength = record.len2;
      paddedLength += (SIZE_INT - paddedLength % SIZE_INT) % SIZE_INT;
      byte[] bufIn = new byte[SIZE_HEADER + paddedLength];
      for (int i = 0; i < record.num2; i++) {
         if (!PacketUtil.read(tcpSocket, bufIn))
            throw new IllegalArgumentException("failed on i = " + i);
         if (!Validator.validHeader(record.len2, record.secretC, 1, bufIn))
            throw new IllegalArgumentException();
         for (int j = 0; j < record.len2; j++)
            if (bufIn[SIZE_HEADER + j] != (byte) record.c)
               throw new IllegalArgumentException();
      }
      byte[] bufOut = PacketUtil.toByteArray(record.secretD = rand.nextInt());
      byte[] packet = PacketUtil.packetize(bufOut, PacketUtil.toByteArray(record.secretC),
                                           "d1", record.id);
      tcpSocket.getOutputStream().write(packet);
   }

   // A class for recording all of the fields associated with a full Server - Client
   // exchange according to the protocol. The fields are filled in as the program
   // executes, so if transmission terminates midway through, this record will be incomplete.
   private class TransmissionRecord {
      public int id;

      // step a - b random parameters
      public int num1;
      public int len1;
      public int udpPort;

      // step c - d random parameters
      public int num2;
      public int len2;
      public char c;
      public int tcpPort;

      // secrets
      public int secretA;
      public int secretB;
      public int secretC;
      public int secretD;

      // prints a report of the exchange
      public void print() {
         System.out.println("STEP A - B");
         System.out.println("   num1:    " + num1);
         System.out.println("   len1:    " + len1);
         System.out.println("   updPort: " + udpPort);
         System.out.println("STEP C - D");
         System.out.println("   num2:    " + num2);
         System.out.println("   len2:    " + len2);
         System.out.println("   c:     0d" + (int) c);
         System.out.println("   tcpPort: " + tcpPort);
         System.out.println("SECRETS:");
         System.out.println("   a:       " + secretA);
         System.out.println("   b:       " + secretB);
         System.out.println("   c:       " + secretC);
         System.out.println("   d:       " + secretD);
      }
   }
}