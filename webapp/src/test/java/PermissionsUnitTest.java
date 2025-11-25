import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.AfterAll;
import static org.junit.jupiter.api.Assertions.*;

import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.HashMap;

/*
  Base class with setup and teardown for unit tests.
  Tests should extend this.

  This version uses a config file with a security role on one of the
  directories and sets up clients with usernames and passwords.
*/
public abstract class PermissionsUnitTest {

    protected static UnitTestServer server;
    protected static UnitTestClient client_not_authenticated;
    protected static UnitTestClient client_in_role;
    protected static UnitTestClient client_in_no_role;
    protected static UnitTestClient client_in_other_role;

    // Default config file to use
    protected static String config = "../../data/tests/permissions/config.csv";

    @BeforeAll
    public static void setUp() throws Exception {

        // Locate file with the virtual directory config for the test data
        String build_dir = System.getProperty("cmake.build.dir");
        assertNotNull(build_dir);
        String config_file = build_dir + "/" + config;

        // Define users
        HashMap<String,String> userPasswords = new HashMap<String,String>();
        HashMap<String,String[]> userRoles = new HashMap<String,String[]>();
        // user001 belongs to the restricted role
        userPasswords.put("user001", "password001");
        userRoles.put("user001", new String[] {"restricted-access"});
        // user002 belongs to no role
        userPasswords.put("user002", "password002");
        userRoles.put("user002", new String[] {});
        // user003 belongs to another role
        userPasswords.put("user003", "password003");
        userRoles.put("user003", new String[] {"other-restricted-access"});

        // Define security constraints. Note that to restrict a directory we
        // need separate patterns for the  directory and its contents. These
        // patterns need to match the constraints in the virtual directory
        // config to ensure that authentication is triggered as needed in the
        // client.
        HashMap<String,String[]> securityConstraints = new HashMap<String,String[]>();
        securityConstraints.put("restricted-access", new String[] {"tests/permissions/restricted",
                                                                   "tests/permissions/restricted/*"});

        // Start the server
        server = new UnitTestServer(config_file, userPasswords, userRoles, securityConstraints);

        // Set up clients
        client_not_authenticated = server.newClient();
        client_in_role = server.newClient("user001", "password001");
        client_in_no_role = server.newClient("user002", "password002");
        client_in_other_role = server.newClient("user003", "password003");
    }

    @AfterAll
    public static void tearDown() throws Exception {
        server.stop();
    }
}
