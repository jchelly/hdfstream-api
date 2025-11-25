import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.AfterAll;
import static org.junit.jupiter.api.Assertions.*;

/*
  Test requesting a HDF5 group from the server.
*/
public class TestGroup extends BasicUnitTest {

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testDefaultMetadata(boolean post) throws Exception {

        // Request root without max depth or size limit:
        // Default is to load datasets up to 2GB but not recurse into subgroups.
        String max_depth = null;
        String data_size_limit = null;
        HDF5Object root = client.requestObject("/tests/basic/test_data.hdf5", "/", max_depth, data_size_limit, 200, post);

        // Check the response is what we expected:
        assertNotEquals(null, root);
        assertEquals("/", root.path);
        assertEquals(1, root.members.size());

        // The dataset data should be a 1D array with 10 elements
        HDF5Object dataset = root.members.get("data");
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
    public void testAllMetadata(boolean post) throws Exception {

        // Request all metadata but no dataset contents from a file
        String max_depth = "10";
        String data_size_limit = "0";
        HDF5Object root = client.requestObject("/tests/basic/test_data.hdf5", "/", max_depth, data_size_limit, 200, post);

        // Check the response is what we expected:
        assertNotEquals(null, root);
        assertEquals("/", root.path);
        assertEquals(1, root.members.size());

        // The dataset data should be a 1D array with 10 elements
        HDF5Object dataset = root.members.get("data");
        assertNotEquals(null, dataset);
        assertEquals(1, dataset.shape.length);
        assertEquals(10, dataset.shape[0]);

        // We should not have loaded the dataset contents
        assertEquals(null, dataset.data);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testAllData(boolean post) throws Exception {

        // Request all metadata and dataset contents from a file
        String max_depth = "10";
        String data_size_limit = "1000000";
        HDF5Object root = client.requestObject("/tests/basic/test_data.hdf5", "/", max_depth, data_size_limit, 200, post);

        // Check the response is what we expected:
        assertNotEquals(null, root);
        assertEquals("/", root.path);
        assertEquals(1, root.members.size());

        // The dataset data should be a 1D array with 10 elements
        HDF5Object dataset = root.members.get("data");
        assertNotEquals(null, dataset);
        assertEquals(1, dataset.shape.length);
        assertEquals(10, dataset.shape[0]);

        // We should have loaded the dataset contents too
        NDArray array = dataset.data;
        assertNotEquals(null, array);
        assertEquals(1, array.shape.length);
        assertEquals(10, array.shape[0]);
        for(int i=0; i<10; i+=1)
            assertEquals(i, array.getElementAsLong(i));
    }

    @ParameterizedTest
    @ValueSource(booleans = {false})
    public void testInvalidMaxDepth(boolean post) throws Exception {

        // Request with invalid max depth
        String max_depth = "abc";
        String data_size_limit = "0";
        HDF5Object root = client.requestObject("/tests/basic/test_data.hdf5", "/", max_depth, data_size_limit, 400, post);

        // Check the response is what we expected:
        assertEquals(null, root);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false})
    public void testMaxDepthLeadingSpace(boolean post) throws Exception {

        // Request with invalid max depth
        String max_depth = " 10";
        String data_size_limit = "0";
        HDF5Object root = client.requestObject("/tests/basic/test_data.hdf5", "/", max_depth, data_size_limit, 400, post);

        // Check the response is what we expected:
        assertEquals(null, root);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false})
    public void testMaxDepthTrailingSpace(boolean post) throws Exception {

        // Request with invalid max depth
        String max_depth = "10 ";
        String data_size_limit = "0";
        HDF5Object root = client.requestObject("/tests/basic/test_data.hdf5", "/", max_depth, data_size_limit, 400, post);

        // Check the response is what we expected:
        assertEquals(null, root);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testNegativeMaxDepth(boolean post) throws Exception {

        // Request with invalid max depth
        String max_depth = "-100";
        String data_size_limit = "0";
        HDF5Object root = client.requestObject("/tests/basic/test_data.hdf5", "/", max_depth, data_size_limit, 400, post);

        // Check the response is what we expected:
        assertEquals(null, root);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false})
    public void testFloatMaxDepth(boolean post) throws Exception {

        // Request with invalid max depth
        String max_depth = "3.0";
        String data_size_limit = "0";
        HDF5Object root = client.requestObject("/tests/basic/test_data.hdf5", "/", max_depth, data_size_limit, 400, post);

        // Check the response is what we expected:
        assertEquals(null, root);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false})
    public void testEmptyMaxDepth(boolean post) throws Exception {

        // Request with invalid max depth
        String max_depth = "";
        String data_size_limit = "0";
        HDF5Object root = client.requestObject("/tests/basic/test_data.hdf5", "/", max_depth, data_size_limit, 400, post);

        // Check the response is what we expected:
        assertEquals(null, root);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false})
    public void testSpaceMaxDepth(boolean post) throws Exception {

        // Request with invalid max depth
        String max_depth = " ";
        String data_size_limit = "0";
        HDF5Object root = client.requestObject("/tests/basic/test_data.hdf5", "/", max_depth, data_size_limit, 400, post);

        // Check the response is what we expected:
        assertEquals(null, root);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false})
    public void testInvalidDataSizeLimit(boolean post) throws Exception {

        // Request with invalid data size limit
        String max_depth = "10";
        String data_size_limit = "xyz";
        HDF5Object root = client.requestObject("/tests/basic/test_data.hdf5", "/", max_depth, data_size_limit, 400, post);

        // Check the response is what we expected:
        assertEquals(null, root);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false})
    public void testLeadingSpaceDataSizeLimit(boolean post) throws Exception {

        // Request with invalid data size limit
        String max_depth = "10";
        String data_size_limit = " 10000";
        HDF5Object root = client.requestObject("/tests/basic/test_data.hdf5", "/", max_depth, data_size_limit, 400, post);

        // Check the response is what we expected:
        assertEquals(null, root);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false})
    public void testTrailingSpaceDataSizeLimit(boolean post) throws Exception {

        // Request with invalid data size limit
        String max_depth = "10";
        String data_size_limit = "10000 ";
        HDF5Object root = client.requestObject("/tests/basic/test_data.hdf5", "/", max_depth, data_size_limit, 400, post);

        // Check the response is what we expected:
        assertEquals(null, root);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testNegativeDataSizeLimit(boolean post) throws Exception {

        // Request with invalid data size limit
        String max_depth = "10";
        String data_size_limit = "-100";
        HDF5Object root = client.requestObject("/tests/basic/test_data.hdf5", "/", max_depth, data_size_limit, 400, post);

        // Check the response is what we expected:
        assertEquals(null, root);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false})
    public void testFloatDataSizeLimit(boolean post) throws Exception {

        // Request with invalid data size limit
        String max_depth = "10";
        String data_size_limit = "1000.0";
        HDF5Object root = client.requestObject("/tests/basic/test_data.hdf5", "/", max_depth, data_size_limit, 400, post);

        // Check the response is what we expected:
        assertEquals(null, root);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false})
    public void testEmptyDataSizeLimit(boolean post) throws Exception {

        // Request with invalid data size limit
        String max_depth = "10";
        String data_size_limit = "";
        HDF5Object root = client.requestObject("/tests/basic/test_data.hdf5", "/", max_depth, data_size_limit, 400, post);

        // Check the response is what we expected:
        assertEquals(null, root);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false})
    public void testSpaceDataSizeLimit(boolean post) throws Exception {

        // Request with invalid data size limit
        String max_depth = "10";
        String data_size_limit = " ";
        HDF5Object root = client.requestObject("/tests/basic/test_data.hdf5", "/", max_depth, data_size_limit, 400, post);

        // Check the response is what we expected:
        assertEquals(null, root);
    }
}
