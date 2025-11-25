import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.AfterAll;
import static org.junit.jupiter.api.Assertions.*;

/*
  Test requesting multiple array slices from a 2D array.
*/
public class TestMultiSlice2D extends BasicUnitTest {

    /*
      Request the specified slices of a 2D array from a test data file.

      slices is the slice specifier to be passed to the web API
      shape is the shape of the array which we expect to be returned

      If the expected return status is not 200, then start and count
      are not used and can be null.
    */
    public void requestAndCheckSlices(String slices, long starts[], long counts[], int status, boolean post) throws Exception {

        // Request the slice(s)
        NDArray array = client.requestSlices("/tests/basic/test_data_2d.hdf5", "array2d", slices, status, post);

        // If that worked, check the result
        if(status == 200) {
            assertNotNull(array);

            // Starts and counts have one element per slice per dimension
            int nr_dims = 2;
            assertEquals(0, starts.length % nr_dims);
            int nr_slices = starts.length / nr_dims;
            assertEquals(starts.length, counts.length);

            // Compute expected shape of the result
            long shape[] = new long[2];
            // Slices are stacked along the first dimension
            for(int slice_nr=0; slice_nr<nr_slices; slice_nr+=1)
                shape[0] += counts[slice_nr*nr_dims+0];
            // Slices should all have the same shape in other dimensions
            for(int slice_nr=0; slice_nr<nr_slices; slice_nr+=1) {
                for(int dim_nr=1; dim_nr<nr_dims; dim_nr+=1) {
                    shape[dim_nr] = counts[slice_nr*nr_dims+dim_nr];
                    assertEquals(counts[0*nr_dims+dim_nr], shape[dim_nr]);
                }
            }

            // Check the array has the expected shape
            assertArrayEquals(shape, array.shape);

            // Now loop over slices
            for(int slice_nr=0; slice_nr<nr_slices; slice_nr+=1) {

                // Loop over input elements in this slice and compute corresponding output element coordinates
                long i_out = 0;
                for(long i_in=starts[slice_nr*nr_dims+0]; i_in<counts[slice_nr*nr_dims+0]; i_in +=1) {
                    long j_out = 0;
                    for(long j_in=starts[slice_nr*nr_dims+1]; j_in<counts[slice_nr*nr_dims+1]; j_in +=1) {

                        // Look up the value returned by the web service
                        long returned_value = array.getElementAsLong((int) (i_out*shape[1]+j_out));

                        // Compute the expected value in the input file:
                        // Each value is a simple function of its coordinates in the array.
                        long expected_value = 1000*i_in + j_in;

                        // Compare
                        assertEquals(returned_value, expected_value);
                        j_out += 1;
                    }
                    i_out += 1;
                }
            }
        }
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testReadAllOneSlice(boolean post) throws Exception {
        // Request the full 2D array as one large slice
        String slices = "0:100,0:200";
        long starts[] = {0, 0};
        long counts[] = {100, 200};
        requestAndCheckSlices(slices, starts, counts, 200, post);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testReadAllTwoSlices(boolean post) throws Exception {
        // Request the full 2D array as two stacked slices
        String slices = "0:50,0:200;50:100,0:200";
        long starts[] = {0,  0,   50, 0};
        long counts[] = {50, 200, 50, 200};
        requestAndCheckSlices(slices, starts, counts, 200, post);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testReadTwoSlicesSkipLeft(boolean post) throws Exception {
        // Miss out some elements along one side of the 2D array
        String slices = "0:50,30:200;50:100,30:200";
        long starts[] = {0,  30,  50, 30};
        long counts[] = {50, 170, 50, 170};
        requestAndCheckSlices(slices, starts, counts, 200, post);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testReadTwoSlicesSkipRight(boolean post) throws Exception {
        // Miss out some elements along the other side of the 2D array
        String slices = "0:50,0:170;50:100,0:170";
        long starts[] = {0,  0,   50, 0};
        long counts[] = {50, 170, 50, 170};
        requestAndCheckSlices(slices, starts, counts, 200, post);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testReadTwoSlicesWithGap(boolean post) throws Exception {
        // Skip some elements in the first dimension by shrinking the first slice
        String slices = "0:40,0:200;50:100,0:200";
        long starts[] = {0,  0,   50, 0};
        long counts[] = {40, 200, 50, 200};
        requestAndCheckSlices(slices, starts, counts, 200, post);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testReadPartialSlice(boolean post) throws Exception {
        // Request just part of the array
        String slices = "50:60,90:105";
        long starts[] = {50, 90};
        long counts[] = {10, 15};
        requestAndCheckSlices(slices, starts, counts, 200, post);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testReadPartialTwoSlices(boolean post) throws Exception {
        // Request just part of the array
        String slices = "50:55,90:105;55:60,90:105";
        long starts[] = {50, 90, 55, 90};
        long counts[] = {5,  15, 5,  15};
        requestAndCheckSlices(slices, starts, counts, 200, post);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testReadOutOfBoundsSlice(boolean post) throws Exception {
        // This slice exceeds the dataset bounds in one dimension
        String slices = "50:60,90:500";
        requestAndCheckSlices(slices, null, null, 404, post);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testReadInvalidSlice(boolean post) throws Exception {
        // This is not a valid slice specifier
        String slices = "abc";
        requestAndCheckSlices(slices, null, null, 400, post);
    }
}
