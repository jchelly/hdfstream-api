import uk.ac.dur.cosma.virtual_directory.VirtualDirectory;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectoryException;
import uk.ac.dur.cosma.virtual_directory.VirtualPathInfo;
import uk.ac.dur.cosma.virtual_directory.CheckRole;

import java.io.StringReader;
import java.io.BufferedReader;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

// Check that the available paths depend on our security role as expected
// in the case of multiple roles.
public class TestMultiRole {

    private static void verify(boolean condition) {
        if(!condition) {
            throw new RuntimeException("Test failed");
        }
    }

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
            "root,                               no_such_file, 0, 0, directory\n" +
            "root/subdir1,                       no_such_file, 0, 0, directory\n" +
            "root/subdir1/file1,                 no_such_file, 0, 0, application/octet-stream\n" +
            "root/subdir1/subdir2,               no_such_file, 0, 0, directory, role1\n" +
            "root/subdir1/subdir2/file2,         no_such_file, 0, 0, application/octet-stream\n" +
            "root/subdir1/subdir2/subdir3,       no_such_file, 0, 0, directory, role1;role2\n" +
            "root/subdir1/subdir2/subdir3/file3, no_such_file, 0, 0, application/octet-stream\n";

        // Set up the reader
        BufferedReader reader = new BufferedReader(new StringReader(config));

        // Construct the virtual directory structure
        VirtualDirectory vdir = new VirtualDirectory();
        vdir.addFromReader(reader);

        CheckRole in_none  = (name) -> false;
        CheckRole in_role1 = (name) -> name.equals("role1");
        CheckRole in_role2 = (name) -> name.equals("role2");
        CheckRole in_both  = (name) -> (name.equals("role1") || name.equals("role2"));

        // Anyone should be able to access subdir1
        assertTrue(isResolved(vdir, "root/subdir1", in_none));
        assertTrue(isResolved(vdir, "root/subdir1/file1", in_none));
        assertTrue(isResolved(vdir, "root/subdir1", in_role1));
        assertTrue(isResolved(vdir, "root/subdir1/file1", in_role1));
        assertTrue(isResolved(vdir, "root/subdir1", in_role2));
        assertTrue(isResolved(vdir, "root/subdir1/file1", in_role2));
        assertTrue(isResolved(vdir, "root/subdir1", in_both));
        assertTrue(isResolved(vdir, "root/subdir1/file1", in_both));

        // Need to belong to role1 to access subdir2
        assertFalse(isResolved(vdir, "root/subdir1/subdir2", in_none));
        assertFalse(isResolved(vdir, "root/subdir1/subdir2/file2", in_none));
        assertTrue( isResolved(vdir, "root/subdir1/subdir2", in_role1));
        assertTrue( isResolved(vdir, "root/subdir1/subdir2/file2", in_role1));
        assertFalse(isResolved(vdir, "root/subdir1/subdir2", in_role2));
        assertFalse(isResolved(vdir, "root/subdir1/subdir2/file2", in_role2));
        assertTrue( isResolved(vdir, "root/subdir1/subdir2", in_both));
        assertTrue( isResolved(vdir, "root/subdir1/subdir2/file2", in_both));

        // Need to belong to role1 AND role2 to access subdir3
        assertFalse(isResolved(vdir, "root/subdir1/subdir2/subdir3", in_none));
        assertFalse(isResolved(vdir, "root/subdir1/subdir2/subdir3/file3", in_none));
        assertFalse(isResolved(vdir, "root/subdir1/subdir2/subdir3", in_role1));
        assertFalse(isResolved(vdir, "root/subdir1/subdir2/subdir3/file3", in_role1));
        assertFalse(isResolved(vdir, "root/subdir1/subdir2/subdir3", in_role2));
        assertFalse(isResolved(vdir, "root/subdir1/subdir2/subdir3/file3", in_role2));
        assertTrue( isResolved(vdir, "root/subdir1/subdir2/subdir3", in_both));
        assertTrue( isResolved(vdir, "root/subdir1/subdir2/subdir3/file3", in_both));
    }
}
