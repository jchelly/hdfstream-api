import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.AfterAll;
import static org.junit.jupiter.api.Assertions.*;

/*
  Test requesting multiple array slices from the server.
*/
public class TestMultiSlice extends BasicUnitTest {

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testTwoSlices(boolean post) throws Exception {

        // Send requests for the full dataset contents in two slices in various
        // valid (but maybe not sensible!) ways
        String[] slices = {"0:10", "0:5;5:10", "0:0;0:10", "0:9;9:10", "0:10;10:10", "0;1:10", "0:9;9"};
        for(String slice : slices) {
            // Request the data
            NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", slice, 200, post);
            // Check the response is what we expected:
            assertNotEquals(null, array);
            assertEquals(1, array.shape.length); // Array is 1D
            assertEquals(10, array.shape[0]);   // Array has 10 elements
            for(int i=0; i<array.shape[0]; i+=1)
                assertEquals(i, array.getElementAsLong(i)); // Value equals index
        }
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testTwoPartialSlices(boolean post) throws Exception {

        // Send requests for part of the dataset contents in two slices in various
        // valid (but maybe not sensible!) ways
        String[] slices = {"3:9", "3:6;6:9", "3:3;3:9", "3:8;8:9", "3:9;9:9", "3;4:9", "3:8;8"};
        for(String slice : slices) {
            // Request the data
            NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", slice, 200, post);
            // Check the response is what we expected:
            assertNotEquals(null, array);
            assertEquals(1, array.shape.length); // Array is 1D
            assertEquals(6, array.shape[0]);   // Array has 6 elements
            for(int i=0; i<array.shape[0]; i+=1)
                assertEquals(3+i, array.getElementAsLong(i)); // Value equals index
        }
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testPartialIndexes(boolean post) throws Exception {

        // Test sending individual indexes instead of ranges
        String[] slices = {"3;4;5;6;7;8"};
        for(String slice : slices) {
            // Request the data
            NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", slice, 200, post);
            // Check the response is what we expected:
            assertNotEquals(null, array);
            assertEquals(1, array.shape.length); // Array is 1D
            assertEquals(6, array.shape[0]);   // Array has 6 elements
            for(int i=0; i<array.shape[0]; i+=1)
                assertEquals(3+i, array.getElementAsLong(i)); // Value equals index
        }
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testPartialIndexesWrongOrder(boolean post) throws Exception {

        // Test sending individual indexes instead of ranges
        String[] slices = {"3;4;6;5;7;8"};
        for(String slice : slices) {
            // Request the data
            NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", slice, 404, post);
            // Check the response is what we expected:
            assertEquals(null, array);
        }
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testPartialIndexesDuplicate(boolean post) throws Exception {

        // Test sending individual indexes instead of ranges
        String[] slices = {"3;4;5;5;7;8"};
        for(String slice : slices) {
            // Request the data
            NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", slice, 404, post);
            // Check the response is what we expected:
            assertEquals(null, array);
        }
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testInvalidSlices1(boolean post) throws Exception {

        // Send invalid slice specifiers - should all get a 400 bad request back
        String[] slices = {"a:b;c:d", ":;:", " : ; : ", "abc:def", "0:5;5:", ":5;5:10", ";0:10", "0:10;"};
        for(String slice : slices) {
            // Request the data
            NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", slice, 400, post);
            // Check the response is what we expected:
            assertEquals(null, array);
        }
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testInvalidSlices2(boolean post) throws Exception {

        // These are valid slice specifiers but not valid for this dataset, so we get a 404
        // An empty string is a valid slice of a scalar dataset but this dataset is 1D.
        String[] slices = {"", ";", ";;", "  ;  ; ; "};
        for(String slice : slices) {
            // Request the data
            NDArray array = client.requestSlices("/tests/basic/test_data.hdf5", "data", slice, 404, post);
            // Check the response is what we expected:
            assertEquals(null, array);
        }
    }
}
