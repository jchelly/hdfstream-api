import uk.ac.dur.cosma.virtual_directory.VirtualDirectory;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectoryException;
import uk.ac.dur.cosma.virtual_directory.VirtualPathInfo;

import java.io.StringReader;
import java.io.BufferedReader;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

// Check that implicit virtual directory creation works as expected:
// Here we only specify files in the virtual directory config.
public class TestImplicitDirectory {

    private static void verify(boolean condition) {
        if(!condition) {
            throw new RuntimeException("Test failed");
        }
    }

    @Test
    public void main() throws VirtualDirectoryException {

        // Config 'file' to read. Columns are:
        // Real path, virtual path, size, last modified, media type
        String config =
            "root/subdir1/file1,            no_such_file, 0, 0, application/octet-stream\n" +
            "root/subdir1/file2,            no_such_file, 0, 0, application/octet-stream\n" +
            "root/subdir2/file1,            no_such_file, 0, 0, application/octet-stream\n" +
            "root/subdir2/subsubdir1/file1, no_such_file, 0, 0, application/octet-stream\n";

        // Set up the reader
        BufferedReader reader = new BufferedReader(new StringReader(config));

        // Construct the virtual directory structure
        VirtualDirectory vdir = new VirtualDirectory();
        vdir.addFromReader(reader);

        // Root should be a directory
        VirtualPathInfo root = vdir.resolvePath("root", (in) -> true);
        assertNull(root.file);
        assertNotNull(root.directory);

        // Subdir1 is a directory with two files
        VirtualPathInfo subdir1 = vdir.resolvePath("root/subdir1", (in) -> true);
        assertNull(subdir1.file);
        assertNotNull(subdir1.directory);
        assertEquals(2, subdir1.directory.getFiles((in) -> true).size());

        // Subdir2 is a directory with one file
        VirtualPathInfo subdir2 = vdir.resolvePath("root/subdir2", (in) -> true);
        assertNull(subdir2.file);
        assertNotNull(subdir2.directory);
        assertEquals(1, subdir2.directory.getFiles((in) -> true).size());

        // Subsubdir1 is a directory with one file
        VirtualPathInfo subsubdir1 = vdir.resolvePath("root/subdir2/subsubdir1", (in) -> true);
        assertNull(subsubdir1.file);
        assertNotNull(subsubdir1.directory);
        assertEquals(1, subsubdir1.directory.getFiles((in) -> true).size());

        // Check files:
        // In this case we should get a file and parent directory back.
        VirtualPathInfo subdir1_file1 = vdir.resolvePath("root/subdir1/file1", (in) -> true);
        assertNotNull(subdir1_file1.file);
        assertNotNull(subdir1_file1.directory);
        assertEquals("file1", subdir1_file1.basename);
        VirtualPathInfo subdir1_file2 = vdir.resolvePath("root/subdir1/file2", (in) -> true);
        assertNotNull(subdir1_file2.file);
        assertNotNull(subdir1_file2.directory);
        assertEquals("file2", subdir1_file2.basename);
        VirtualPathInfo subdir2_file1 = vdir.resolvePath("root/subdir2/file1", (in) -> true);
        assertNotNull(subdir2_file1.file);
        assertNotNull(subdir2_file1.directory);
        assertEquals("file1", subdir2_file1.basename);
        VirtualPathInfo subsubdir1_file1 = vdir.resolvePath("root/subdir2/subsubdir1/file1", (in) -> true);
        assertNotNull(subsubdir1_file1.file);
        assertNotNull(subsubdir1_file1.directory);
        assertEquals("file1", subsubdir1_file1.basename);
    }
}
