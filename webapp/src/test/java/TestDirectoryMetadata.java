import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;
import static org.junit.jupiter.api.Assertions.*;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.AfterAll;

/*
  Test requesting directory descriptions from the server
*/
public class TestDirectoryMetadata extends BasicUnitTest {

    @BeforeAll
    public static void setUp() throws Exception {
        startServer("../../data/tests/metadata/config.csv");
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testDescription(boolean post) throws Exception {

        DirectoryEntry entry = client.requestDirectoryEntry("/tests/metadata", 200, post);

        // Should have two files and no subdirectories
        assertEquals(0, entry.directories.size());
        assertEquals(2, entry.files.size());

        // Check description
        assertEquals("metadata test directory", entry.description);

        // Check labels
        assertEquals(2, entry.labels.size());
        assertEquals("first test data file", entry.labels.get("test_data.hdf5"));
        assertEquals("second test data file", entry.labels.get("test_data_2d.hdf5"));
    }
}
