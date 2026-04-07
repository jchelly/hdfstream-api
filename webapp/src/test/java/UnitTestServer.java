import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.AfterAll;
import static org.junit.jupiter.api.Assertions.*;

import uk.ac.dur.cosma.hdfstream.*;

import org.apache.catalina.Context;
import org.apache.catalina.startup.Tomcat;
import org.apache.tomcat.util.descriptor.web.LoginConfig;
import org.apache.tomcat.util.descriptor.web.SecurityConstraint;
import org.apache.tomcat.util.descriptor.web.SecurityCollection;
import org.apache.catalina.authenticator.BasicAuthenticator;
import org.apache.commons.io.FileUtils;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.impl.client.HttpClients;

import javax.servlet.http.*;
import java.io.File;
import java.nio.file.Files;
import java.io.IOException;
import java.util.*;

/*
   Class which starts up the HDFStream servlet in an embedded Tomcat instance for testing

   Also includes methods to request data from the server.
*/
public class UnitTestServer {

    private Tomcat tomcat;
    private File tempDir;
    private CloseableHttpClient client;

    public String baseURL;

    public UnitTestServer(String config_file) throws Exception {
        // Initialize without defining a Realm, users or constraints by default
        this(config_file, null, null, null);
    }

    public UnitTestServer(String config_file, Map<String, String> userPasswords,
                          Map<String, String[]> userRoles,
                          Map<String, String[]> securityConstraints) throws Exception {

        // Create a tomcat instance
        tomcat = new Tomcat();
        tomcat.setPort(0); // auto-assign port

        // Put tomcat's workspace in a temporary dir
        tempDir = Files.createTempDirectory("test-hdfstream-servlet").toFile();
        tomcat.setBaseDir(tempDir.getAbsolutePath());

        // Create the context
        Context context = tomcat.addContext("/hdfstream", System.getProperty("java.io.tmpdir"));

        // Set up the authentication realm and users, if specified
        if(userPasswords != null) {
            assertNotNull(userRoles);
            UnitTestRealm realm = new UnitTestRealm();
            for (Map.Entry<String, String> entry : userPasswords.entrySet()) {
                String username = entry.getKey();
                String password = entry.getValue();
                String[] roles = userRoles.get(username);
                assertNotNull(username);
                assertNotNull(password);
                assertNotNull(roles);
                realm.addUser(username, password, roles);
            }
            tomcat.getEngine().setRealm(realm);
        }

        // Set parameters which we would normally get from web.xml
        context.addParameter("nr_processes", "4");
        context.addParameter("max_open_files", "10");
        context.addParameter("max_open_datasets", "10");
        context.addParameter("directory_config", config_file);
        context.addParameter("buffer_size", "102400");
        context.addParameter("max_hdf5_name_length", "8192");
        context.addParameter("file_cache_check_interval", "10");
        context.addParameter("file_cache_expiry_interval", "60");
        context.addParameter("max_requests_per_user", "4");
        context.addParameter("external_config", "1");
        context.addParameter("max_cache_size", "10240");
        context.addParameter("max_cached_response_size", "1024");

        // Register listener that starts the process pool
        context.addApplicationListener(UnitTestContextListener.class.getName());

        // Register the servlets
        Tomcat.addServlet(context, "HDFStreamServlet", new HDFStreamServlet());
        context.addServletMappingDecoded("/msgpack/*", "HDFStreamServlet");
	Tomcat.addServlet(context, "TarFileServlet", new TarFileServlet());
        context.addServletMappingDecoded("/download/*", "TarFileServlet");
	Tomcat.addServlet(context, "StatusServlet", new StatusServlet());
        context.addServletMappingDecoded("/status", "StatusServlet");

        // Set login config, if users were specified
        if(userPasswords != null) {
            LoginConfig loginConfig = new LoginConfig();
            loginConfig.setAuthMethod("BASIC");
            loginConfig.setRealmName("UnitTestRealm");
            context.setLoginConfig(loginConfig);
        }

        // Set up any security constraints that have been requested:
        // Keys are role names and values are virtual paths, without any
        // leading slash.
        if(securityConstraints != null ) {
            for (Map.Entry<String, String[]> entry : securityConstraints.entrySet()) {
                String role = entry.getKey();
                String[] patterns = entry.getValue();
                context.addSecurityRole(role);
                // Create a new security constraint for this role
                SecurityConstraint constraint = new SecurityConstraint();
                constraint.setAuthConstraint(true);
                constraint.addAuthRole(role);
                // Add the specified patterns to the constraint
                SecurityCollection collection = new SecurityCollection();
                for(String pattern : patterns) {
                    collection.addPattern("/download/"+pattern);
                    collection.addPattern("/msgpack/"+pattern);
                }
                constraint.addCollection(collection);
                context.addConstraint(constraint);
            }
        }

        // Enable basic auth
        context.getPipeline().addValve(new BasicAuthenticator());

        // Start the server
        tomcat.start();

        // Get the URL of the server
        int port = tomcat.getConnector().getLocalPort();
        baseURL = "http://localhost:" + port + "/hdfstream";

        // Create the http client to make requests to the server
        client = HttpClients.createDefault();
    }

    public UnitTestClient newClient() throws Exception {
        return new UnitTestClient(baseURL);
    }

    public UnitTestClient newClient(String username, String password) throws Exception {
        return new UnitTestClient(baseURL, username, password);
    }

    public void stop() throws Exception {

        // Close the http client
        client.close();

        // Stop the server
        tomcat.stop();
        tomcat.destroy();

        // Remove tomcat's workspace directory
        FileUtils.deleteDirectory(tempDir);
    }
}
