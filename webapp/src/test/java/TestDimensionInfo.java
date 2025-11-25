import static org.junit.jupiter.api.Assertions.*;
import org.junit.jupiter.api.Test;
import java.io.IOException;

import org.msgpack.core.MessagePack;
import org.msgpack.core.MessagePacker;
import org.msgpack.core.MessageUnpacker;
import org.msgpack.core.MessageBufferPacker;

import uk.ac.dur.cosma.hdfstream.DimensionInfo;
import uk.ac.dur.cosma.hdfstream.InvalidSliceException;
import uk.ac.dur.cosma.hdfstream.MsgpackUtils;


public class TestDimensionInfo {

    @Test
    public void testTwoScalars() throws Exception {

        // Test the simple case where the slice is just [start,count] scalars.
        // Encode the slice info for one dimension.
        MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packArrayHeader(2);
        packer.packInt(0); // start=0
        packer.packInt(1); // count=1

        // Get the encoded msgpack data as a byte array
        packer.close();
        byte[] data = packer.toByteArray();

        // Unpack the data
        MessageUnpacker unpacker = MessagePack.newDefaultUnpacker(data);
        DimensionInfo dim = new DimensionInfo(unpacker);

        // Check the result
        assertEquals(1, dim.start.length);
        assertEquals(0, dim.start[0]);
        assertEquals(1, dim.count.length);
        assertEquals(1, dim.count[0]);
    }

    @Test
    public void testThreeScalars() throws Exception {

        // This array has the wrong length
        MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packArrayHeader(3);
        packer.packInt(0); // start=0
        packer.packInt(1); // count=1
        packer.packInt(2); // ???=2

        // Get the encoded msgpack data as a byte array
        packer.close();
        byte[] data = packer.toByteArray();

        // Unpack the data
        MessageUnpacker unpacker = MessagePack.newDefaultUnpacker(data);
        assertThrows(InvalidSliceException.class, () -> {
                DimensionInfo dim = new DimensionInfo(unpacker);
            });
    }

    @Test
    public void testString() throws Exception {

        // This isn't even an array!
        MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packString("xyz");

        // Get the encoded msgpack data as a byte array
        packer.close();
        byte[] data = packer.toByteArray();

        // Unpack the data
        MessageUnpacker unpacker = MessagePack.newDefaultUnpacker(data);
        assertThrows(InvalidSliceException.class, () -> {
                DimensionInfo dim = new DimensionInfo(unpacker);
            });
    }

    @Test
    public void testTwoArrays() throws Exception {

        // Test the case where start and count are arrays
        long start[] = {0, 1, 2, 3};
        long count[] = {4, 5, 6, 7};
        MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packArrayHeader(2);
        MsgpackUtils.packLongArray(packer, start);
        MsgpackUtils.packLongArray(packer, count);

        // Get the encoded msgpack data as a byte array
        packer.close();
        byte[] data = packer.toByteArray();

        // Unpack the data
        MessageUnpacker unpacker = MessagePack.newDefaultUnpacker(data);
        DimensionInfo dim = new DimensionInfo(unpacker);

        // Check the result
        assertEquals(4, dim.start.length);
        assertEquals(4, dim.count.length);
        for(int i=0; i<4; i+=1) {
            assertEquals(i, dim.start[i]);
            assertEquals(i+4, dim.count[i]);
        }
    }

    @Test
    public void testTwoDifferentSizedArrays() throws Exception {

        // Test the case where start and count are arrays
        long start[] = {0, 1, 2, 3};
        long count[] = {4, 5, 6, 7, 8};
        MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packArrayHeader(2);
        MsgpackUtils.packLongArray(packer, start);
        MsgpackUtils.packLongArray(packer, count);

        // Get the encoded msgpack data as a byte array
        packer.close();
        byte[] data = packer.toByteArray();

        // Unpack the data. Should fail.
        MessageUnpacker unpacker = MessagePack.newDefaultUnpacker(data);
        assertThrows(InvalidSliceException.class, () -> {
                DimensionInfo dim = new DimensionInfo(unpacker);
            });
    }

    @Test
    public void testArrayAndScalar() throws Exception {

        // Test the case where start is an array and count is a scalar
        long start[] = {0, 1, 2, 3};
        MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packArrayHeader(2);
        MsgpackUtils.packLongArray(packer, start);
        packer.packLong(10);

        // Get the encoded msgpack data as a byte array
        packer.close();
        byte[] data = packer.toByteArray();

        // Unpack the data
        MessageUnpacker unpacker = MessagePack.newDefaultUnpacker(data);
        DimensionInfo dim = new DimensionInfo(unpacker);

        // Check the result
        assertEquals(4, dim.start.length);
        for(int i=0; i<4; i+=1) {
            assertEquals(i, dim.start[i]);
        }
        assertEquals(1, dim.count.length);
        assertEquals(10, dim.count[0]);
    }

    @Test
    public void testScalarAndArray() throws Exception {

        // Test the case where start is a scalar and count is an array
        long count[] = {0, 1, 2, 3};
        MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packArrayHeader(2);
        packer.packLong(10);
        MsgpackUtils.packLongArray(packer, count);

        // Get the encoded msgpack data as a byte array
        packer.close();
        byte[] data = packer.toByteArray();

        // Unpack the data. Should fail.
        MessageUnpacker unpacker = MessagePack.newDefaultUnpacker(data);
        assertThrows(InvalidSliceException.class, () -> {
                DimensionInfo dim = new DimensionInfo(unpacker);
            });
    }

    @Test
    public void testScalarAndOneElementArray() throws Exception {

        // Test the case where we have a scalar and a on element array
        long count[] = {20};
        MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packArrayHeader(2);
        packer.packLong(10);
        MsgpackUtils.packLongArray(packer, count);

        // Get the encoded msgpack data as a byte array
        packer.close();
        byte[] data = packer.toByteArray();

        // Unpack the data
        MessageUnpacker unpacker = MessagePack.newDefaultUnpacker(data);
        DimensionInfo dim = new DimensionInfo(unpacker);

        // Check the result
        assertEquals(1, dim.start.length);
        assertEquals(10, dim.start[0]);
        assertEquals(1, dim.count.length);
        assertEquals(20, dim.count[0]);
    }
}


