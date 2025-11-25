import uk.ac.dur.cosma.virtual_directory.VirtualDirectory;
import uk.ac.dur.cosma.virtual_directory.VirtualFile;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectoryException;
import uk.ac.dur.cosma.virtual_directory.VirtualPathInfo;
import uk.ac.dur.cosma.virtual_directory.CheckRole;

import java.io.StringReader;
import java.io.BufferedReader;
import java.util.LinkedHashMap;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

// Check that finding all files in a directory hierarchy directory
// depends on our security role as expected in the case of multiple roles.
public class TestGetAllFiles {

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

        LinkedHashMap<String, VirtualFile> result;

        // Users in no role can only see file1
        result = vdir.getAllFiles(in_none);
        assertEquals(1, result.size());
        assertTrue(result.containsKey("root/subdir1/file1"));

        // Users in role2 only can only see file1
        result = vdir.getAllFiles(in_role2);
        assertEquals(1, result.size());
        assertTrue(result.containsKey("root/subdir1/file1"));

        // Members of role1 can see file1 and file2
        result = vdir.getAllFiles(in_role1);
        assertEquals(2, result.size());
        assertTrue(result.containsKey("root/subdir1/file1"));
        assertTrue(result.containsKey("root/subdir1/subdir2/file2"));

        // Members of role1+role2 can see all three files
        result = vdir.getAllFiles(in_both);
        assertEquals(3, result.size());
        assertTrue(result.containsKey("root/subdir1/file1"));
        assertTrue(result.containsKey("root/subdir1/subdir2/file2"));
        assertTrue(result.containsKey("root/subdir1/subdir2/subdir3/file3"));
    }
}
