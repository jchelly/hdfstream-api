import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.AfterAll;
import static org.junit.jupiter.api.Assertions.*;

/*
  Test requesting an array slice from the server.
*/
public class TestSingleSlice extends BasicUnitTest {

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testFullSlice1D(boolean post) throws Exception {

        // Send a request for the full contents of a test dataset by specifying all elements
        NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", "0:10", 200, post);

        // Check the response is what we expected:
        assertNotEquals(null, array);
        assertEquals(1, array.shape.length); // Array is 1D
        assertEquals(10, array.shape[0]);   // Array has 10 elements
        for(int i=0; i<array.shape[0]; i+=1)
            assertEquals(i, array.getElementAsLong(i)); // Value equals index
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testMissingObject(boolean post) throws Exception {

        // Send a request for the full contents of a test dataset by specifying all elements,
        // but forgetting to specify a dataset name.
        NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", null, "0:10", 400, post);
        assertEquals(null, array); // Should throw an IOException before we get here
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testInvalidObject(boolean post) throws Exception {

        // Send a request for the full contents of a test dataset by specifying all elements,
        // but with an incorrect dataset name.
        NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "NOT-FOUND", "0:10", 404, post);
        assertEquals(null, array);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testInvalidPath(boolean post) throws Exception {

        // Send a request for the full contents of a test dataset by specifying all elements,
        // but with an incorrect file name.
        NDArray array = client.requestSlices("/tests/basic/test_data.hdf6", "data", "0:10", 404, post);
        assertEquals(null, array);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testPartialSlice1D(boolean post) throws Exception {

        // Send a request for partial contents of a test dataset
        NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", "3:8", 200, post);

        // Check the response is what we expected:
        assertNotEquals(null, array);
        assertEquals(1, array.shape.length); // Array is 1D
        assertEquals(5, array.shape[0]);   // Array has 5 elements
        for(int i=0; i<array.shape[0]; i+=1)
            assertEquals(i+3, array.getElementAsLong(i)); // Value equals index
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testSingleElementSlice1D(boolean post) throws Exception {

        // Send a request for partial contents of a test dataset
        NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", "5:6", 200, post);

        // Check the response is what we expected:
        assertNotEquals(null, array);
        assertEquals(1, array.shape.length); // Array is 1D
        assertEquals(1, array.shape[0]);   // Array has 1 elements
        assertEquals(5, array.getElementAsLong(0));
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testSingleElementIndex1D(boolean post) throws Exception {

        // Request a single element with just an index - no range
        NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", "5", 200, post);

        // Check the response is what we expected:
        assertNotEquals(null, array);
        assertEquals(1, array.shape.length); // Array is 1D
        assertEquals(1, array.shape[0]);   // Array has 1 elements
        assertEquals(5, array.getElementAsLong(0));
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testZeroSizeSlice1D(boolean post) throws Exception {

        // Send a request for partial contents of a test dataset
        NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", "0:0", 200, post);

        // Check the response is what we expected:
        assertNotEquals(null, array);
        assertEquals(1, array.shape.length); // Array is 1D
        assertEquals(0, array.shape[0]);   // Array has 0 elements
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testZeroSizeAtEndSlice1D(boolean post) throws Exception {

        // Send a request for partial contents of a test dataset:
        // This is a zero size slice at the end of the array.
        // The way bounds checking is done means this is accepted, but a
        // zero sized slice further out of bounds is not.
        NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", "10:10", 200, post);

        // Check the response is what we expected:
        assertNotEquals(null, array);
        assertEquals(1, array.shape.length); // Array is 1D
        assertEquals(0, array.shape[0]);   // Array has 0 elements
    }

    /*
      Test some invalid requests

      Requests with improperly formatted parameters return a 400 bad request status
      Requests which specify a non-existent dataset or out of range elements return 404 not found

      Possibly out of range slices or the wrong number of dimensions should return 400 instead?
    */
    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testFullSlice1DOutOfBounds1(boolean post) throws Exception {

        // Send an out of range request
        NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", "0:11", 404, post);
        assertEquals(null, array);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testFullSlice1DOutOfBounds2(boolean post) throws Exception {

        // Send an out of range request
        NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", "10:11", 404, post);
        assertEquals(null, array);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testFullSlice1DOutOfBounds3(boolean post) throws Exception {

        // Send an out of range request
        NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", "5:15", 404, post);
        assertEquals(null, array);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testZeroSizeSlice1DOutOfBounds(boolean post) throws Exception {

        // Send an out of range request for a zero size slice
        NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", "11:11", 404, post);
        assertEquals(null, array);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testFullSliceBadSlice1(boolean post) throws Exception {

        // Send a request for the full contents of a test dataset by specifying all elements
        // Note that we don't support python style omission of the start or end index.
        NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", "0:", 400, post);
        assertEquals(null, array);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testFullSliceBadSlice2(boolean post) throws Exception {

        // Send a request for the full contents of a test dataset by specifying all elements
        // Note that we don't support python style omission of the start or end index.
        NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", ":10", 400, post);
        assertEquals(null, array);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testFullSliceBadSlice3(boolean post) throws Exception {

        // Send a request for the full contents of a test dataset by specifying all elements
        // Note that we don't support python style omission of the start or end index.
        NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", ":", 400, post);
        assertEquals(null, array);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testFullSliceBadSlice4(boolean post) throws Exception {

        // Send a request for the full contents of a test dataset by specifying all elements
        // Note that we don't support python style ellipsis.
        NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", "...", 400, post);
        assertEquals(null, array);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true}) 
    public void testFullSliceTooManyDims(boolean post) throws Exception {

        // Send a request for the full contents of a test dataset by specifying all elements
        // In this case we ask for a 2D slice of a 1D array, which should fail.
        NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", "0:10,0:3", 404, post);
        assertEquals(null, array); // Should throw an IOException before we get here
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testSliceNegativeStart(boolean post) throws Exception {

        NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", "-1:10", 400, post);
        assertEquals(null, array);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testSliceNegativeStop(boolean post) throws Exception {

        NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", "0:-1", 400, post);
        assertEquals(null, array);
    }

}
