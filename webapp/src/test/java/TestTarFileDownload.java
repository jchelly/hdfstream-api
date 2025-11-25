import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.AfterAll;
import static org.junit.jupiter.api.Assertions.*;

import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.HashMap;

/*
  Test downloading a file from the server
*/
public class TestTarFileDownload extends BasicUnitTest {

    public void doTarFileDownload(String filename, int expected_status) throws Exception {

	// Request the contents of a tar file
	HashMap<String, byte[]> data = client.requestTarFile(filename, expected_status);
	if(expected_status != 200) {
	    assertNull(data);
	    return;
	} else {
	    assertNotNull(data);
	}

	// Loop over the files we received
	for (HashMap.Entry<String, byte[]> entry : data.entrySet()) {
	    String response_path = entry.getKey();
	    byte[] response_data = entry.getValue();

	    // Read the same file from the file system
	    String build_dir = System.getProperty("cmake.build.dir");
	    assertNotNull(build_dir);
	    String real_path = build_dir + "/../../../data/" + response_path;
	    byte file_data[] = Files.readAllBytes(Paths.get(real_path));

	    // Check that the tar file contents match the data from the file system
	    assertArrayEquals(file_data, response_data);
	}
    }

    @Test
    public void testTarFileDownload() throws Exception {
	doTarFileDownload("/tests/basic", 200);
    }

    @Test
    public void testFailedTarFileDownload() throws Exception {
	doTarFileDownload("/tests/not-found", 404);
    }

    // Tomcat normalizes paths, so this works (somewhat surprisingly!)
    // This doesn't allow access to any paths outside the config, but
    // perhaps it should be changed anyway?
    @Test
    public void testParentDirTarFileDownload() throws Exception {
	doTarFileDownload("/tests/../tests/basic", 200);
    }

    // The service does not currently allow downloading everything from the root
    @Test
    public void testRootTarFileDownload() throws Exception {
	doTarFileDownload("/", 400);
    }

    @Test
    public void testEmptyTarFileDownload() throws Exception {
	doTarFileDownload("", 400);
    }
}
