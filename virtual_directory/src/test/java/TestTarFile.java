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
import java.util.stream.Stream;
import java.util.Arrays;
import org.apache.commons.compress.archivers.tar.*;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.MethodSource;


// Check that tar file generation works as expected
public class TestTarFile {

    private static int[][] test_cases = {
        {1,   2,   3,   4},
        {1,   2,   0,   4},
        {100, 200, 300, 400},
        {100, 200, 300, 0},
        {0, 200, 300, 400},
        {512, 512, 512, 512},
        {20*512, 20*512, 20*512, 20*512},
        {20*512-1, 20*512, 20*512, 20*512},
        {20*512+1, 20*512, 20*512, 20*512},
        {100, 200, 511, 400}, // Files just larger or smaller than record size
        {100, 200, 512, 400},
        {100, 200, 513, 400},
        {100, 200, 20*512-1, 400}, // Files just larger or smaller than block size
        {100, 200, 20*512+0, 400},
        {100, 200, 20*512+1, 400},
    };

    private static Stream<int[]> testCases() {
        return Arrays.stream(test_cases);
    }

    @ParameterizedTest(name = "Test case {index}")
    @MethodSource("testCases")
    void test(int[] test_case) throws VirtualDirectoryException, IOException {
        run_test(test_case);
    }

    public void run_test(int[] test_case) throws VirtualDirectoryException, IOException {

        // Config 'file' to read. Columns are:
        // Real path, virtual path, size, last modified, media type
        String config =
            "root,                          no_such_file,                   0, 0, directory\n" +
            "root/subdir1,                  no_such_file,                   0, 0, directory\n" +
            "root/subdir1/file1,            tmp_test_tar_file/s1_file1,     "+Integer.valueOf(test_case[0])+", 0, application/octet-stream\n" +
            "root/subdir1/file2,            tmp_test_tar_file/s1_file2,     "+Integer.valueOf(test_case[1])+", 0, application/octet-stream\n" +
            "root/subdir2,                  no_such_file,                   0, 0, directory\n" +
            "root/subdir2/file1,            tmp_test_tar_file/s2_file1,     "+Integer.valueOf(test_case[2])+", 0, application/octet-stream\n" +
            "root/subdir2/subsubdir1,       no_such_file,                   0, 0, directory\n" +
            "root/subdir2/subsubdir1/file1, tmp_test_tar_file/s2_ss1_file1, "+Integer.valueOf(test_case[3])+", 0, application/octet-stream\n";

        // Set up the reader
        BufferedReader reader = new BufferedReader(new StringReader(config));

        // Construct the virtual directory structure
        VirtualDirectory vdir = new VirtualDirectory();
        vdir.addFromReader(reader);

        // Create a directory to store the test files
        Files.createDirectories(Paths.get("tmp_test_tar_file"));

        // Set up an array of bytes to write to the files
        int nmax = 0;
        for(int i=0; i<test_case.length; i+=1)
            if(nmax < test_case[i])nmax = test_case[i];
        byte[] data = new byte[nmax];
        for(int i=0; i<nmax; i+=1)
            data[i] =  (byte) (i % 128);

        // Create the files
        FileOutputStream s1_file1 = new FileOutputStream(new File("tmp_test_tar_file/s1_file1"));
        s1_file1.write(data, 0, test_case[0]);
        s1_file1.close();

        FileOutputStream s1_file2 = new FileOutputStream(new File("tmp_test_tar_file/s1_file2"));
        s1_file2.write(data, 0, test_case[1]);
        s1_file2.close();

        FileOutputStream s2_file1 = new FileOutputStream(new File("tmp_test_tar_file/s2_file1"));
        s2_file1.write(data, 0, test_case[2]);
        s2_file1.close();

        FileOutputStream s2_ss1_file1 = new FileOutputStream(new File("tmp_test_tar_file/s2_ss1_file1"));
        s2_ss1_file1.write(data, 0, test_case[3]);
        s2_ss1_file1.close();

        // Write the test files to a new tar file
        TarFile tarfile = new TarFile("", vdir, 128);
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
                    assertEquals(test_case[0], input_data.length);
                    break;
                case "root/subdir1/file2":
                    assertEquals(test_case[1], input_data.length);
                    break;
                case "root/subdir2/file1":
                    assertEquals(test_case[2], input_data.length);
                    break;
                case "root/subdir2/subsubdir1/file1":
                    assertEquals(test_case[3], input_data.length);
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
