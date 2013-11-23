/*
   Chris Alfino
   1024472
   CSE 461

   This class contains the main executable for the server. Once run, it listens for
   incoming UDP packets that fit the specification for Step A1, and creates new threads
   to handle the packets that do.
*/

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
      while (true) {
         System.out.println("Server accepting Step A1 packets.");
         try {
            // receive packets on Step A's port, forking a new thread if
            // we receive a new packet on that socket.
            byte[] bufIn = new byte[SIZE_HEADER + "hello world ".length()];
            DatagramPacket in = new DatagramPacket(bufIn, bufIn.length);
            socket.receive(in);

            try {
               new ServerThread(in);
            } catch (IllegalArgumentException e) {
               // silently refuse to respond to malformed packet
            }
         } catch (Exception e) {
            System.err.println("Exception. Terminating reception.");
            System.err.println(e.getMessage());
            e.printStackTrace();
         }
      }
   }
}