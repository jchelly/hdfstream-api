import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.AfterAll;
import static org.junit.jupiter.api.Assertions.*;

import java.nio.file.Files;
import java.nio.file.Paths;

/*
  Base class with setup and teardown for unit tests.
  Tests should extend this.
*/
public abstract class BasicUnitTest {

    protected static UnitTestServer server;
    protected static UnitTestClient client;

    // Default config file to use
    protected static String default_config = "../../data/tests/basic/config.csv";

    public static void startServer(String config) throws Exception {

        // Locate file with the virtual directory config for the test data
        String build_dir = System.getProperty("cmake.build.dir");
        assertNotNull(build_dir);
        String config_file = build_dir + "/" + config;

        // Start the server
        server = new UnitTestServer(config_file);

        // Set up the default client
        client = server.newClient();
    }

    @BeforeAll
    public static void setUp() throws Exception {
        startServer(default_config);
    }

    @AfterAll
    public static void tearDown() throws Exception {
        server.stop();
    }
}
