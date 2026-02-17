package uk.ac.dur.cosma.hdfstream;

import uk.ac.dur.cosma.virtual_directory.VirtualDirectory;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectoryException;


public abstract class ConfigManager {

    /* Initialize the configuration */
    public ConfigManager() {}

    /* Reload the configuration, if possible */
    public abstract void reload() throws VirtualDirectoryException;

    /* Get the current virtual directory root */
    public abstract VirtualDirectory getRoot();
}
