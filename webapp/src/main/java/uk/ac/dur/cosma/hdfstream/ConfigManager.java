package uk.ac.dur.cosma.hdfstream;

import uk.ac.dur.cosma.virtual_directory.VirtualDirectory;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectoryException;


public abstract class ConfigManager {

    /* Initialize the configuration */
    public ConfigManager() {}

    /* Get the current virtual directory root */
    public abstract VirtualDirectory getRoot();
}
