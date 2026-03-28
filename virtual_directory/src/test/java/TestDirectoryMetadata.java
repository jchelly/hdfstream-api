import static org.junit.jupiter.api.Assertions.*;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

import org.msgpack.core.MessagePack;
import org.msgpack.core.MessagePacker;
import org.msgpack.core.MessageUnpacker;
import org.msgpack.core.MessageBufferPacker;

import uk.ac.dur.cosma.virtual_directory.VirtualDirectory;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectoryException;
import uk.ac.dur.cosma.virtual_directory.DirectoryMetadata;


public class TestDirectoryMetadata {

    @TempDir
    public Path tempDir;

    @Test
    public void testUnpackMetadata() throws Exception {

	// Generate the test data
	MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packMapHeader(2);
	// Directory description entry
	packer.packString("description");
	packer.packString("Directory description");
	// File labels entry
	packer.packString("file_labels");
        packer.packMapHeader(3);
	packer.packString("file0");
	packer.packString("file_description0");
	packer.packString("file1");
	packer.packString("file_description1");
	packer.packString("file2");
	packer.packString("file_description2");
	packer.close();
        byte[] bytes = packer.toByteArray();

	// Write to a temp file
        Path file = Files.createTempFile(tempDir, "msgpack-", ".bin");
        Files.write(file, bytes);

	// Try to read it back with the DirectoryMetadata class
	DirectoryMetadata md = new DirectoryMetadata(file.toString());
	assertNotNull(md);
	assertEquals("Directory description", md.description);
	assertEquals(3, md.file_labels.size());
	assertEquals("file_description0", md.file_labels.get("file0"));
	assertEquals("file_description1", md.file_labels.get("file1"));
	assertEquals("file_description2", md.file_labels.get("file2"));
    }

    @Test
    public void testNoLabels() throws Exception {

	// Generate the test data
	MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packMapHeader(1);
	// Directory description entry
	packer.packString("description");
	packer.packString("Directory description");
	packer.close();
        byte[] bytes = packer.toByteArray();

	// Write to a temp file
        Path file = Files.createTempFile(tempDir, "msgpack-", ".bin");
        Files.write(file, bytes);

	// Try to read it back with the DirectoryMetadata class
	DirectoryMetadata md = new DirectoryMetadata(file.toString());
	assertNotNull(md);
	assertEquals("Directory description", md.description);
	assertEquals(null, md.file_labels);
    }

    @Test
    public void testNoDescription() throws Exception {

	// Generate the test data
	MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packMapHeader(1);
	// File labels entry
	packer.packString("file_labels");
        packer.packMapHeader(3);
	packer.packString("file0");
	packer.packString("file_description0");
	packer.packString("file1");
	packer.packString("file_description1");
	packer.packString("file2");
	packer.packString("file_description2");
	packer.close();
        byte[] bytes = packer.toByteArray();

	// Write to a temp file
        Path file = Files.createTempFile(tempDir, "msgpack-", ".bin");
        Files.write(file, bytes);

	// Try to read it back with the DirectoryMetadata class
	DirectoryMetadata md = new DirectoryMetadata(file.toString());
	assertNotNull(md);
	assertEquals(null, md.description);
	assertEquals(3, md.file_labels.size());
	assertEquals("file_description0", md.file_labels.get("file0"));
	assertEquals("file_description1", md.file_labels.get("file1"));
	assertEquals("file_description2", md.file_labels.get("file2"));
    }

    @Test
    public void testWrongFile() throws Exception {
	assertThrows(IOException.class, () -> {
		DirectoryMetadata md = new DirectoryMetadata("no_such_file");
	    });
    }

    @Test
    public void testInvalidData() throws Exception {

	// Generate the test data
	MessageBufferPacker packer = MessagePack.newDefaultBufferPacker();
        packer.packLong(1);
	packer.close();
        byte[] bytes = packer.toByteArray();

	// Write to a temp file
        Path file = Files.createTempFile(tempDir, "msgpack-", ".bin");
        Files.write(file, bytes);

	// Try to read it back with the DirectoryMetadata class
	assertThrows(IOException.class, () -> {
		DirectoryMetadata md = new DirectoryMetadata(file.toString());
	    });
    }
}
