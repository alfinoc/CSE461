import java.io.*;
import java.net.*;
import java.util.*;

public class Validator {
    public static final int SIZE_HEADER = 12;
    public static final int SIZE_INT = 4;

	// better be 12 bytes
	// payloadLen better fit received inside 12 + receivedLength
	// payloadLen, pSecret, step all have to match
	// id bytes must be between 0 - 999
	// pre: received has length % 4 == 1 and has a 0xF in final position
	// client can still imitate this pattern, but there's no way for me to find
	// out how many bytes i actually received
	public static boolean validHeader(int payloadLen, int pSecret, int step,
									  byte[] received) {
		if (received.length < SIZE_HEADER || received.length % 4 != 1
				|| 0xF != received[received.length - 1])
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

		twoByteBuff[2] = received[SIZE_INT * 2 + 2];
		twoByteBuff[3] = received[SIZE_INT * 2 + 3];
		int id = PacketUtil.extractInt(twoByteBuff, 0);
		boolean passId = 0 <= id && id <= 999;

		System.out.println("header pass: " + (passLen && passSecret && passStep && passId));
		return passLen && passSecret && passStep && passId;
	}
}