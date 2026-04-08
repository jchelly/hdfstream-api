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

    protected static UnitTestServer server = null;
    protected static UnitTestClient client = null;

    // Default config file to use
    protected static String default_config = "../../data/tests/basic/config.csv";

    public static void startServer(String config) throws Exception {

        if(server != null)stopServer();

        // Locate file with the virtual directory config for the test data
        String build_dir = System.getProperty("cmake.build.dir");
        assertNotNull(build_dir);
        String config_file = build_dir + "/" + config;

        // Start the server with default cache params and no access restrictions
        server = new UnitTestServer(config_file);

        // Set up the default client
        client = server.newClient();
    }

    public static void startServer(String config, long max_response_cache_size,
                                   int max_cached_response_size) throws Exception {

        if(server != null)stopServer();

        // Locate file with the virtual directory config for the test data
        String build_dir = System.getProperty("cmake.build.dir");
        assertNotNull(build_dir);
        String config_file = build_dir + "/" + config;

        // Start the server with specified cache parameters
        server = new UnitTestServer(config_file, null, null, null,
                                    max_response_cache_size,
                                    max_cached_response_size);

        // Set up the default client
        client = server.newClient();
    }

    public static void stopServer() throws Exception {
        if(server != null) {
            server.stop();
            server = null;
            client = null;
        }
    }

    @BeforeAll
    public static void setUp() throws Exception {
        startServer(default_config);
    }

    @AfterAll
    public static void tearDown() throws Exception {
        stopServer();
    }
}
