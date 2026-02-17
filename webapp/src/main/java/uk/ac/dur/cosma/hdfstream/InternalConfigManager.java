package uk.ac.dur.cosma.hdfstream;

import javax.servlet.ServletContext;
import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.util.zip.GZIPInputStream;

import uk.ac.dur.cosma.virtual_directory.VirtualDirectory;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectoryException;


public class InternalConfigManager extends ConfigManager {

    private VirtualDirectory root = null;

    /*
      Initialize the configuration. This implementation assumes the
      config is in the .war file and cannot be updated while the
      service is running.
    */
    public InternalConfigManager(ServletContext context, String[] directory_configs) throws VirtualDirectoryException {
        super();
        VirtualDirectory vdir = new VirtualDirectory();
        for(String directory_config : directory_configs) {
            if(directory_config.endsWith(".gz")) {
                /* Assume config file is gzipped if it has a .gz extension */
                try (BufferedReader reader = new BufferedReader(new InputStreamReader(new GZIPInputStream(context.getResourceAsStream(directory_config))))) {
                    vdir.addFromReader(reader);
                } catch (Exception e) {
                    throw new VirtualDirectoryException("Failed to read gzipped virtual directory configuration: " + directory_config + " : " + e.getMessage());
                }
            } else {
                /* Uncompressed config file */
                try (BufferedReader reader = new BufferedReader(new InputStreamReader(context.getResourceAsStream(directory_config)))) {
                    vdir.addFromReader(reader);
                } catch (Exception e) {
                    throw new VirtualDirectoryException("Failed to read virtual directory configuration: " + directory_config + " : " + e.getMessage());
                }
            }
        }
        /* If that worked, make the new config active */
        root = vdir;
    }

    /* Reloading is not possible */
    public void reload() throws VirtualDirectoryException {
        throw new VirtualDirectoryException("Reloading is not supported!");
    }

    /* Get the current virtual directory root */
    public VirtualDirectory getRoot() {
        return root;
    }
}
