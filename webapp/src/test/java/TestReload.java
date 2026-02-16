import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.AfterAll;
import static org.junit.jupiter.api.Assertions.*;

/*
  Test requesting a HDF5 group from the server.
*/
public class TestReload extends BasicUnitTest {

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testReloading(boolean post) throws Exception {

        // Request a dataset
        {
            HDF5Object dataset = client.requestObject("/tests/basic/test_data.hdf5", "/data", null, null, 200, post);
            assertNotEquals(null, dataset);
        }

        // Reload the server config
        assertEquals(true, client.requestReload());

        // Try the dataset request again
        {
            HDF5Object dataset = client.requestObject("/tests/basic/test_data.hdf5", "/data", null, null, 200, post);
            assertNotEquals(null, dataset);
        }
    }
}
