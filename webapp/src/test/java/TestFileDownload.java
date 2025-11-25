import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.AfterAll;
import static org.junit.jupiter.api.Assertions.*;

import java.nio.file.Files;
import java.nio.file.Paths;

/*
  Test downloading a file from the server
*/
public class TestFileDownload extends BasicUnitTest {

    @Test
    public void testFileNotFound() throws Exception {
	// Download the wrong filename!
	byte[] data = client.requestFile("/tests/basic/test_data.hdf6", 404);
    }

    public void doFileDownload(String virtual_path) throws Exception {

        // Request and decode the directory entry for the file to download
        DirectoryEntry entry = client.requestDirectoryEntry(virtual_path, 200, false);

	// Download the file and check we get the expected amount of data
	byte[] response_data = client.requestFile(virtual_path, 200);
	assertEquals(response_data.length, entry.size);

	// Locate the same file on the file system
        String build_dir = System.getProperty("cmake.build.dir");
        assertNotNull(build_dir);
        String real_path = build_dir + "/../../../data" + virtual_path;

	// Read the file directly
        byte file_data[] = Files.readAllBytes(Paths.get(real_path));

	// Compare
	assertEquals(file_data.length, response_data.length);
	assertArrayEquals(file_data, response_data);
    }

    @Test
    public void testfileDownload1() throws Exception {
	doFileDownload("/tests/basic/test_data.hdf5");
    }

    @Test
    public void testfileDownload2() throws Exception {
	doFileDownload("/tests/basic/test_data_2d.hdf5");
    }

    // Paths are normalized by tomcat before the servlet gets them
    @Test
    public void testfileDownload3() throws Exception {
	doFileDownload("/tests/../tests/basic/test_data_2d.hdf5");
    }
}
