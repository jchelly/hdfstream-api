import uk.ac.dur.cosma.virtual_directory.VirtualDirectory;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectoryException;
import uk.ac.dur.cosma.virtual_directory.VirtualPathInfo;
import uk.ac.dur.cosma.virtual_directory.CheckRole;

import java.io.StringReader;
import java.io.BufferedReader;
import java.util.HashSet;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

// Check that all file and directory properties are consistent with the input config
public class TestFileProperties {

    @Test
    public void main() throws VirtualDirectoryException, NumberFormatException {

        // Config 'file' to read. Columns are:
        // Real path, virtual path, size, last modified, media type, [role1;role2;...]
        String config =
            "root,                          no_such_file1, 1, 10, directory\n" +
            "root/subdir1,                  no_such_file2, 2, 20, directory\n" +
            "root/subdir1/file1,            no_such_file3, 3, 30, application/octet-stream\n" +
            "root/subdir1/file2,            no_such_file4, 4, 40, application/octet-stream\n" +
            "root/subdir2,                  no_such_file5, 5, 50, directory, role1\n" +
            "root/subdir2/file1,            no_such_file6, 6, 60, application/octet-stream\n" +
            "root/subdir2/subsubdir1,       no_such_file7, 7, 70, directory\n" +
            "root/subdir2/subsubdir1/file1, no_such_file8, 8, 80, application/octet-stream\n";

        // Set up the reader
        BufferedReader reader = new BufferedReader(new StringReader(config));

        // Construct the virtual directory structure
        VirtualDirectory vdir = new VirtualDirectory();
        vdir.addFromReader(reader);

        // Iterate over lines in the input configuration
        for(String line : config.split("\n")) {

            // Split this line into fields
            String fields[] = line.split(",");

            // Extract fields
            String virtual_path = fields[0].trim();
            String filesystem_path = fields[1].trim();
            long length = Long.valueOf(fields[2].trim());
            long last_modified = Long.valueOf(fields[3].trim());
            String media_type = fields[4].trim();
            String role_list = null;
            if(fields.length >= 6)role_list = fields[5].trim();
            HashSet<String> role_set = new HashSet<String>();
            if(role_list != null) {
                for(String role_name: role_list.split(";")) {
                    role_set.add(role_name);
                }
            }

            // Get the file and/or directory objects
            VirtualPathInfo path_info = vdir.resolvePath(virtual_path, (name) -> true);

            // Which fields we validate depends on whether this is a file or directory
            if(media_type.equals("directory")) {
                // This should be a directory
                assertNotEquals(null, path_info.directory);
                assertEquals(null, path_info.file);
                // Check roles associated with the directory:
                // The directory may have inherited extra roles from parents
                // but all roles in the config file must be present.
                for(String role_name : role_set) {
                    assertTrue(path_info.directory.required_roles.contains(role_name));
                }

            } else {
                // This should be a file
                assertNotNull(path_info.directory);
                assertNotNull(path_info.file);
                // Check file properties
                assertEquals("/"+virtual_path, path_info.file.virtual_path); // addFromReader adds leading slash
                assertEquals(filesystem_path, path_info.file.filesystem_path);
                assertEquals(length, path_info.file.length);
                assertEquals(last_modified*1000, path_info.file.last_modified); // Input is in secs but java uses millisecs
                assertEquals(media_type, path_info.file.media_type);
            }
        }
    }
}
