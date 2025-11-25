import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.AfterAll;
import static org.junit.jupiter.api.Assertions.*;

/*
  Test requesting a HDF5 group from the server.
*/
public class TestDataset extends BasicUnitTest {

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testDatasetDefault(boolean post) throws Exception {

        // Request a dataset without specifying size limit
        // Default is 2GB so we should get the full contents back.
        HDF5Object dataset = client.requestObject("/tests/basic/test_data.hdf5", "/data", null, null, 200, post);

        // The dataset data should be a 1D array with 10 elements
        assertNotEquals(null, dataset);
        assertEquals(1, dataset.shape.length);
        assertEquals(10, dataset.shape[0]);

        // We should have loaded the dataset contents
        NDArray array = dataset.data;
        assertNotEquals(null, array);
        assertEquals(1, array.shape.length);
        assertEquals(10, array.shape[0]);
        for(int i=0; i<10; i+=1)
            assertEquals(i, array.getElementAsLong(i));
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testDatasetLargeSizeLimit(boolean post) throws Exception {

        // Request a dataset with a large size limit so we download the full contents
        String data_size_limit = "1000000"; // Larger than the dataset (in bytes)
        HDF5Object dataset = client.requestObject("/tests/basic/test_data.hdf5", "/data", null, data_size_limit, 200, post);

        // The dataset data should be a 1D array with 10 elements
        assertNotEquals(null, dataset);
        assertEquals(1, dataset.shape.length);
        assertEquals(10, dataset.shape[0]);

        // We should have loaded the dataset contents
        NDArray array = dataset.data;
        assertNotEquals(null, array);
        assertEquals(1, array.shape.length);
        assertEquals(10, array.shape[0]);
        for(int i=0; i<10; i+=1)
            assertEquals(i, array.getElementAsLong(i));
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testDatasetSmallSizeLimit(boolean post) throws Exception {

        // Request a dataset with a size limit just a bit smaller than the dataset
        String data_size_limit = "70"; // Dataset contains 10 8 byte integers
        HDF5Object dataset = client.requestObject("/tests/basic/test_data.hdf5", "/data", null, data_size_limit, 200, post);

        // The dataset data should be a 1D array with 10 elements
        assertNotEquals(null, dataset);
        assertEquals(1, dataset.shape.length);
        assertEquals(10, dataset.shape[0]);

        // We should not have loaded the dataset contents
        assertEquals(null, dataset.data);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testDatasetZeroSizeLimit(boolean post) throws Exception {

        // Request a dataset with a zero size limit so we just get metadata
        String data_size_limit = "0";
        HDF5Object dataset = client.requestObject("/tests/basic/test_data.hdf5", "/data", null, data_size_limit, 200, post);

        // The dataset data should be a 1D array with 10 elements
        assertNotEquals(null, dataset);
        assertEquals(1, dataset.shape.length);
        assertEquals(10, dataset.shape[0]);

        // We should not have loaded the dataset contents
        assertEquals(null, dataset.data);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false})
    public void testDatasetInvalidSizeLimit(boolean post) throws Exception {

        String data_size_limit = "abc";
        HDF5Object dataset = client.requestObject("/tests/basic/test_data.hdf5", "/data", null, data_size_limit, 400, post);
        assertEquals(null, dataset);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false})
    public void testDatasetEmptySizeLimit(boolean post) throws Exception {

        String data_size_limit = "";
        HDF5Object dataset = client.requestObject("/tests/basic/test_data.hdf5", "/data", null, data_size_limit, 400, post);
        assertEquals(null, dataset);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testDatasetNegativeSizeLimit(boolean post) throws Exception {

        String data_size_limit = "-1";
        HDF5Object dataset = client.requestObject("/tests/basic/test_data.hdf5", "/data", null, data_size_limit, 400, post);
        assertEquals(null, dataset);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false})
    public void testDatasetFloatSizeLimit(boolean post) throws Exception {

        String data_size_limit = "100.0"; // This should not be accepted
        HDF5Object dataset = client.requestObject("/tests/basic/test_data.hdf5", "/data", null, data_size_limit, 400, post);
        assertEquals(null, dataset);
    }
}
