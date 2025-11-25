import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import java.util.Arrays;

import uk.ac.dur.cosma.libhdfstream.*;

// Check that we can start up the HDFStream C API (i.e. native libraries are found)
public class TestHDFStreamJNI {

    @Test
    public void main() {

        /* Set API parameters */
        int nr_processes = 1;
        int max_open_files = 5;
        int max_open_datasets = 10;
        int file_cache_check_interval = 10;
        int file_cache_expiry_interval = 60;

        /* Start the process pool */
        HDFStream hs = new HDFStream(nr_processes, max_open_files, max_open_datasets, file_cache_check_interval, file_cache_expiry_interval);
        assertNotNull(hs);

        /* Shut down */
        hs.free();
    }
}
