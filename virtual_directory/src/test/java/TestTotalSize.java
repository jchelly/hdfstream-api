import uk.ac.dur.cosma.virtual_directory.VirtualDirectory;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectoryException;
import uk.ac.dur.cosma.virtual_directory.VirtualPathInfo;
import uk.ac.dur.cosma.virtual_directory.CheckRole;

import java.io.StringReader;
import java.io.BufferedReader;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

// Check that the total file size is correct and depends on our security role as expected
public class TestTotalSize {

    @Test
    public void TestTotalSizeByRole() throws VirtualDirectoryException {

        // Config 'file' to read. Columns are:
        // Real path, virtual path, size, last modified, media type, [role1;role2;...]
        String config =
            "root,                          no_such_file, 0,   0, directory\n" +
            "root/subdir1,                  no_such_file, 0,   0, directory\n" +
            "root/subdir1/file1,            no_such_file, 100, 0, application/octet-stream\n" +
            "root/subdir1/file2,            no_such_file, 75,  0, application/octet-stream\n" +
            "root/subdir2,                  no_such_file, 0,   0, directory, role1\n" +
            "root/subdir2/file1,            no_such_file, 3,   0, application/octet-stream\n" +
            "root/subdir2/subsubdir1,       no_such_file, 0,   0, directory\n" +
            "root/subdir2/subsubdir1/file1, no_such_file, 220, 0, application/octet-stream\n";

        // Set up the reader
        BufferedReader reader = new BufferedReader(new StringReader(config));

        // Construct the virtual directory structure
        VirtualDirectory vdir = new VirtualDirectory();
        vdir.addFromReader(reader);

        CheckRole in_role1 = (name) -> name.equals("role1");
        CheckRole in_role2 = (name) -> name.equals("role2");

        // Total size depends on which files we can access. Role1 can access all files.
        assertEquals(100+75+3+220, vdir.getTotalSize(in_role1));

        // Role2 can only access a subset
        assertEquals(100+75, vdir.getTotalSize(in_role2));
    }
}
