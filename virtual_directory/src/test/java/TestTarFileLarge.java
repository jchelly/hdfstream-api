import uk.ac.dur.cosma.virtual_directory.VirtualDirectory;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectoryException;
import uk.ac.dur.cosma.virtual_directory.VirtualPathInfo;
import uk.ac.dur.cosma.virtual_directory.TarFile;

import java.io.StringReader;
import java.io.BufferedReader;
import java.io.FileOutputStream;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.nio.file.Paths;
import java.nio.file.Files;
import java.io.File;
import java.util.HashSet;
import org.apache.commons.compress.archivers.tar.*;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

// Check that tar file generation works as expected.
// In this case we include files larger than the record size (512) and block size (20*512).
public class TestTarFileLarge {

    @Test
    public void main() throws VirtualDirectoryException, IOException {

        // Config 'file' to read. Columns are:
        // Real path, virtual path, size, last modified, media type
        String config =
            "root,                          no_such_file,                   0, 0, directory\n" +
            "root/subdir1,                  no_such_file,                   0, 0, directory\n" +
            "root/subdir1/file1,            tmp_test_tar_file/s1_file1,     10240, 0, application/octet-stream\n" + // exactly one block
            "root/subdir1/file2,            tmp_test_tar_file/s1_file2,     13792, 0, application/octet-stream\n" + // just over one block
            "root/subdir2,                  no_such_file,                   0, 0, directory\n" +
            "root/subdir2/file1,            tmp_test_tar_file/s2_file1,     202209, 0, application/octet-stream\n" + // many blocks plus a bit
            "root/subdir2/subsubdir1,       no_such_file,                   0, 0, directory\n" +
            "root/subdir2/subsubdir1/file1, tmp_test_tar_file/s2_ss1_file1, 399360, 0, application/octet-stream\n"; // a whole number of blocks

        // Set up the reader
        BufferedReader reader = new BufferedReader(new StringReader(config));

        // Construct the virtual directory structure
        VirtualDirectory vdir = new VirtualDirectory();
        vdir.addFromReader(reader);

        // Create a directory to store the test files
        Files.createDirectories(Paths.get("tmp_test_tar_file"));

        // Set up an array of bytes to write to the files
        int nmax = 1024*1024;
        byte[] data = new byte[nmax];
        for(int i=0; i<nmax; i+=1)
            data[i] =  (byte) (i % 128);

        // Create the files
        FileOutputStream s1_file1 = new FileOutputStream(new File("tmp_test_tar_file/s1_file1"));
        s1_file1.write(data, 0, 10240);
        s1_file1.close();

        FileOutputStream s1_file2 = new FileOutputStream(new File("tmp_test_tar_file/s1_file2"));
        s1_file2.write(data, 0, 13792);
        s1_file2.close();

        FileOutputStream s2_file1 = new FileOutputStream(new File("tmp_test_tar_file/s2_file1"));
        s2_file1.write(data, 0, 202209);
        s2_file1.close();

        FileOutputStream s2_ss1_file1 = new FileOutputStream(new File("tmp_test_tar_file/s2_ss1_file1"));
        s2_ss1_file1.write(data, 0, 399360);
        s2_ss1_file1.close();

        // Write the test files to a new tar file
        TarFile tarfile = new TarFile("", vdir, 1024);
        FileOutputStream tarstream = new FileOutputStream(new File("tmp_test_tar_file.tar"));
        tarfile.write(tarstream);
        tarstream.close();

        // Now we read back the tar file and check the results
        TarArchiveInputStream instream = new TarArchiveInputStream(new FileInputStream(new File("tmp_test_tar_file.tar")));
        TarArchiveEntry entry = null;
        HashSet<String> all_paths = new HashSet<String>();
        while ((entry = instream.getNextEntry()) != null) {
            if (entry.isFile()) {
                // Get the file metadata
                String input_path = entry.getName();
                all_paths.add(input_path);
                // Read all data from the file
                byte[] input_data = instream.readAllBytes();

                // Now verify file names and contents
                switch(input_path) {
                case "root/subdir1/file1":
                    assertEquals(10240, input_data.length);
                    break;
                case "root/subdir1/file2":
                    assertEquals(13792, input_data.length);
                    break;
                case "root/subdir2/file1":
                    assertEquals(202209, input_data.length);
                    break;
                case "root/subdir2/subsubdir1/file1":
                    assertEquals(399360, input_data.length);
                    break;
                default:
                    fail("Invalid filename: "+input_path);
                }

                // Check file contents
                for(int i=0; i<input_data.length; i+=1)
                    assertEquals((byte) (i % 128), input_data[i]);
            }
        }
        // Check we found all of the files
        assertEquals(4, all_paths.size());
    }
}
