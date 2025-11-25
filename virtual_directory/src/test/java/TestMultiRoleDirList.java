import uk.ac.dur.cosma.virtual_directory.VirtualDirectory;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectoryException;
import uk.ac.dur.cosma.virtual_directory.VirtualPathInfo;
import uk.ac.dur.cosma.virtual_directory.CheckRole;

import java.io.StringReader;
import java.io.BufferedReader;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

// Check that directory and file listings depend on our security role as
// expected in the case of multiple roles.
public class TestMultiRoleDirList {

    private static boolean isResolved(VirtualDirectory vdir, String path, CheckRole in_role) {

        boolean resolved = true;
        try {
            VirtualPathInfo pinfo = vdir.resolvePath(path, in_role);
        } catch (VirtualDirectoryException e) {
            resolved = false;
        }
        return resolved;
    }

    private static int countFiles(VirtualDirectory root, String path, CheckRole in_role) throws Exception {
        VirtualPathInfo pi = root.resolvePath(path, in_role);
        assertNull(pi.file);
        assertNotNull(pi.directory);
        return pi.directory.getFiles(in_role).size();
    }

    private static int countDirectories(VirtualDirectory root, String path, CheckRole in_role) throws Exception {
        VirtualPathInfo pi = root.resolvePath(path, in_role);
        assertNull(pi.file);
        assertNotNull(pi.directory);
        return pi.directory.getDirectories(in_role).size();
    }

    @Test
    public void main() throws Exception {

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

        // Everyone can list the directory root/subdir1, but only role1 members see the subdirectory
        assertEquals(0, countDirectories(vdir, "root/subdir1", in_none));
        assertEquals(1, countDirectories(vdir, "root/subdir1", in_role1));
        assertEquals(0, countDirectories(vdir, "root/subdir1", in_role2));
        assertEquals(1, countDirectories(vdir, "root/subdir1", in_both));

        // Everyone can see the file root/subdir1/file1
        assertEquals(1, countFiles(vdir, "root/subdir1", in_none));
        assertEquals(1, countFiles(vdir, "root/subdir1", in_role1));
        assertEquals(1, countFiles(vdir, "root/subdir1", in_role2));
        assertEquals(1, countFiles(vdir, "root/subdir1", in_both));

        // Role1 members can list root/subdir1/subdir2, but only role1+role2 members see the subdirectory
        assertFalse(isResolved(vdir, "root/subdir1/subdir2", in_none));
        assertEquals(0,countDirectories(vdir, "root/subdir1/subdir2", in_role1));
        assertFalse(isResolved(vdir, "root/subdir1/subdir2", in_role2));
        assertEquals(1, countDirectories(vdir, "root/subdir1/subdir2", in_both));

        // Role1 members can access the file in root/subdir1/subdir2
        assertEquals(1, countFiles(vdir, "root/subdir1/subdir2", in_role1));
        assertEquals(1, countFiles(vdir, "root/subdir1/subdir2", in_both));

        // Role1+role2 members can list root/subdir1/subdir2/subdir3 and access the file
        assertFalse(isResolved(vdir, "root/subdir1/subdir2/subdir3", in_none));
        assertFalse(isResolved(vdir, "root/subdir1/subdir2/subdir3", in_role1));
        assertFalse(isResolved(vdir, "root/subdir1/subdir2/subdir3", in_role2));
        assertEquals(1, countFiles(vdir, "root/subdir1/subdir2/subdir3", in_both));
        assertEquals(0, countDirectories(vdir, "root/subdir1/subdir2/subdir3", in_both));
    }
}
