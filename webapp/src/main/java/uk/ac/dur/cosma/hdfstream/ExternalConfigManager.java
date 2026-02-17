package uk.ac.dur.cosma.hdfstream;

import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.util.zip.GZIPInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.util.concurrent.locks.ReentrantLock;

import uk.ac.dur.cosma.virtual_directory.VirtualDirectory;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectoryException;


public class ExternalConfigManager extends ConfigManager {

    private String[] directory_configs = null;
    private volatile VirtualDirectory root = null;
    private final ReentrantLock lock = new ReentrantLock();

    /* Initialize the configuration. This implementation reads
     * external files (i.e. not in the .war) and can reload the config
     * without reloading the whole web app */
    public ExternalConfigManager(String[] directory_configs) throws VirtualDirectoryException {
        super();
        this.directory_configs = directory_configs;
        load();
    }

    private void load() throws VirtualDirectoryException {

        /* Fail if loading is already in progress */
        if (!lock.tryLock()) {
            throw new VirtualDirectoryException("Reload is already in progress");
        }

        /* Try to reload the config */
        try {
            VirtualDirectory vdir = new VirtualDirectory();
            for(String directory_config : directory_configs) {
                if(directory_config.endsWith(".gz")) {
                    /* Assume config file is gzipped if it has a .gz extension */
                    try (BufferedReader reader = new BufferedReader(new InputStreamReader(new GZIPInputStream(new FileInputStream(new File(directory_config)))))) {
                        vdir.addFromReader(reader);
                    } catch (Exception e) {
                        throw new VirtualDirectoryException("Failed to read gzipped virtual directory configuration: " + directory_config + " : " + e.getMessage());
                    }
                } else {
                    /* Uncompressed config file */
                    try (BufferedReader reader = new BufferedReader(new InputStreamReader(new FileInputStream(new File(directory_config))))) {
                        vdir.addFromReader(reader);
                    } catch (Exception e) {
                        throw new VirtualDirectoryException("Failed to read virtual directory configuration: " + directory_config + " : " + e.getMessage());
                    }
                }
            }
            /* If that worked, make the new config active */
            root = vdir;
        } finally {
            /* Whatever happened, always release the lock */
            lock.unlock();
        }
    }

    /* Reload the configuration */
    public void reload() throws VirtualDirectoryException {
        load();
    }

    /* Get the current virtual directory root */
    public VirtualDirectory getRoot() {
        return root;
    }
}
