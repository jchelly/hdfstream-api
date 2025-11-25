import uk.ac.dur.cosma.virtual_directory.VirtualDirectory;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectoryException;
import uk.ac.dur.cosma.virtual_directory.VirtualPathInfo;
import uk.ac.dur.cosma.virtual_directory.CheckRole;

import java.io.StringReader;
import java.io.BufferedReader;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

// Check that the available paths depend on our security role as expected
public class TestSingleRole {

    private static boolean isResolved(VirtualDirectory vdir, String path, CheckRole in_role) {

        boolean resolved = true;
        try {
            VirtualPathInfo pinfo = vdir.resolvePath(path, in_role);
        } catch (VirtualDirectoryException e) {
            resolved = false;
        }
        return resolved;
    }

    @Test
    public void main() throws VirtualDirectoryException {

        // Config 'file' to read. Columns are:
        // Real path, virtual path, size, last modified, media type, [role1;role2;...]
        String config =
            "root,                          no_such_file, 0, 0, directory\n" +
            "root/subdir1,                  no_such_file, 0, 0, directory\n" +
            "root/subdir1/file1,            no_such_file, 0, 0, application/octet-stream\n" +
            "root/subdir1/file2,            no_such_file, 0, 0, application/octet-stream\n" +
            "root/subdir2,                  no_such_file, 0, 0, directory, role1\n" +
            "root/subdir2/file1,            no_such_file, 0, 0, application/octet-stream\n" +
            "root/subdir2/subsubdir1,       no_such_file, 0, 0, directory\n" +
            "root/subdir2/subsubdir1/file1, no_such_file, 0, 0, application/octet-stream\n";

        // Set up the reader
        BufferedReader reader = new BufferedReader(new StringReader(config));

        // Construct the virtual directory structure
        VirtualDirectory vdir = new VirtualDirectory();
        vdir.addFromReader(reader);

        CheckRole in_role1 = (name) -> name.equals("role1");
        CheckRole in_role2 = (name) -> name.equals("role2");

        // If we're in role1 we should be able to access subdir2
        assertTrue(isResolved(vdir, "root/subdir2", in_role1));

        // If we're in role1 there should be a total of four files
        assertEquals(4, vdir.getAllFiles(in_role1).size());

        // If we're not in role1 we should not be able to resolve subdir2
        assertFalse(isResolved(vdir, "root/subdir2", in_role2));

        // We also should not be able to resolve anything below subdir2
        assertFalse(isResolved(vdir, "root/subdir2/file1", in_role2));
        assertFalse(isResolved(vdir, "root/subdir2/subsubdir1", in_role2));
        assertFalse(isResolved(vdir, "root/subdir2/subsubdir1/file1", in_role2));

        // If we're not in role1 there should be a total of two files
        assertEquals(2, vdir.getAllFiles(in_role2).size());

        // Get a reference to subdir2
        VirtualDirectory subdir2 = vdir.resolvePath("root/subdir2", in_role1).directory; // Use role1 so it succeeds

        // Should have 1 file if we're in role1 and 0 otherwise
        assertEquals(1, subdir2.getFiles(in_role1).size());
        assertEquals(0, subdir2.getFiles(in_role2).size());

        // Should have 1 directory if we're in role1 and 0 otherwise
        assertEquals(1, subdir2.getDirectories(in_role1).size());
        assertEquals(0, subdir2.getDirectories(in_role2).size());
    }
}
