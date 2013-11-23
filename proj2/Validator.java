/*
   Chris Alfino
   1024472
   CSE 461

   This class contains one Validator method to validate packet headers according
   to the protocol specification.
*/

import java.io.*;
import java.net.*;
import java.util.*;

public class Validator {
   public static final int SIZE_HEADER = 12;
   public static final int SIZE_INT = 4;

   // Returns true if the follow criteria are met by 'received' packet,
   // false otherwise:
   //    - packet is at least 12 bytes
   //    - 'payloadLen' will fit in the packet with the header
   //    - 'payloadLen', 'pSecret', and 'step' all match fields in the header
   //    - integer in 'id' position of header are in appropriate 0 - 999 range
   public static boolean validHeader(int payloadLen, int pSecret, int step,
                                     byte[] received) {
      if (received.length < SIZE_HEADER)
         return false;
      
      // validate length
      boolean passLen = payloadLen == PacketUtil.extractInt(received, 0);
      passLen = passLen && received.length >= SIZE_HEADER + payloadLen;

      // validate secret
      boolean passSecret = pSecret == PacketUtil.extractInt(received, SIZE_INT);

      // validate step
      byte[] twoByteBuff = new byte[SIZE_INT];
      twoByteBuff[2] = received[SIZE_INT * 2];
      twoByteBuff[3] = received[SIZE_INT * 2 + 1];
      boolean passStep = step == PacketUtil.extractInt(twoByteBuff, 0);

      // validate id
      twoByteBuff[2] = received[SIZE_INT * 2 + 2];
      twoByteBuff[3] = received[SIZE_INT * 2 + 3];
      int id = PacketUtil.extractInt(twoByteBuff, 0);
      boolean passId = 0 <= id && id <= 999;

      return passLen && passSecret && passStep && passId;
   }
}
