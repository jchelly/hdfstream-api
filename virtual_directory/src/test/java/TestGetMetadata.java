import uk.ac.dur.cosma.virtual_directory.VirtualDirectory;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectoryException;
import uk.ac.dur.cosma.virtual_directory.VirtualPathInfo;
import uk.ac.dur.cosma.virtual_directory.DirectoryMetadata;

import java.io.StringReader;
import java.io.BufferedReader;
import java.nio.file.Path;
import java.nio.file.Files;
import java.io.IOException;

import org.msgpack.core.MessagePack;
import org.msgpack.core.MessagePacker;
import org.msgpack.core.MessageUnpacker;
import org.msgpack.core.MessageBufferPacker;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import org.junit.jupiter.api.io.TempDir;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.CsvSource;

// Construct a VirtualDirectory from a BufferedReader and check we get the expected paths
public class TestGetMetadata {

    @TempDir
    public Path tempDir;

    @ParameterizedTest
    @CsvSource({"true,  true",
                "true,  false",
                "false, true",
                })
    public void TestDecoding(boolean have_description, boolean have_labels) throws VirtualDirectoryException, IOException {

        int nr_entries = 0;
        if(have_description)nr_entries+=1;
        if(have_labels)nr_entries+=1;

	// Generate the test directory metadata data
	MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packMapHeader(nr_entries);
	// Directory description entry
        if(have_description) {
            packer.packString("description");
            packer.packString("Directory description");
        }
	// File labels entry
        if(have_labels) {
            packer.packString("labels");
            packer.packMapHeader(1);
            packer.packString("file0");
            packer.packString("file_description0");
        }
	packer.close();
        byte[] bytes = packer.toByteArray();

        // Write to a temp file
        Path metadata_file = Files.createTempFile(tempDir, "msgpack-", ".bin");
        Files.write(metadata_file, bytes);

        // Config 'file' to read. Columns are:
        // Real path, virtual path, size, last modified, media type
        String config =
            "root,, 0, 0, directory\n" +
            "root/subdir1," + metadata_file.toString() + ", 0, 0, directory\n" +
            "root/subdir1/file1, no_such_file, 0, 0, application/octet-stream\n";

        // Set up the reader
        BufferedReader reader = new BufferedReader(new StringReader(config));

        // Construct the virtual directory structure
        VirtualDirectory vdir = new VirtualDirectory();
        vdir.addFromReader(reader);

        // Root should be a directory with no metadata
        VirtualPathInfo root = vdir.resolvePath("root", (in) -> true);
        assertNull(root.file);
        assertNotNull(root.directory);
        assertNull(root.directory.getMetadata());

        // Subdir1 is a directory with one file
        VirtualPathInfo subdir1 = vdir.resolvePath("root/subdir1", (in) -> true);
        assertNull(subdir1.file);
        assertNotNull(subdir1.directory);
        assertEquals(1, subdir1.directory.getFiles((in) -> true).size());

        // Check metadata contents
        DirectoryMetadata md = subdir1.directory.getMetadata();
        assertNotNull(md);
        if(have_description) {
            assertEquals("Directory description", md.description);
        } else {
            assertNull(md.description);
        }
        if(have_labels) {
            assertEquals(1, md.labels.size());
            assertEquals("file_description0", md.labels.get("file0"));
        } else {
            assertNull(md.labels);
        }

        // Check files
        VirtualPathInfo subdir1_file1 = vdir.resolvePath("root/subdir1/file1", (in) -> true);
        assertNotNull(subdir1_file1.file);
        assertNotNull(subdir1_file1.directory);
        assertEquals("file1", subdir1_file1.basename);
    }
}
