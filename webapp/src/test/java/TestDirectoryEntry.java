import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.AfterAll;
import static org.junit.jupiter.api.Assertions.*;

/*
  Test requesting directory listings from the server.
*/
public class TestDirectoryEntry extends BasicUnitTest {

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testTopLevelNoMaxDepth(boolean post) throws Exception {

        // Send a request for the root directory listing to the server
        DirectoryEntry entry = client.requestDirectoryEntry("", 200, post);

        // Check the response is what we expected:
        // Should contain a single directory called "tests" and no files.
        assertEquals(1, entry.directories.size());
        assertEquals(0, entry.files.size());
        assertEquals(true, entry.directories.containsKey("tests"));
        assertEquals(null, entry.directories.get("tests")); // We didn't load any subdirs
    }

    @ParameterizedTest
    @ValueSource(booleans = {false})
    public void testTopLevelInvalidMaxDepth(boolean post) throws Exception {

        // Send a request for the root directory with max_depth value which is not an integer
        DirectoryEntry entry = client.requestDirectoryEntry("", "abc", 400, post); // Should get bad request status code
        assertEquals(null, entry);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testTopLevelMaxDepth0(boolean post) throws Exception {

        // Send a request for the root directory with max_depth=0
        DirectoryEntry entry = client.requestDirectoryEntry("", "0", 200, post);
        assertEquals(1, entry.directories.size());
        assertEquals(0, entry.files.size());
        assertEquals(true, entry.directories.containsKey("tests"));
        assertEquals(null, entry.directories.get("tests")); // We didn't load any subdirs
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testTopLevelMaxDepth1(boolean post) throws Exception {

        // Send a request for the root directory with max_depth=1
        DirectoryEntry root_dir = client.requestDirectoryEntry("", "1", 200, post);
        assertEquals(1, root_dir.directories.size());
        assertEquals(0, root_dir.files.size());
        assertEquals(true, root_dir.directories.containsKey("tests"));

        // Should have loaded the tests subdir but not its subdirectories
        DirectoryEntry tests_dir = root_dir.directories.get("tests");
        assertNotEquals(null, tests_dir);
        assertEquals(1, tests_dir.directories.size());
        assertEquals(0, tests_dir.files.size());
        assertEquals(true, tests_dir.directories.containsKey("basic"));
        assertEquals(null, tests_dir.directories.get("basic")); // We didn't load subdirs of tests_dir
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testTopLevelMaxDepth2(boolean post) throws Exception {

        // Send a request for the root directory with max_depth=2
        DirectoryEntry root_dir = client.requestDirectoryEntry("", "2", 200, post);
        assertEquals(1, root_dir.directories.size());
        assertEquals(0, root_dir.files.size());
        assertEquals(true, root_dir.directories.containsKey("tests"));

        // Should have loaded the /tests subdir
        DirectoryEntry tests_dir = root_dir.directories.get("tests");
        assertNotEquals(null, tests_dir);
        assertEquals(1, tests_dir.directories.size());
        assertEquals(0, tests_dir.files.size());
        assertEquals(true, tests_dir.directories.containsKey("basic"));
        assertNotEquals(null, tests_dir.directories.get("basic"));

        // Should also have loaded /tests/basic subdir
        DirectoryEntry basic_dir = tests_dir.directories.get("basic");
        assertNotEquals(null, basic_dir);
        assertEquals(0, basic_dir.directories.size());
        assertEquals(2, basic_dir.files.size());

        // And we should have metadata for a file /tests/basic/test_data.hdf5
        DirectoryEntry test_data_hdf5 = basic_dir.files.get("test_data.hdf5");
        assertNotEquals(null, test_data_hdf5);
        assertEquals("application/x-hdf5", test_data_hdf5.type);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testSubDirMaxDepth1(boolean post) throws Exception {

        // Send a request for the tests directory with max_depth=1
        DirectoryEntry tests_dir = client.requestDirectoryEntry("/tests", "1", 200, post);
        assertNotEquals(null, tests_dir);
        assertEquals(1, tests_dir.directories.size());
        assertEquals(0, tests_dir.files.size());
        assertEquals(true, tests_dir.directories.containsKey("basic"));
        assertNotEquals(null, tests_dir.directories.get("basic"));

        // Should also have loaded /tests/basic subdir
        DirectoryEntry basic_dir = tests_dir.directories.get("basic");
        assertNotEquals(null, basic_dir);
        assertEquals(0, basic_dir.directories.size());
        assertEquals(2, basic_dir.files.size());

        // And we should have metadata for a file /tests/basic/test_data.hdf5
        DirectoryEntry test_data_hdf5 = basic_dir.files.get("test_data.hdf5");
        assertNotEquals(null, test_data_hdf5);
        assertEquals("application/x-hdf5", test_data_hdf5.type);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testFile(boolean post) throws Exception {
        // Send a request for file metadata
        DirectoryEntry test_data_hdf5 = client.requestDirectoryEntry("/tests/basic/test_data.hdf5", 200, post);
        assertNotEquals(null, test_data_hdf5);
        assertEquals("application/x-hdf5", test_data_hdf5.type);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testMissingFile(boolean post) throws Exception {
        // Send a request for metadata for a non-existent file (or directory) in a directory
        DirectoryEntry test_data_hdf5 = client.requestDirectoryEntry("/tests/basic/test_data.hdf5-NOT-FOUND", 404, post);
        assertEquals(null, test_data_hdf5);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testMissingDirectory(boolean post) throws Exception {
        // Send a request for metadata for a non-existent top level directory
        DirectoryEntry tests_dir = client.requestDirectoryEntry("/tests-NOT-FOUND", 404, post);
        assertEquals(null, tests_dir);
    }

}
