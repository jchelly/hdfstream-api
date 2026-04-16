package uk.ac.dur.cosma.hdfstream;

import javax.servlet.ServletContext;
import javax.servlet.ServletContextEvent;
import javax.servlet.ServletContextListener;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectory;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectoryException;

import uk.ac.dur.cosma.libhdfstream.*;

public class HDFStreamContextListener implements ServletContextListener{

    ServletContext context;

    // Split a string on ';', trimming any whitespace
    public static String[] split(String s) {
        String[] fields = s.split(";");
        int n = fields.length;
        String[] result = new String[n];
        for(int i=0; i<n; i+=1) {
            result[i] = fields[i].trim();
        }
        return result;
    }

    // During unit tests this is overriden to use the hdfstream_reader executable from the build directory.
    // When fully installed and running the executable is picked up from the library install path.
    protected HDFStream startHDFStream(int nr_processes, int max_open_files, int max_open_datasets,
                                       int file_cache_check_interval, int file_cache_expiry_interval) {
        return new HDFStream(nr_processes, max_open_files, max_open_datasets, file_cache_check_interval, file_cache_expiry_interval);
    }

    public void contextInitialized(ServletContextEvent contextEvent) {

        /* Store reference to the servlet context */
	context = contextEvent.getServletContext();

        /* Get cache parameters from web.xml */
        int nr_processes = Integer.valueOf(context.getInitParameter("nr_processes"));
        int max_open_files = Integer.valueOf(context.getInitParameter("max_open_files"));
        int max_open_datasets = Integer.valueOf(context.getInitParameter("max_open_datasets"));
        String[] directory_configs = split(context.getInitParameter("directory_config"));
        int buffer_size = Integer.valueOf(context.getInitParameter("buffer_size"));
        int max_hdf5_name_length = Integer.valueOf(context.getInitParameter("max_hdf5_name_length"));
        int file_cache_check_interval = Integer.valueOf(context.getInitParameter("file_cache_check_interval"));
        int file_cache_expiry_interval = Integer.valueOf(context.getInitParameter("file_cache_expiry_interval"));
        int max_requests_per_user = Integer.valueOf(context.getInitParameter("max_requests_per_user"));
        int external_config = Integer.valueOf(context.getInitParameter("external_config"));
        int max_cached_response_size = Integer.valueOf(context.getInitParameter("max_cached_response_size"));
        long max_response_cache_size = Long.valueOf(context.getInitParameter("max_response_cache_size"));

        /* Store HDF5 name length limit */
        context.setAttribute("max_hdf5_name_length", max_hdf5_name_length);

        /* Store number of processes */
        context.setAttribute("nr_processes", nr_processes);

        /* Initialize hdfstream library on startup. Starts the reader processes. */
        HDFStream hs = startHDFStream(nr_processes, max_open_files, max_open_datasets, file_cache_check_interval, file_cache_expiry_interval);
        if(hs == null)throw new RuntimeException("HDFStream process pool failed to start!");
	context.setAttribute("hdfstream", hs);
	context.setAttribute("buffer_size", buffer_size);

        /*
          Read virtual directory configuration. directory_configs may
          contain multiple filenames separated by semicolons, in which
          case we read all of the specified files.
        */
        ConfigManager config;
        try {
            if(external_config != 0) {
                config = new ExternalConfigManager(directory_configs);
            } else {
                config = new InternalConfigManager(context, directory_configs);
            }
        } catch (VirtualDirectoryException e) {
            config = null;
        }
        context.setAttribute("config", config);

        /* Set up object to limit concurrent requests per user */
        ConcurrentRequestCount crc = new ConcurrentRequestCount(max_requests_per_user);
	context.setAttribute("concurrent_request_count", crc);

        /* Create the request cache */
        CacheInfo ci = new CacheInfo(max_cached_response_size, max_response_cache_size);
	context.setAttribute("cache_info", ci);
    }

    public void contextDestroyed(ServletContextEvent contextEvent) {
        /*
          Stop reader processes on shutdown.
        */
        HDFStream hs = (HDFStream) context.getAttribute("hdfstream");
        if(hs != null)hs.free();
    }
}
