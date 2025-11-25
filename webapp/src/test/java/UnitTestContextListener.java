import javax.servlet.ServletContext;
import java.io.InputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.BufferedReader;
import java.io.IOException;

import uk.ac.dur.cosma.hdfstream.HDFStreamContextListener;
import uk.ac.dur.cosma.libhdfstream.HDFStream;

/*
  In the full web app we read the virtual directory config as a resource from /WEB-INF.

  When running unit tests we don't deploy the full web app directory structure, so
  here we override a method to make the servlet read its config by opening the file
  directly instead.

  We also override the function that starts the process pool to use executables from
  the build directory rather than the install location.
*/
public class UnitTestContextListener extends HDFStreamContextListener {

    @Override
    protected InputStream getResource(ServletContext context, String name) throws IOException {
        return new FileInputStream(new File(name));
    }

    @Override
    protected HDFStream startHDFStream(int nr_processes, int max_open_files, int max_open_datasets,
                                       int file_cache_check_interval, int file_cache_expiry_interval) {

        // Get the path to the reader executable in the build directory
        String build_dir = System.getProperty("cmake.build.dir");
        if(build_dir == null)throw new RuntimeException("CMake build directory (property cmake.build.dir) is not set!");
        String executable = build_dir + "/../../src/reader/hdfstream_reader";

        // Start the process pool, specifying the path to the reader executable
        return new HDFStream(nr_processes, executable, max_open_files, max_open_datasets,
                             file_cache_check_interval, file_cache_expiry_interval);
    }
}

