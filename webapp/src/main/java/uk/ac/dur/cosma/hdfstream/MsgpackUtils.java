package uk.ac.dur.cosma.hdfstream;

import org.msgpack.core.MessagePacker;
import org.msgpack.core.MessageUnpacker;
import java.nio.charset.StandardCharsets;
import java.io.IOException;
import org.msgpack.core.MessageUnpacker;
import org.msgpack.core.MessageFormat;
import org.msgpack.value.ValueType;

public class MsgpackUtils {

    public static String unpackLimitedString(MessageUnpacker unpacker, int maxBytes) throws IOException {
        int byteLength = unpacker.unpackRawStringHeader();
        if (byteLength > maxBytes) {
            throw new IOException("String exceeds allowed size: " + byteLength + " bytes");
        }
        byte[] rawBytes = unpacker.readPayload(byteLength);
        return new String(rawBytes, StandardCharsets.UTF_8);
    }

    // Get the type of the next value in a msgpack stream
    public static ValueType getNextType(MessageUnpacker unpacker) throws IOException {
        MessageFormat format = unpacker.getNextFormat();
        return format.getValueType();
    }

    // Unpack an array of msgpack integers to a java array of longs
    public static long[] unpackLongArray(MessageUnpacker unpacker, int max_length) throws IOException {
        int n = unpacker.unpackArrayHeader();
        if(n > max_length)throw new IOException("Array is too large!");
        long[] array = new long[n];
        for(int i=0; i<n; i+=1) {
            array[i] = unpacker.unpackLong();
        }
        return array;
    }

    // Unpack the next integer or array of integers and return an array of longs.
    // Scalars are returned as an array of one element.
    public static long[] unpackLongArrayOrScalar(MessageUnpacker unpacker, int max_length) throws IOException {
        if(getNextType(unpacker)==ValueType.INTEGER) {
            // It's a scalar
            long[] array = new long[1];
            array[0] = unpacker.unpackLong();
            return array;
        } else {
            // It's an array (presumably - will throw an exception if not)
            return unpackLongArray(unpacker, max_length);
        }
    }

    // Pack an array of longs as an array of msgpack integers
    public static void packLongArray(MessagePacker packer, long array[]) throws IOException {
        packer.packArrayHeader(array.length);
        for(int i=0; i<array.length; i+=1) {
            packer.packLong(array[i]);
        }
        return;
    }
}
