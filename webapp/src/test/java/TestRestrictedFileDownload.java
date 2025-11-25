import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.AfterAll;
import static org.junit.jupiter.api.Assertions.*;

import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.HashMap;
import java.util.List;

/*
  Test downloading a file from the server with security restrictions

  Files which the user does not have access to should not appear in directory
  listings or tar file downloads. Attempting to download or open files with
  no access should generate a 401 if not logged in and a 403 if logged in but
  not a member of the required role.
*/
public class TestRestrictedFileDownload extends PermissionsUnitTest {

    @Test
    public void testUnrestrictedFile() throws Exception {
        // This text file has no security restrictions
	byte[] response_data = client_not_authenticated.requestFile("/tests/permissions/unrestricted/data.txt", 200);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testUnrestrictedHDF5File(boolean post) throws Exception {
        // This HDF5 file has no security restrictions
	HDF5Object object = client_not_authenticated.requestObject("/tests/permissions/unrestricted/data.hdf5",
                                                                   "scalar_string", "1", "1024", 200, post);
        assertNotNull(object);
        String dataset_contents = object.data.getElementAsString(0);
        assertEquals("unrestricted file data", dataset_contents);
    }

    @Test
    public void testRestrictedFile() throws Exception {
        // This file should not be downloadable unless we're a member of the right role
        byte[] data;
        data = client_not_authenticated.requestFile("/tests/permissions/restricted/data.txt", 401);
        data = client_in_role.requestFile("/tests/permissions/restricted/data.txt", 200);
        data = client_in_no_role.requestFile("/tests/permissions/restricted/data.txt", 403);
        data = client_in_other_role.requestFile("/tests/permissions/restricted/data.txt", 403);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testRestrictedHDF5File(boolean post) throws Exception {
        // This HDF5 file should also not be readable unless we're a member of the right role
	HDF5Object object;
        object = client_not_authenticated.requestObject("/tests/permissions/restricted/data.hdf5", "scalar_string", "1", "1024", 401, post);
        object = client_in_role.requestObject("/tests/permissions/restricted/data.hdf5", "scalar_string", "1", "1024", 200, post);
        object = client_in_no_role.requestObject("/tests/permissions/restricted/data.hdf5", "scalar_string", "1", "1024", 403, post);
        object = client_in_other_role.requestObject("/tests/permissions/restricted/data.hdf5", "scalar_string", "1", "1024", 403, post);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testRestrictedDirectoryNotListed(boolean post) throws Exception {
        // Check that the restricted directory does not appear in a directory listing if we're not logged in
        DirectoryEntry entry;
        entry = client_not_authenticated.requestDirectoryEntry("/tests/permissions", "10", 200, post);
        assertNotNull(entry);
        assertEquals(1, entry.directories.size());
        assertTrue(entry.directories.containsKey("unrestricted"));

        // Or if we don't belong to any role
        entry = client_in_no_role.requestDirectoryEntry("/tests/permissions", "10", 200, post);
        assertNotNull(entry);
        assertEquals(1, entry.directories.size());
        assertTrue(entry.directories.containsKey("unrestricted"));

        // Or if we belong to the wrong role
        entry = client_in_other_role.requestDirectoryEntry("/tests/permissions", "10", 200, post);
        assertNotNull(entry);
        assertEquals(1, entry.directories.size());
        assertTrue(entry.directories.containsKey("unrestricted"));
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testRestrictedDirectoryNotFound(boolean post) throws Exception {
        // We shouldn't be able to list a restricted directory without the right role membership
        DirectoryEntry entry;
        entry = client_not_authenticated.requestDirectoryEntry("/tests/permissions/restricted", "10", 401, post);
        entry = client_in_role.requestDirectoryEntry("/tests/permissions/restricted", "10", 200, post);
        entry = client_in_no_role.requestDirectoryEntry("/tests/permissions/restricted", "10", 403, post);
        entry = client_in_other_role.requestDirectoryEntry("/tests/permissions/restricted", "10", 403, post);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testUnrestrictedFileListed(boolean post) throws Exception {
        // We should be able to get metadata for an unrestricted file without logging in
        DirectoryEntry entry = client_not_authenticated.requestDirectoryEntry("/tests/permissions/unrestricted/data.txt", "10", 200, post);
        assertNotNull(entry);
    }

    @ParameterizedTest
    @ValueSource(booleans = {false, true})
    public void testRestrictedFileNotListed(boolean post) throws Exception {
        // We should not be able to get metadata for a restricted file unless we belong to the right role
        DirectoryEntry entry;
        entry = client_not_authenticated.requestDirectoryEntry("/tests/permissions/restricted/data.txt", "10", 401, post);
        entry = client_in_role.requestDirectoryEntry("/tests/permissions/restricted/data.txt", "10", 200, post);
        entry = client_in_no_role.requestDirectoryEntry("/tests/permissions/restricted/data.txt", "10", 403, post);
        entry = client_in_other_role.requestDirectoryEntry("/tests/permissions/restricted/data.txt", "10", 403, post);
    }

    @Test
    public void testRestrictedTarFileContents() throws Exception {

        // Tar file downloads should not include restricted files if not a role member
        HashMap<String, byte[]> files;
        files = client_not_authenticated.requestTarFile("/tests/permissions", 200);
        assertEquals(2, files.size());
        assertTrue(files.containsKey("tests/permissions/unrestricted/data.txt"));
        assertTrue(files.containsKey("tests/permissions/unrestricted/data.hdf5"));
        files = client_in_no_role.requestTarFile("/tests/permissions", 200);
        assertEquals(2, files.size());
        assertTrue(files.containsKey("tests/permissions/unrestricted/data.txt"));
        assertTrue(files.containsKey("tests/permissions/unrestricted/data.hdf5"));
        files = client_in_other_role.requestTarFile("/tests/permissions", 200);
        assertEquals(2, files.size());
        assertTrue(files.containsKey("tests/permissions/unrestricted/data.txt"));
        assertTrue(files.containsKey("tests/permissions/unrestricted/data.hdf5"));
    }
}
