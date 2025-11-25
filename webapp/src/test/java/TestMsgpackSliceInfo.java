import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import java.util.Arrays;

import org.msgpack.core.MessagePack;
import org.msgpack.core.MessagePacker;
import org.msgpack.core.MessageUnpacker;
import org.msgpack.core.MessageBufferPacker;

import uk.ac.dur.cosma.hdfstream.DimensionInfo;
import uk.ac.dur.cosma.hdfstream.InvalidSliceException;
import uk.ac.dur.cosma.hdfstream.MsgpackUtils;
import uk.ac.dur.cosma.hdfstream.SliceInfo;

// Check that we can decode msgpack slice parameters correctly
public class TestMsgpackSliceInfo {

    private static SliceInfo closeAndUnpack(MessageBufferPacker packer) throws Exception {

        // Get the encoded msgpack data as a byte array
        packer.close();
        byte[] data = packer.toByteArray();

        // Unpack the data
        MessageUnpacker unpacker = MessagePack.newDefaultUnpacker(data);
        return SliceInfo.fromUnpacker(unpacker);
    }

    @Test
    public void TestSingleSlice1D() throws Exception {

        // Simple case: a single slice of a 1D dataset
        MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packArrayHeader(1); // We have one dimension
        packer.packArrayHeader(2); // [start,count] array header
        packer.packInt(0); // start=0
        packer.packInt(1); // count=1

        // Decode the slice
        SliceInfo slice = closeAndUnpack(packer);

        // Check the result
        assertEquals(1, slice.nr_slices);
        assertEquals(1, slice.rank);
        assertEquals(0, slice.starts[0]);
        assertEquals(1, slice.counts[0]);
    }

    @Test
    public void TestSingleSlice1DWrongSize() throws Exception {

        // Invalid specifier with too many elements
        MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packArrayHeader(1); // We have one dimension
        packer.packArrayHeader(3); // [start,count,???] array header
        packer.packInt(0); // start=0
        packer.packInt(1); // count=1
        packer.packInt(2); // ???=2

        // Decode the slice
        assertThrows(InvalidSliceException.class, () -> {
                SliceInfo slice = closeAndUnpack(packer);
            });
    }

    @Test
    public void TestSingleSlice1DWrongType() throws Exception {

        // Invalid specifier with a float element
        MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packArrayHeader(1); // We have one dimension
        packer.packArrayHeader(2); // [start,count] array header
        packer.packInt(0); // start=0
        packer.packFloat(1.0f); // count=1

        // Decode the slice
        assertThrows(InvalidSliceException.class, () -> {
                SliceInfo slice = closeAndUnpack(packer);
            });
    }

    @Test
    public void TestMultiSlice1D() throws Exception {

        // Two slices of a 1D dataset
        MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packArrayHeader(1); // We have one dimension
        long start[] = {0, 5};
        long count[] = {3, 4};
        packer.packArrayHeader(2); // [start,count] array header
        MsgpackUtils.packLongArray(packer, start);
        MsgpackUtils.packLongArray(packer, count);

        // Decode the slice
        SliceInfo slice = closeAndUnpack(packer);

        // Check the result
        assertEquals(slice.nr_slices, 2);
        assertEquals(slice.rank, 1);
        assertArrayEquals(new long[] {0, 5}, slice.starts);
        assertArrayEquals(new long[] {3, 4}, slice.counts);
    }

    @Test
    public void TestMultiSlice1DScalarCount() throws Exception {

        // Two slices of a 1D dataset, with only one count specified.
        MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packArrayHeader(1); // We have one dimension
        long start[] = {0, 5};
        packer.packArrayHeader(2); // [start,count] array header
        MsgpackUtils.packLongArray(packer, start); // start array
        packer.packLong(10); // single count to be applied to all slices

        // Decode the slice
        SliceInfo slice = closeAndUnpack(packer);

        // Check the result
        assertEquals(slice.nr_slices, 2);
        assertEquals(slice.rank, 1);
        assertArrayEquals(new long[] {0, 5}, slice.starts);
        assertArrayEquals(new long[] {10, 10}, slice.counts);
    }

    @Test
    public void TestMultiSlice1DScalarStart() throws Exception {

        // Two slices of a 1D dataset, with only one start specified
        MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packArrayHeader(1); // We have one dimension
        long count[] = {5, 20};
        packer.packArrayHeader(2); // [start,count] array header
        packer.packLong(10); // invalid single start
        MsgpackUtils.packLongArray(packer, count); // count array

        // Decode the slice
        assertThrows(InvalidSliceException.class, () -> {
                SliceInfo slice = closeAndUnpack(packer);
            });
    }


    @Test
    public void TestSingleSlice2D() throws Exception {

        // A single slice of a 2D dataset using scalar starts and counts
        MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packArrayHeader(2);
        // Slice in the first dimension
        packer.packArrayHeader(2);
        packer.packInt(10); // start
        packer.packInt(20); // count
        // Slice in the second dimension
        packer.packArrayHeader(2);
        packer.packInt(0); // start
        packer.packInt(3); // count

        // Decode the slice
        SliceInfo slice = closeAndUnpack(packer);

        // Check the result
        assertEquals(1, slice.nr_slices);
        assertEquals(2, slice.rank);
        assertArrayEquals(new long[] {10, 0}, slice.starts);
        assertArrayEquals(new long[] {20, 3}, slice.counts);
    }

    @Test
    public void TestMultiSlice2D() throws Exception {

        // Two slices of a 2D dataset
        MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packArrayHeader(2);
        // Slice in the first dimension
        packer.packArrayHeader(2);
        MsgpackUtils.packLongArray(packer, new long[] {50, 100}); // start array
        MsgpackUtils.packLongArray(packer, new long[] {7,  14});  // count array
        // Slice in the second dimension
        packer.packArrayHeader(2);
        packer.packInt(1); // start
        packer.packInt(3); // count

        // Decode the slice
        SliceInfo slice = closeAndUnpack(packer);

        // Check the result
        assertEquals(2, slice.nr_slices);
        assertEquals(2, slice.rank);
        assertArrayEquals(new long[] {50, 1, 100, 1}, slice.starts);
        assertArrayEquals(new long[] {7,  3, 14,  3}, slice.counts);
    }

    @Test
    public void TestMultiSlice2DScalarCount() throws Exception {

        // Two slices of a 2D dataset with array starts and all scalar counts
        MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packArrayHeader(2);
        // Slice in the first dimension
        packer.packArrayHeader(2);
        MsgpackUtils.packLongArray(packer, new long[] {50, 100}); // start array
        packer.packInt(1); // count scalar
        // Slice in the second dimension
        packer.packArrayHeader(2);
        packer.packInt(2); // start
        packer.packInt(3); // count

        // Decode the slice
        SliceInfo slice = closeAndUnpack(packer);

        // Check the result
        assertEquals(2, slice.nr_slices);
        assertEquals(2, slice.rank);
        assertArrayEquals(new long[] {50, 2, 100, 2}, slice.starts);
        assertArrayEquals(new long[] {1,  3, 1,   3}, slice.counts);
    }

    @Test
    public void TestMultiSlice2DAllArrays() throws Exception {

        // Two slices of a 2D dataset with array starts and counts in all dims
        // (not valid - fancy indexing is only allowed in the first dimension)
        MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packArrayHeader(2);
        // Slice in the first dimension
        packer.packArrayHeader(2);
        MsgpackUtils.packLongArray(packer, new long[] {50, 100}); // start array
        MsgpackUtils.packLongArray(packer, new long[] {7,  14});  // count array
        // Slice in the second dimension
        packer.packArrayHeader(2);
        MsgpackUtils.packLongArray(packer, new long[] {0,  0}); // start array
        MsgpackUtils.packLongArray(packer, new long[] {3,  3});  // count array

        // Decode the slice
        assertThrows(InvalidSliceException.class, () -> {
                SliceInfo slice = closeAndUnpack(packer);
            });
    }
}
