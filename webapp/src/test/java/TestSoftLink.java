import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;
import static org.junit.jupiter.api.Assertions.*;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.AfterAll;

/*
  Test requesting soft links from the server
*/
public class TestSoftLink extends BasicUnitTest {

    @BeforeAll
    public static void setUp() throws Exception {
        startServer("../../data/tests/soft_links/config.csv");
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testRequestLinkToGroup(boolean post) throws Exception {

        // Request a link to a group. This should automatically be dereferenced.
        String max_depth = null;
        String data_size_limit = null;
        HDF5Object result = client.requestObject("/tests/soft_links/test_data.hdf5", "/LinkToGroup", max_depth, data_size_limit, 200, post);

        // The result should be a group, and not a soft link
        assertNotEquals(null, result);
        assertTrue(result.isGroup());
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testRequestLinkToDataset(boolean post) throws Exception {

        // Request a link to a dataset. This should automatically be dereferenced.
        String max_depth = null;
        String data_size_limit = null;
        HDF5Object result = client.requestObject("/tests/soft_links/test_data.hdf5", "/LinkToDataset", max_depth, data_size_limit, 200, post);

        // The result should be a dataset, and not a soft link
        assertNotEquals(null, result);
        assertTrue(result.isDataset());
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testRequestRootWithLinks(boolean post) throws Exception {

        // Request the root group, which contains some soft links
        String max_depth = "5";
        String data_size_limit = "0";
        HDF5Object result = client.requestObject("/tests/soft_links/test_data.hdf5", "/", max_depth, data_size_limit, 200, post);
        assertNotEquals(null, result);

        // Check for expected entries and their types
        assertTrue(result.members.get("Group").isGroup());
        assertTrue(result.members.get("LinkToGroup").isSoftLink());
        assertTrue(result.members.get("Dataset").isDataset());
        assertTrue(result.members.get("LinkToDataset").isSoftLink());

        // Check link targets
        assertEquals("/Group", result.members.get("LinkToGroup").target);
        assertEquals("/Dataset", result.members.get("LinkToDataset").target);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testRequestRootWithBrokenLink(boolean post) throws Exception {

        // Request the root group, which contains some soft links (one of which is broken).
        String max_depth = "5";
        String data_size_limit = "0";
        HDF5Object result = client.requestObject("/tests/soft_links/broken_link.hdf5", "/", max_depth, data_size_limit, 200, post);
        assertNotEquals(null, result);
        assertEquals("/NoSuchDataset", result.members.get("BrokenLink").target);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testRequestBrokenLink(boolean post) throws Exception {

        // Request a broken link. Should fail.
        String max_depth = null;
        String data_size_limit = null;
        HDF5Object result = client.requestObject("/tests/soft_links/test_data.hdf5", "/BrokenLink", max_depth, data_size_limit, 404, post);
        assertEquals(null, result);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testRequestDatasetViaLinkedGroup(boolean post) throws Exception {

        // Request a dataset using a path that goes through a soft link
        String max_depth = null;
        String data_size_limit = null;
        HDF5Object result = client.requestObject("/tests/soft_links/test_data.hdf5", "/LinkToGroup/DatasetInGroup", max_depth, data_size_limit, 200, post);

        // The result should be a dataset
        assertNotEquals(null, result);
        assertTrue(result.isDataset());
    }
}
