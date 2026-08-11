package uk.ac.dur.cosma.hdfstream;

import java.lang.Long;
import java.lang.Integer;
import javax.servlet.Servlet;
import javax.servlet.ServletException;
import javax.servlet.ServletOutputStream;
import javax.servlet.ServletContext;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.security.Principal;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.Map;
import uk.ac.dur.cosma.virtual_directory.VirtualDirectory;
import uk.ac.dur.cosma.virtual_directory.CheckRole;
import org.msgpack.core.MessagePacker;
import org.msgpack.core.MessageUnpacker;
import org.msgpack.core.MessagePack;

import uk.ac.dur.cosma.libhdfstream.*;

public class HDFStreamServlet extends HttpServlet implements Servlet {

    public HDFStreamServlet() {}

    @Override
    protected void doGet(HttpServletRequest request, HttpServletResponse response)
	throws ServletException, IOException
    {
	doHeadOrGet(request, response, true);
    }

    @Override
    protected void doHead(HttpServletRequest request, HttpServletResponse response)
	throws ServletException, IOException
    {
	doHeadOrGet(request, response, false);
    }

    protected void sendMsgpackError(HttpServletResponse response, int status, String message) throws IOException {

        // Set content type which we'll return
        response.setContentType("application/x-msgpack");

        // Set http status code
        response.setStatus(status);

        // Set up msgpack packer
	ServletOutputStream out = response.getOutputStream();
        MessagePacker packer = MessagePack.newDefaultPacker(out);

        // Send msgpack encoded {"error": message} as the body
        packer.packMapHeader(1);
        packer.packString("error");
        packer.packString(message);

        // Flush packing buffer
        packer.close();
    }

    // Get the user name from a request (may be null if not authenticated)
    protected String getUserName(HttpServletRequest request) {
        String username = null;
        Principal principal = request.getUserPrincipal();
        if(principal != null) {
            username = request.getUserPrincipal().getName();
        }
        return username;
    }

    // Return a function to determine if the current user belongs to a role
    protected CheckRole getCheckRole(HttpServletRequest request) {
        CheckRole in_role;
        String username = getUserName(request);
        if(username != null) {
            in_role = (role_name) -> request.isUserInRole(role_name);
        } else {
            in_role = (role_name) -> false;
        }
        return in_role;
    }

    // Handle HEAD or GET requests
    // Procedure is the same in either case except that for HEAD we return without writing the response body
    protected void doHeadOrGet(HttpServletRequest request, HttpServletResponse response, boolean is_get)
	throws ServletException, IOException
    {
        // Get the virtual directory structure
        ServletContext context = getServletContext();
        VirtualDirectory virtual_directory = ((ConfigManager) context.getAttribute("config")).getRoot();

        // Find the cache
        CacheInfo cache_info = (CacheInfo) context.getAttribute("cache_info");

        // Expression which returns true if user belongs to a role:
        // This determines which directories we can see.
        CheckRole in_role = getCheckRole(request);

        // Extract directory path from the request. Since this servlet is
        // mapped at /msgpack this does not include the /msgpack prefix.
        String path = request.getPathInfo();

        // If we have a path it will start with a slash, which we need to remove
        if((path != null) && (path.length() > 0)) {
            path = path.substring(1);
        }

        // Get object name from the request
        String object = request.getParameter("object");

        // Locate the concurrent request count
	ConcurrentRequestCount crc = (ConcurrentRequestCount) context.getAttribute("concurrent_request_count");

        // Get maximum recursion depth
        int max_depth = 0;
        String max_depth_string = request.getParameter("max_depth");
        if(max_depth_string != null) {
            try {
                max_depth = Integer.valueOf(max_depth_string);
            } catch (NumberFormatException e) {
                sendMsgpackError(response, HttpServletResponse.SC_BAD_REQUEST, "Unable to interpret max_depth parameter as integer");
                return;
            }
        }

        // Get maximum data size for eager loading
        long data_size_limit = Integer.MAX_VALUE;
        String data_size_limit_string = request.getParameter("data_size_limit");
        if(data_size_limit_string != null) {
            try {
                data_size_limit = Integer.valueOf(data_size_limit_string);
            } catch (NumberFormatException e) {
                sendMsgpackError(response, HttpServletResponse.SC_BAD_REQUEST, "Unable to interpret data_size_limit parameter as integer");
                return;
            }
        }

        // Get slicing information, if specified
        SliceInfo slice = null;
        String slice_string = request.getParameter("slice");
        if(slice_string != null) {
            try {
                slice = new SliceInfo(slice_string);
            } catch (InvalidSliceException e) {
                sendMsgpackError(response, HttpServletResponse.SC_BAD_REQUEST, e.getMessage());
                return;
            }
        }

        // Look up some parameters we need from the servlet context
        int max_hdf5_name_length = (Integer) context.getAttribute("max_hdf5_name_length");
        int buffer_size = (Integer) context.getAttribute("buffer_size");
        HDFStream hs = (HDFStream) context.getAttribute("hdfstream");

        // Initialize the response
        HDFStreamRequest hreq = null;
        try {
            hreq = new HDFStreamRequest(virtual_directory, in_role, cache_info,
                                        path, object, max_depth, data_size_limit,
                                        slice, max_hdf5_name_length);
        } catch (HDFStreamRequestException e) {
            sendMsgpackError(response, e.getStatusCode(), e.getErrorMessage());
            return;
        }

        // Set the content type of the result
        response.setContentType("application/x-msgpack");

	// Determine identifier used to limit concurrent requests:
	// Username if logged in, and IP address otherwise.
        String username = getUserName(request);
	String identifier = null;
	if(username != null)
	    identifier = username;
	else
	    identifier = request.getRemoteAddr();

        // Find the request count
        RequestCounter requestCounter = (RequestCounter) context.getAttribute("request_counter");

        // Send the response body
        long bytes_written = -1;
        crc.acquire(identifier);
        try {
            ServletOutputStream out = response.getOutputStream();
            bytes_written = hreq.streamResponse(hs, out, buffer_size, is_get);
        } catch (HDFStreamRequestException e) {
            sendMsgpackError(response, e.getStatusCode(), e.getErrorMessage());
            return;
        } finally {
            crc.release(identifier);
        }

        // Log the request (unless it was just a head request)
        if(is_get)requestCounter.logRequest(RequestCounter.MSGPACK, bytes_written);
        return;
    }

    /*
      Handle http POST requests

      These contain the same parameters as a GET, but msgpack encoded in the
      body of the post request.

      TODO: limit reads with limited size input stream?
    */
    protected void doPost(HttpServletRequest request, HttpServletResponse response)
	throws ServletException, IOException
    {
        // Get the virtual directory structure
        ServletContext context = getServletContext();
        VirtualDirectory virtual_directory = ((ConfigManager) context.getAttribute("config")).getRoot();

        // Find the cache
        CacheInfo cache_info = (CacheInfo) context.getAttribute("cache_info");

        // Expression which returns true if user belongs to a role:
        // This determines which directories we can see.
        CheckRole in_role = getCheckRole(request);

        // Extract directory path from the request. Since this servlet is
        // mapped at /msgpack this does not include the /msgpack prefix.
        String path = request.getPathInfo();

        // If we have a path it will start with a slash, which we need to remove
        if((path != null) && (path.length() > 0)) {
            path = path.substring(1);
        }

        // Check that the content type of the request body is msgpack
        String contentType = request.getContentType();
        if (!"application/x-msgpack".equals(contentType)) {
            sendMsgpackError(response, HttpServletResponse.SC_UNSUPPORTED_MEDIA_TYPE, "POST requests must have content type application/x-msgpack");
            return;
        }

        // Get the maximum length of HDF5 names and paths
        int max_hdf5_name_length = (Integer) context.getAttribute("max_hdf5_name_length");

        // Get the input stream for the request body
        InputStream inputStream = request.getInputStream();

        // Unpack parameters from the msgpack encoded request body.
        // We expect a msgpack map where keys are parameter names and values
        // are parameter values. First, set default values.
        String object = null;
        int max_depth = 0;
        long data_size_limit = Integer.MAX_VALUE;
        SliceInfo slice = null;

        // Then try to unpack the request body
        try {
            MessageUnpacker unpacker = MessagePack.newDefaultUnpacker(inputStream);
            // We expect the content to be a msgpack map
            Map<String, Object> data = new HashMap<>();
            int mapSize = unpacker.unpackMapHeader();
            // Check we have no more map entries than we have supported parameters (avoid decoding giant maps!)
            if(mapSize > 4) {
                sendMsgpackError(response, HttpServletResponse.SC_BAD_REQUEST, "Too many parameters in msgpack request");
                return;
            }
            // Loop over the map entries
            Map<String, Integer> paramCounts = new HashMap<>();
            for (int i = 0; i < mapSize; i++) {
                String name = unpacker.unpackString();
                // Count how many times we've seen this parameter name and abort if we have duplicates
                paramCounts.put(name, paramCounts.getOrDefault(name, 0) + 1);
                if(paramCounts.get(name) > 1) {
                    sendMsgpackError(response, HttpServletResponse.SC_BAD_REQUEST, "Duplicate parameter: "+name);
                    return;
                }
                // Identify and decode this parameter
                switch(name) {
                case "object":
                    object = MsgpackUtils.unpackLimitedString(unpacker, max_hdf5_name_length);
                    break;
                case "max_depth":
                    max_depth = unpacker.unpackInt();
                    break;
                case "data_size_limit":
                    data_size_limit = unpacker.unpackLong();
                    break;
                case "slice":
                    // Slice could be expressed as a string or as nested arrays of integers
                    slice = SliceInfo.fromUnpacker(unpacker);
                    break;
                default:
                    sendMsgpackError(response, HttpServletResponse.SC_BAD_REQUEST, "Unrecognized parameter: "+name);
                    return;
                }
            }
        } catch (Exception e) {
            sendMsgpackError(response, HttpServletResponse.SC_BAD_REQUEST, "Unable to decode request: "+e.getMessage());
            return;
        }

        // Locate the concurrent request count
	ConcurrentRequestCount crc = (ConcurrentRequestCount) context.getAttribute("concurrent_request_count");

        // Look up some parameters we need from the servlet context
        int buffer_size = (Integer) context.getAttribute("buffer_size");
        HDFStream hs = (HDFStream) context.getAttribute("hdfstream");

        // Initialize the response
        HDFStreamRequest hreq = null;
        try {
            hreq = new HDFStreamRequest(virtual_directory, in_role, cache_info,
                                        path, object, max_depth, data_size_limit,
                                        slice, max_hdf5_name_length);
        } catch (HDFStreamRequestException e) {
            sendMsgpackError(response, e.getStatusCode(), e.getErrorMessage());
            return;
        }

        // Set the content type of the result
        response.setContentType("application/x-msgpack");

	// Determine identifier used to limit concurrent requests:
	// Username if logged in, and IP address otherwise.
        String username = getUserName(request);
	String identifier = null;
	if(username != null)
	    identifier = username;
	else
	    identifier = request.getRemoteAddr();

        // Find the request count
        RequestCounter requestCounter = (RequestCounter) context.getAttribute("request_counter");

        // Send the response body
        long bytes_written = -1;
        crc.acquire(identifier);
        try {
            ServletOutputStream out = response.getOutputStream();
            bytes_written = hreq.streamResponse(hs, out, buffer_size, true);
        } catch (HDFStreamRequestException e) {
            sendMsgpackError(response, e.getStatusCode(), e.getErrorMessage());
            return;
        } finally {
            crc.release(identifier);
        }

        // Log the request
        requestCounter.logRequest(RequestCounter.MSGPACK, bytes_written);
        return;
    }
}
