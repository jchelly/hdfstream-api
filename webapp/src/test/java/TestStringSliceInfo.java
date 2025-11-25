import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import java.util.Arrays;

import uk.ac.dur.cosma.hdfstream.SliceInfo;
import uk.ac.dur.cosma.hdfstream.InvalidSliceException;

// Check that we can decode slice parameter strings correctly
public class TestStringSliceInfo {

    @Test
    public void Test1() throws Exception {
        // A scalar (i.e. 0-d) "slice"
        SliceInfo slice = new SliceInfo("");
        assertEquals(1, slice.nr_slices);
        assertEquals(0, slice.rank);
        assertEquals(0, slice.starts.length);
        assertEquals(0, slice.counts.length);
    }

    @Test
    public void Test2() throws Exception {
        // Multiple scalar slices. Not useful, but should be parsable.
        SliceInfo slice = new SliceInfo(";;");
        assertEquals(3, slice.nr_slices);
        assertEquals(0, slice.rank);
        assertEquals(0, slice.starts.length);
        assertEquals(0, slice.counts.length);
    }

    @Test
    public void Test3() throws Exception {
        // Simple 1D slice
        SliceInfo slice = new SliceInfo("0:10");
        long expected_starts[] = {0};
        long expected_counts[] = {10};
        assertEquals(1, slice.nr_slices);
        assertEquals(1, slice.rank);
        assertArrayEquals(expected_starts, slice.starts);
        assertArrayEquals(expected_counts, slice.counts);
    }

    @Test
    public void Test4() throws Exception {
        // Simple 1D slice with an offset
        SliceInfo slice = new SliceInfo("100:120");
        long expected_starts[] = {100};
        long expected_counts[] = {20};
        assertEquals(1, slice.nr_slices);
        assertEquals(1, slice.rank);
        assertArrayEquals(expected_starts, slice.starts);
        assertArrayEquals(expected_counts, slice.counts);
    }

    @Test
    public void Test5() throws Exception {
        // Two 1D slices
        SliceInfo slice = new SliceInfo("0:10;50:70");
        long expected_starts[] = {0, 50};
        long expected_counts[] = {10, 20};
        assertEquals(2, slice.nr_slices);
        assertEquals(1, slice.rank);
        assertArrayEquals(expected_starts, slice.starts);
        assertArrayEquals(expected_counts, slice.counts);
    }

    @Test
    public void Test6() throws Exception {
        // A 2D slice
        SliceInfo slice = new SliceInfo("100:200,0:3");
        long expected_starts[] = {100, 0};
        long expected_counts[] = {100, 3};
        assertEquals(1, slice.nr_slices);
        assertEquals(2, slice.rank);
        assertArrayEquals(expected_starts, slice.starts);
        assertArrayEquals(expected_counts, slice.counts);
    }

    @Test
    public void Test7() throws Exception {
        // Two 2D slices
        SliceInfo slice = new SliceInfo("100:200,0:3;1000:1150,0:3");
        long expected_starts[] = {100, 0, 1000, 0};
        long expected_counts[] = {100, 3, 150, 3};
        assertEquals(2, slice.nr_slices);
        assertEquals(2, slice.rank);
        assertArrayEquals(expected_starts, slice.starts);
        assertArrayEquals(expected_counts, slice.counts);
    }

    @Test
    public void Test8() throws Exception {
        // Two 2D slices with some extraneous whitespace (should still work)
        SliceInfo slice = new SliceInfo("100 :   200 , 0:    3;1000: 1150   ,   0 : 3");
        long expected_starts[] = {100, 0, 1000, 0};
        long expected_counts[] = {100, 3, 150, 3};
        assertEquals(2, slice.nr_slices);
        assertEquals(2, slice.rank);
        assertArrayEquals(expected_starts, slice.starts);
        assertArrayEquals(expected_counts, slice.counts);
    }

    @Test
    public void Test9() throws Exception {
        // Should fail if slices don't all have the same rank
        try {
            SliceInfo slice = new SliceInfo("100:200,0:3;1000:1150");
            fail("Expected InvalidSliceException");
        } catch (InvalidSliceException e) {}
    }

    @Test
    public void Test10() throws Exception {
        // Negative indexes should not be accepted
        try {
            SliceInfo slice = new SliceInfo("-10:10");
            fail("Expected InvalidSliceException");
        } catch (InvalidSliceException e) {}
        try {
            SliceInfo slice = new SliceInfo("-20:-10");
            fail("Expected InvalidSliceException");
        } catch (InvalidSliceException e) {}
        // Negative length slices are also not allowed (zero length is ok)
        try {
            SliceInfo slice = new SliceInfo("10:9");
            fail("Expected InvalidSliceException");
        } catch (InvalidSliceException e) {}
    }
}
