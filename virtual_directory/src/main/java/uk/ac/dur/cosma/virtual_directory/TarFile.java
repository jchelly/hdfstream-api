package uk.ac.dur.cosma.virtual_directory;

import java.io.*;
import java.util.LinkedHashMap;
import java.util.Map;

import org.apache.commons.compress.archivers.tar.*;
import org.apache.commons.io.output.CountingOutputStream;
import org.apache.commons.io.output.NullOutputStream;


public class TarFile {

    private LinkedHashMap<String, VirtualFile> files;
    private int buffer_size;

    protected static final int BLOCK_SIZE  = 20*512;
    protected static final int RECORD_SIZE = 512; /* RECORD_SIZE must be 512 */

    private int nr_files;
    public long nr_bytes;
    private long last_modified;
    private long file_offset[];
    private String prefix; // Prefix to add to paths stored in the tar file

    private static long getFileHeaderSize(String filename, long last_modified, long file_size) {

        // Create a tar output stream that counts and discards any bytes written
        NullOutputStream nout = NullOutputStream.INSTANCE;
        CountingOutputStream cout = new CountingOutputStream(nout);
        TarArchiveOutputStream tarstream = new TarArchiveOutputStream(cout, BLOCK_SIZE);
        tarstream.setBigNumberMode(TarArchiveOutputStream.BIGNUMBER_POSIX);
        tarstream.setLongFileMode(TarArchiveOutputStream.LONGFILE_POSIX);

        // Create a tar entry for this file
        TarArchiveEntry fileentry = new TarArchiveEntry(filename);
        fileentry.setSize(file_size);
        fileentry.setModTime(last_modified);

        // Write the entry
        try {
            tarstream.putArchiveEntry(fileentry);
            tarstream.flush();
        } catch (IOException e) {
            // Probably ought to throw an unchecked exception if we get here...
            throw new RuntimeException("Failed to determine tar entry header size");
        }

        // Return the size in bytes
        return cout.getByteCount();
    }

    // Create a tar file only including only directories where in_role returns true for all roles
    // required to access the directory
    public TarFile(String prefix, VirtualDirectory vdir, int buffer_size, CheckRole in_role) {
        initTarFile(prefix, vdir, buffer_size, in_role);
    }

    // Create a tar file including all subdirectories regardless of role membership
    public TarFile(String prefix, VirtualDirectory vdir, int buffer_size) {
        initTarFile(prefix, vdir, buffer_size, (in) -> true);
    }

    public void initTarFile(String prefix, VirtualDirectory vdir, int buffer_size, CheckRole in_role) {

        // Store the buffer size for copying data
        this.buffer_size = buffer_size;

        // Store prefix for paths within the tar file
        this.prefix = prefix;

        // Find all of the virtual files to add to the tar file
        files = vdir.getAllFiles(in_role);

        // Compute the byte offset to each file in the output
        nr_bytes = 0;
        nr_files = files.size();
        long file_offset[] = new long[nr_files];
        int file_nr = 0;
        last_modified = 0;
        for(Map.Entry<String, VirtualFile> entry : files.entrySet()) {
            String virtual_path = entry.getKey();
            VirtualFile file = entry.getValue();
            file_offset[file_nr] = nr_bytes;
            // Find size and modification time of this file
            long file_size = file.getLength();
            long file_last_modified = file.getLastModified();
            if(file_last_modified > last_modified)
                last_modified = file_last_modified;
            // In the tar file the size is rounded up to next multiple of RECORD_SIZE
	    if ((file_size % RECORD_SIZE) != 0)file_size = ((file_size/RECORD_SIZE) * RECORD_SIZE) + RECORD_SIZE;
            // Determine size of the header. May be more than RECORD_SIZE if the filename is long.
            long header_size = getFileHeaderSize(prefix+virtual_path, file_last_modified, file_size);
            // Accumulate total size of the tar file
	    nr_bytes += file_size + header_size;
            // Advance to the next file
            file_nr += 1;
        }

        // Account for two empty records at end of file
	nr_bytes += 2*RECORD_SIZE;

	// Round up total size to an integer number of blocks
	if(nr_bytes % BLOCK_SIZE != 0)
	    nr_bytes = ((nr_bytes / BLOCK_SIZE) * BLOCK_SIZE) + BLOCK_SIZE;
    }

    public void write(OutputStream out) throws IOException {

        // Sanity check: number of bytes written should equal nr_bytes
        CountingOutputStream cout = new CountingOutputStream(out);

        try (TarArchiveOutputStream tarstream = new TarArchiveOutputStream(cout, BLOCK_SIZE);) {

            tarstream.setBigNumberMode(TarArchiveOutputStream.BIGNUMBER_POSIX);
            tarstream.setLongFileMode(TarArchiveOutputStream.LONGFILE_POSIX);

            // Loop over files to write out
            for(Map.Entry<String, VirtualFile> entry : files.entrySet()) {
                String virtual_path = entry.getKey();
                VirtualFile virtual_file = entry.getValue();

                // Open the actual file on the file system
                File real_file = new File(virtual_file.filesystem_path);

                // Create archive entry using the virtual filename
                TarArchiveEntry fileentry = new TarArchiveEntry(prefix+virtual_path);
                fileentry.setSize(virtual_file.getLength());
                fileentry.setModTime(virtual_file.getLastModified());
                tarstream.putArchiveEntry(fileentry);

                // Copy file data to output
                try (FileInputStream instream = new FileInputStream(real_file)) {
                    int nread;
                    byte[] bytes = new byte[buffer_size];
                    while ((nread = instream.read(bytes)) != -1) {
                        tarstream.write(bytes, 0, nread);
                    }
                }

                // Close the archive entry
                tarstream.closeArchiveEntry();
            }
            tarstream.finish();
            tarstream.flush();
            if(cout.getByteCount() != nr_bytes) {
                throw new RuntimeException("Unexpected number of bytes written");
            }
            tarstream.close();
        }
    }
}
