package uk.ac.dur.cosma.hdfstream;

import java.lang.Long;
import java.lang.Integer;
import javax.servlet.Servlet;
import javax.servlet.ServletException;
import javax.servlet.ServletOutputStream;
import javax.servlet.ServletContext;
import javax.servlet.RequestDispatcher;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.security.Principal;
import java.io.File;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.io.FileInputStream;
import java.nio.ByteBuffer;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import uk.ac.dur.cosma.virtual_directory.VirtualFile;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectory;
import uk.ac.dur.cosma.virtual_directory.TarFile;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectoryException;
import uk.ac.dur.cosma.virtual_directory.VirtualPathInfo;
import uk.ac.dur.cosma.virtual_directory.CheckRole;

public class TarFileServlet extends HttpServlet implements Servlet {

    public TarFileServlet() {}

    protected void streamFile(HttpServletRequest request, HttpServletResponse response, boolean is_get,
                              VirtualFile file, String base_name) throws ServletException, IOException {

        // Open the file to send
        File inputFile;
        try {
            inputFile = new File(file.filesystem_path);
        } catch (Exception e) {
            response.sendError(HttpServletResponse.SC_NOT_FOUND, "Unable to open the specified file.");
            return;
        }

        // Open the input stream
        try (FileInputStream input = new FileInputStream(inputFile)) {

            // Get stream to write out the output to
            ServletOutputStream output = response.getOutputStream();

            // Set response headers
            response.setHeader("Content-Type", file.media_type);
            response.setHeader("Content-Length", String.valueOf(inputFile.length()));
            response.setHeader("Content-Disposition", "attachment; filename=\"" + base_name + "\"");

            // Only copy file contents if this is a get request
            if(is_get) {
                int buffer_size = (Integer) getServletContext().getAttribute("buffer_size");
                try {
                    StreamCopier.copyStream(input, output, buffer_size);
                } catch (IOException e) {
                    response.sendError(HttpServletResponse.SC_NOT_FOUND, "Error while reading the specified file.");
                }
            }
        } catch (Exception e) {
            response.sendError(HttpServletResponse.SC_NOT_FOUND, "Unable to open input stream for the specified file.");
        }
        return;
    }

    protected void doGet(HttpServletRequest request, HttpServletResponse response)
	throws ServletException, IOException
    {
	doHeadOrGet(request, response, true);
    }

    protected void doHead(HttpServletRequest request, HttpServletResponse response)
	throws ServletException, IOException
    {
	doHeadOrGet(request, response, false);
    }

    // Handle HEAD or GET requests
    // Procedure is the same in either case except that for HEAD we return without writing the response body
    protected void doHeadOrGet(HttpServletRequest request, HttpServletResponse response, boolean is_get)
	throws ServletException, IOException
    {
        // Get the directory structure
        ServletContext context = getServletContext();
        VirtualDirectory virtual_directory = ((ConfigManager) context.getAttribute("config")).getRoot();

        // Expression which returns true if user belongs to a role:
        // This determines which directories we can see.
        CheckRole in_role;
        String username = null;
        Principal principal = request.getUserPrincipal();
        if(principal != null) {
            username = request.getUserPrincipal().getName();
            in_role = (role_name) -> request.isUserInRole(role_name);
        } else {
            // If we're not logged in, we belong to no roles.
            username = null;
            in_role = (role_name) -> false;
        }

        // Extract directory path from the request. Since this servlet is
        // mapped at /download this does not include the /download prefix.
        String path = request.getPathInfo();

        // If we have a path it will start with a slash, which we need to remove
        if((path != null) && (path.length() > 0)) {
            path = path.substring(1);
        }

        // Check we have a path
        if((path==null) || (path.length() == 0)) {
            response.sendError(HttpServletResponse.SC_BAD_REQUEST, "No file path was specified");
            return;
        }

        // Find the request count
        RequestCounter requestCounter = (RequestCounter) context.getAttribute("request_counter");

        // Get file or directory from the request
        VirtualPathInfo path_info = null;
        try {
            path_info = virtual_directory.resolvePath(path, in_role);
        } catch (VirtualDirectoryException e) {
            response.sendError(HttpServletResponse.SC_NOT_FOUND, "Invalid path specified: "+path);
            return;
        }
        VirtualFile file = path_info.file;
        VirtualDirectory directory = path_info.directory;
        String basename = path_info.basename;

        if(file != null) {

            // Stream a raw data file
            streamFile(request, response, is_get, file, basename);
            requestCounter.logRequest(RequestCounter.FILE, file.length);

        } else if (directory != null){

            // Determine prefix to add to paths in the tar file:
            // Unpacking all tar files in the same base directory should reproduce the
            // virtual directory structure.
            String prefix;
            if(directory.virtual_path.length() > 0)
                prefix = directory.virtual_path + "/";
            else
                prefix = "";

            // Initialize the tar file output
            int buffer_size = 10*1024*1024;
            TarFile tar_file = new TarFile(prefix, directory, buffer_size, in_role);

            // Generate a name for the file
            String tar_name = directory.virtual_path.replace("/","_") + ".tar";
            tar_name = URLEncoder.encode(tar_name, StandardCharsets.UTF_8);

            // Set response headers
            response.setHeader("Content-Type", "application/x-tar");
            response.setHeader("Content-Length", String.valueOf(tar_file.nr_bytes));
            response.setHeader("Content-Disposition", "attachment; filename=\"" + tar_name + "\"");

            // If this is a head request there's nothing more to do
            if(!is_get)return;

            // Write the tar file to the response
            ServletOutputStream output = response.getOutputStream();
            tar_file.write(output);
            requestCounter.logRequest(RequestCounter.DIRECTORY, tar_file.nr_bytes);

        } else {
            // resolvePath should have thrown an exception in this case
            response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR, "File and directory objects are null");
            return;
        }
        return;
    }
}
